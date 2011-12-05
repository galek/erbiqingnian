//----------------------------------------------------------------------------
// FILE: QuestMessagePump.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
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
#include "QuestProperties.h"
#include "party.h"


QUEST_MESSAGE_RESULT QuestHandleQuestMessage( UNIT * pPlayerCausingMessage,
											  QUEST_MESSAGE_TYPE eMessageType,
											  void *pMessage )
{
		
		// send to quests for all participating players
		if( !pPlayerCausingMessage ) 
			return QMR_OK;
		GAME *pGame = UnitGetGame( pPlayerCausingMessage );
		UNIT *pPlayer = pPlayerCausingMessage;
		ASSERT_RETVAL( pPlayer, QMR_OK );
		LEVEL *pLevel = UnitGetLevel( pPlayer );
		int nOrginalPlayerLevelAreaID = pLevel ? LevelGetLevelAreaID( pLevel ) : INVALID_ID;
		if( UnitIsInLimbo( pPlayer ) )
		{
			return QMR_OK;
		}

		//some messages are only for a single client
		switch( eMessageType )
		{
		case QM_SERVER_LEAVE_LEVEL:
			{
				QUEST_MESSAGE_LEAVE_LEVEL *message = (QUEST_MESSAGE_LEAVE_LEVEL *)pMessage;
				if( message &&
					nOrginalPlayerLevelAreaID != message->nLevelArea )
				{
					s_QuestPlayerLeftArea( pPlayer, nOrginalPlayerLevelAreaID, message->nLevelArea );
				}
			}
			break;
		case QM_SERVER_MONSTER_KILL:
			{
				if( !pMessage )
					break;
				QUEST_MESSAGE_MONSTER_KILL *message = (QUEST_MESSAGE_MONSTER_KILL *)pMessage;
				UNIT *pVictim = UnitGetById( pGame, message->idVictim );
				if( pVictim && QuestUnitIsQuestUnit( pVictim ) )
				{
					int nClass = UnitGetClass( pVictim );
					if( nClass == INVALID_ID )
						break;
					const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskActive = QuestUnitGetTask( pVictim );					
					if( pQuestTaskActive == NULL )
						break;	//didn't find the task
					int nIndex = QuestUnitGetIndex( pVictim );  //get the index of the boss
					//const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask( pQuestTaskActive );
					//int nClassBossID = MYTHOS_QUESTS::QuestGetBossIDToSpawnByIndex( pPlayer, pQuestTaskActive, nIndex );
					ASSERT_RETVAL( MYTHOS_QUESTS::QuestGetBossIDToSpawnByIndex( pPlayer, pQuestTaskActive, nIndex ) != INVALID_ID, QMR_OK );
					QuestUpdateWorld( pGame, pPlayer, UnitGetLevel( pPlayer ), pQuestTaskActive, QUEST_WORLD_UPDATE_QSTATE_BOSS_DEFEATED, pVictim  );
				}
			}
			break;
		case QM_SERVER_TALK_CANCELLED:
			{
				if( !pMessage )
					break;
				QUEST_MESSAGE_TALK_CANCELLED *tMessage = (QUEST_MESSAGE_TALK_CANCELLED *)pMessage;				
				UNIT *pTalkingTo = UnitGetById( pGame, tMessage->idTalkingTo );								
				//int nDialogID = tMessage->nDialog;
				int nQuestID = UnitGetStat( pPlayer, STATS_QUEST_CURRENT );
				const QUEST_TASK_DEFINITION_TUGBOAT *pCurrentQuestTask = QuestNPCGetQuestTask( pGame, pPlayer, pTalkingTo, nQuestID, FALSE );
				if( pCurrentQuestTask && QuestGetQuestAccepted( pGame, pPlayer, pCurrentQuestTask ) == FALSE )
				{				
					//s_QuestSetVistedClear( pPlayer, pCurrentQuestTask );
				}
				else if( pCurrentQuestTask )
				{
					BOOL bUpdated( FALSE );
					if( pTalkingTo )
					{
						for( int t = 0; t < MAX_PEOPLE_TO_TALK_TO; t++ )
						{
							if( pCurrentQuestTask->nQuestSecondaryNPCTalkTo[ t ] == INVALID_ID )
								break;
							if( pCurrentQuestTask->nQuestSecondaryNPCTalkTo[ t ] == pTalkingTo->pUnitData->nNPC )
							{
								s_QuestTaskClearMask( pPlayer, pCurrentQuestTask, t, QUESTTASK_SAVE_NPC ); //save the fact we talk to them
								bUpdated = TRUE;
							}
						}
					}	
					if( bUpdated )
					{
						//we updated so we need to update the world!
						QuestUpdateQuestNPCs( pGame, pPlayer, pTalkingTo );
						//updates the world - slow
						//QuestUpdateNPCsInLevel( pGame, pPlayer, UnitGetLevel( pTalkingTo ) );
					}
				}

			}
			return QMR_OK;
		case QM_SERVER_TALK_STOPPED:	//accepted button
			{
				if( !pMessage )
					break;
				QUEST_MESSAGE_TALK_STOPPED *tMessage = (QUEST_MESSAGE_TALK_STOPPED *)pMessage;
				const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask( NULL );
				int nQuestID = UnitGetStat( pPlayer, STATS_QUEST_CURRENT );	
				UNIT *pTalkingTo = UnitGetById( pGame, tMessage->idTalkingTo );								
				BOOL bQuestHasBeenAccepted( FALSE );
				const QUEST_TASK_DEFINITION_TUGBOAT *pCurrentQuestTask = QuestNPCGetQuestTask( pGame, pPlayer, pTalkingTo, nQuestID, FALSE );
				for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
				{
					//Quest task definition
					
					pQuestTask = QuestGetUnitTaskByQuestQueueIndex( pPlayer, i );
					if( pQuestTask )
					{								
						BOOL questTaskValid = ( pQuestTask->nNPC == pTalkingTo->pUnitData->nNPC ) | ( pQuestTask->nNPC == -1 && UnitIsGodMessanger( pTalkingTo ) );
						if( nQuestID != INVALID_ID &&
							pQuestTask->nParentQuest != nQuestID )
						{
							questTaskValid = FALSE;
						}
						if( !questTaskValid )
						{
							for( int t = 0; t < MAX_PEOPLE_TO_TALK_TO; t++ )
							{
								if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == pTalkingTo->pUnitData->nNPC &&
									( pQuestTask->nQuestNPCTalkToDialog[ t ] == tMessage->nDialog ||
									  pQuestTask->nQuestNPCTalkQuestCompleteDialog[ t ] == tMessage->nDialog ) )
								{
									questTaskValid = TRUE;
									break;
								}
							}
						}
						if( questTaskValid  &&
							( !pCurrentQuestTask || pQuestTask == pCurrentQuestTask ) )
						{
							bQuestHasBeenAccepted = TRUE;
							//We must of accepted
							
							if( QuestGetQuestAccepted( pGame, pPlayer, pQuestTask ) )
							{
								//Award items for talking or completing a quest
								s_QuestPlayerTalkedToQuestNPC( pPlayer, pTalkingTo, pQuestTask, tMessage->nDialog, tMessage->nRewardSelection );																	
								if( QuestIsQuestComplete( pPlayer, pQuestTask->nParentQuest ) ) //must of completed it
								{
									const QUEST_TASK_DEFINITION_TUGBOAT *pNextQuestTask = QuestNPCGetQuestTask( pGame, pPlayer, pTalkingTo );
									if( pNextQuestTask )
									{
										s_QuestEventInteract( pPlayer, pTalkingTo, pNextQuestTask->nParentQuest ); //show next dialog		
									}
								}
								//break;
							}
						}	
					}					
				}		
				if( !bQuestHasBeenAccepted )
				{
					const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestNPCGetQuestTask( pGame, pPlayer, pTalkingTo, nQuestID );
					//Say we accepted it.
					if( pQuestTask &&
						QuestCanStart( pGame, pPlayer, pQuestTask->nParentQuest  ))						
					{
						const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest ); 				
						int iLevel = UnitGetExperienceLevel( pPlayer );
						if( pQuest &&
							!( pQuest->nQuestPlayerStartLevel != INVALID_ID &&
							   iLevel < pQuest->nQuestPlayerStartLevel )  ) //&&
							//!( pQuest->nQuestPlayerEndLevel != INVALID_ID &&
							//  iLevel > pQuest->nQuestPlayerEndLevel ) )
						{
							s_QuestAccepted( pGame, pPlayer, pTalkingTo, pQuestTask ); //say we accepted it
							//s_QuestEventInteract( pPlayer, pTalkingTo, pQuestTask->nParentQuest ); //show next dialog								
							//BOOL isPotentiallyComplete = QuestAllTasksCompleteForQuestTask( pPlayer, pQuestTask->nParentQuest, TRUE );
							//if( isPotentiallyComplete )
							{
								//s_QuestSetVisited( pPlayer, pQuestTask );
								s_QuestUpdate( pGame, pPlayer, pTalkingTo );
								s_QuestGiveItemsOnAccepted( pPlayer, pQuestTask );
								s_QuestPlayerTalkedToQuestNPC( pPlayer, pTalkingTo, pQuestTask, tMessage->nDialog, tMessage->nRewardSelection );
								
							}							
						}

					}

				}
				QuestUpdateWorld( pGame, pPlayer, UnitGetLevel( pPlayer ), NULL, QUEST_WORLD_UPDATE_QSTATE_AFTER_TALK_NPC, pTalkingTo ); 
			}
			return QMR_OK;
		case QM_SERVER_INTERACT:
			{
				if( !pMessage )
					break;
				QUEST_MESSAGE_INTERACT *tMessage = (QUEST_MESSAGE_INTERACT *)pMessage;
				UNIT *pPlayer = UnitGetById( pGame, tMessage->idPlayer );
				UNIT *pSubject = UnitGetById( pGame, tMessage->idSubject );		
				return s_QuestTalkToNPC( pPlayer, pSubject, tMessage->nForcedQuestID );				

			}
		case QM_SERVER_ATTEMPTING_PICKUP:
			{
				if( !pMessage )
					break;
				QUEST_MESSAGE_ATTEMPTING_PICKUP *message = (QUEST_MESSAGE_ATTEMPTING_PICKUP *)pMessage;
				if( message->pPickedUp &&
					QuestUnitIsQuestUnit( message->pPickedUp ) == FALSE )
				{
					s_QuestNoneQuestItemNeedsToBeQuestItem( message->pPlayer, message->pPickedUp );
				}
				//we had this in before we allowed the player to drop quest items.
				//now players can only drop quest items when the quest is over.
				/*
				QUEST_MESSAGE_ATTEMPTING_PICKUP *message = (QUEST_MESSAGE_ATTEMPTING_PICKUP *)pMessage;
				//need to check to see if the unit can be sent to the client : look in items ln:1369
				if( QuestUnitIsQuestUnit( message->pPickedUp ) ) //see if it's a quest item
				{
				
					DWORD dwInvIterateFlags = 0;
					SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
					SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );		
				
					int count = UnitInventoryCountUnitsOfType( pPlayer, MAKE_SPECIES( UnitGetGenus( message->pPickedUp ), UnitGetClass( message->pPickedUp ), dwInvIterateFlags );
					int totalNeeded = QuestGetTotalNumberOfItemsNeeded( pGame, pPlayer, UnitGetClass( message->pPickedUp ) ); 
					if( count >= totalNeeded )
						return QMR_USE_FAIL;
				}
				*/
			}
			return QMR_OK;
		case QM_SERVER_PICKUP:				// item was picked up	
				{
					if( !pMessage )
						break;					
					QUEST_MESSAGE_PICKUP *message = (QUEST_MESSAGE_PICKUP *)pMessage;
					if( QuestUnitIsQuestUnit( message->pPickedUp ) )
					{
						if( s_QuestItemPickedUp( message->pPickedUp,
												 message->pPlayer ) )
						{
							//nothing
						}

						s_QuestTaskPotentiallyComplete( message->pPlayer );

					}
				}
				return QMR_OK;	
		case QM_SERVER_OBJECT_OPERATED:		// object has been operated
				{
					if( !pMessage )
						break;
					QUEST_MESSAGE_OBJECT_OPERATED *message = (QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
					if( QuestUnitIsQuestUnit( message->pTarget ) )
					{
						//update the scripts
						QuestUpdateWorld( pGame, pPlayer, UnitGetLevel(pPlayer ), QuestUnitGetTask( message->pTarget ), QUEST_WORLD_UPDATE_QSTATE_INTERACT_QUESTUNIT,  message->pTarget );
					}

				}
				break;
			// *CLIENT* quest messages
		case QM_CLIENT_OPEN_UI:				// player has opened a ui element
				return QMR_OK;	
		case QM_CLIENT_SKILL_POINT:			// player has received skill point
				return QMR_OK;	
		case QM_CLIENT_QUEST_STATE_CHANGED:	// state has changed
				return QMR_OK;	
		case QM_CLIENT_QUEST_STATUS:			// state has become complete
				return QMR_OK;
		case QM_CLIENT_MONSTER_INIT:			// monster being initialized on client
		case QM_IS_UNIT_VISIBLE:
			{
				UNIT *pTargetUnit( NULL );
				if( !pMessage )
					break;
				if( eMessageType == QM_CLIENT_MONSTER_INIT )
				{
					QUEST_MESSAGE_MONSTER_INIT *message = (QUEST_MESSAGE_MONSTER_INIT *)pMessage;
					pTargetUnit = message->pMonster;
				}
				else
				{
					QUEST_MESSAGE_IS_UNIT_VISIBLE *message = (QUEST_MESSAGE_IS_UNIT_VISIBLE *)pMessage;
					pTargetUnit = message->pUnit;
				}
				if( IS_CLIENT( pGame ) &&
					pTargetUnit )
				{
					LEVEL *pLevel = UnitGetLevel( pTargetUnit );
					
					if( pLevel )
					{
						MYTHOS_LEVELAREAS::ExecuteLevelAreaScript( pLevel,
																   MYTHOS_LEVELAREAS::KSCRIPT_LEVELAREA_UNITINIT,																   
																   GameGetControlUnit( pGame ),
																   pTargetUnit );
					}
				}
			}				
				return QMR_OK;
			// either client or server quest messages
		case QM_CAN_OPERATE_OBJECT:			// can a player operate a thing (like a chest, door etc)
				return QMR_OK;	
		case QM_CAN_USE_ITEM:	
				return QMR_OK;	

 
		}

		
		PARTYID idPlayerParty = pPlayerCausingMessage ? UnitGetPartyId( pPlayerCausingMessage ) : INVALID_ID;
		for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
			 pClient;
			 pClient = ClientGetNextInGame( pClient ))
		{
			
			//if the player isn't in a party lets only do the player; even if there is a billion people on the server. At 
			//the end of this for loop it breaks if idParty==invalid
			if( idPlayerParty != INVALID_ID )
			{
				pPlayer = ClientGetPlayerUnit( pClient );			

				ASSERTX_CONTINUE( pPlayer, "No player for in game client" );			
				if( UnitGetPartyId( pPlayer ) != idPlayerParty )
				{
					continue; // you have to be in the same dungeon
				}
			}
			else
			{
				pPlayer = pPlayerCausingMessage;
			}
			switch( eMessageType )
			{
			case QUEST_MESSAGE_INVALID:
				break;	


			case QM_SERVER_MONSTER_KILL:	// a monster has died
				{			
					if( !pMessage )
						break;
					QUEST_MESSAGE_MONSTER_KILL *message = (QUEST_MESSAGE_MONSTER_KILL *)pMessage;
					UNIT *pVictim = UnitGetById( pGame, message->idVictim );					
					UNIT *pKiller = UnitGetById( pGame, message->idKiller );	
					int nBossIndex = QuestUnitGetIndex( pVictim );
					const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( pVictim );
					if( pQuestTask )
					{
						UNITID monsterSpawn = MYTHOS_QUESTS::QuestGetBossIDToSpawnByIndex( pPlayer, pQuestTask, nBossIndex );
						if( UnitGetId( pVictim ) == monsterSpawn &&
							UnitGetPartyId( pPlayer ) != idPlayerParty )
						{
							break;
						}
					}
					//spawns items from UNITS created from Objects placed in levels	
					s_QuestSpawnItemFromCreatedObject( pVictim, pKiller, pPlayer, true );

					//spawns items from monster classes
					QuestSpawnQuestItems( pGame, pVictim, pKiller, pPlayer );
					//Updates the quest to see if it's complete or not
					s_QuestTaskPotentiallyComplete( pPlayer );



				}
				break;
			case QM_SERVER_ENTER_LEVEL:		// a player has entered into a level
				{
					if( pPlayer &&
						UnitIsInLimbo( pPlayer ),
						UnitGetLevel( pPlayer ) ) 
					{						
					}
				}
				break;
			case QM_SERVER_LEAVE_LEVEL:		// a player has left a level
				break;
			case QM_SERVER_SUB_LEVEL_TRANSITION:	// a player has entered/left a sub level
				break;
			case QM_SERVER_OBJECT_OPERATED:		// object has been operated
				{
					if( !pMessage )
						break;
					QUEST_MESSAGE_OBJECT_OPERATED *message = (QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
					BOOL bIndexInteracted( FALSE );
					//Create any items that need to be created
					//this case is a bit strange but we need to make sure the item spawns so it 
					//can spawn the item the player needs to pick up. So we won't mark this done
					//till the player picks up the item. Meaning we need to make sure it's false
					bIndexInteracted = !s_QuestSpawnItemFromCreatedObject( message->pTarget,																		   
																		   message->pOperator,
																		   pPlayer,
																		   false );

					//Remove any items that need to be removed.
					bIndexInteracted = bIndexInteracted & s_ObjectRemoveItemOnInteract( pPlayer, message->pTarget );

					//start any skills that need be starting
					bIndexInteracted = bIndexInteracted & s_ObjectOperatedUseSkill( pPlayer, message->pTarget );
					

					//lets mark the object as interacted upon
					if( bIndexInteracted  )
					{
						bIndexInteracted = FALSE;
						int taskID = UnitGetStat( message->pTarget, STATS_QUEST_TASK_REF );
						int index = UnitGetStat( message->pTarget, STATS_QUEST_ID_REF );
						//must be a quest created from an object						
						const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestTaskDefinitionGet( taskID ); 
						if( pQuestTask &&
							index >= 0 )
						{
							bIndexInteracted = TRUE;
							s_SetTaskObjectCompleted( pGame,
													  pPlayer,
													  pQuestTask,
													  index,
													  FALSE );
						}
					}

					//send any messages that need sending
					s_QuestBroadcastMessageOnObjectOperate( message->pTarget, pPlayer );

					//Updates the quest to see if it's complete or not
					if( bIndexInteracted )
					{
						s_QuestTaskPotentiallyComplete( message->pOperator );
					}

				}
				break;
			case QM_SERVER_LOOT:				// unit spawned loot, here it is
				break;

			case QM_SERVER_USE_ITEM:			// use this quest item
				break;
			}
	

			if( idPlayerParty == INVALID_ID )
				break;	//we only have 1 person in the party so we can break out
		
		}
	return QMR_OK;
}
