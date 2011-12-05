#include "stdafx.h"
#include "s_QuestNPCServer.h"
#include "states.h"
#include "stringtable.h"
#include "uix.h"
#include "globalindex.h"
#include "s_QuestServer.h"
#include "s_quests.h"
#include "inventory.h"
#include "items.h"
#include "npc.h"
#include "QuestProperties.h"
#include "gameglobals.h"
#include "gossip.h"



//this checks active tasks for any NPC's that need to talk to the player
int s_QuestNPCGetSecondaryDialogInActiveTask( GAME *pGame,
											 UNIT *pPlayer,
											 UNIT *pNPC,
											 const QUEST_TASK_DEFINITION_TUGBOAT *pReturnsQuestTask )
{
	if( pPlayer == NULL ||
		pNPC == NULL ||
		pReturnsQuestTask == NULL )
		return INVALID_ID;
	if( UnitCanInteractWith( pNPC, pPlayer ) == FALSE )
		return INVALID_ID;
	//first lets check to see if it's a Quest Object and if so if it has a dialog to show
	const UNIT_DATA *pNPCUnitData = UnitGetData( pNPC );	
	for( int t = 0; t < MAX_PEOPLE_TO_TALK_TO; t++ )
	{
		if( pReturnsQuestTask->nQuestSecondaryNPCTalkTo[ t ] == INVALID_ID )
			break;
		if( pReturnsQuestTask->nQuestSecondaryNPCTalkTo[ t ] == pNPCUnitData->nNPC )
		{
			s_QuestTaskSetMask( pPlayer, pReturnsQuestTask, t, QUESTTASK_SAVE_NPC ); //save the fact we talk to them
			if( QuestAllTasksCompleteForQuestTask( pPlayer, pReturnsQuestTask->nParentQuest ) == false ||
				pReturnsQuestTask->nQuestNPCTalkQuestCompleteDialog[ t ] == INVALID_ID )
			{
				return pReturnsQuestTask->nQuestNPCTalkToDialog[ t ]; //return Dialog ID to show
			}
			else
			{
				return pReturnsQuestTask->nQuestNPCTalkQuestCompleteDialog[ t ]; //return Dialog ID to show
			}
		}
	}
	return INVALID_ID;
}

int s_QuestGiveLevelAreaForTalkingWithNPC( UNIT *pPlayer,
										   UNIT *pNPC,
										   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERTX_RETZERO( pPlayer, "NO pPlayer ( s_QuestGiveLevelAreaForTalkingWithNPC )" );
	ASSERTX_RETZERO( pNPC, "NO pNPC ( s_QuestGiveLevelAreaForTalkingWithNPC )" );
	ASSERTX_RETZERO( pQuestTask, "NO pQuestTask ( s_QuestGiveLevelAreaForTalkingWithNPC )" );
#ifndef	MYTHOS_QUEST_MAPS_DISABLED	
	for( int t = 0; t < MAX_DUNGEONS_TO_GIVE; t++ )
	{
		if( pQuestTask->nQuestLevelAreaNPCToCommunicateWith[ t ] == pNPC->pUnitData->nNPC )
		{
			return MYTHOS_MAPS::s_CreateGenericMaps_FromAreaIDArray( pPlayer, &pQuestTask->nQuestLevelAreaNPCCommunicate[0], MAX_DUNGEONS_TO_GIVE, TRUE, TRUE );
		}
	}
#endif
	return 0;
	
}



inline void SetNPCRandomSeed( UNIT *pNPC )
{
	
	ASSERT_RETURN( pNPC );
	GAME *pGame = UnitGetGame( pNPC );
	ASSERT_RETURN( pGame );
	if( UnitIsMerchant( pNPC ) )
		return; //Merchants can't be random quest givers.	
	DWORD dwNow = GameGetTick( pGame );
	if( UnitGetStat( pNPC, STATS_NPC_RANDOM_QUEST_SEED ) <= 0 )
	{				
		//set the number of ticks its going to take before the refresh of the seed occurs
		//we don't save this!!!! The random seed is saved for the quest, no need to save it for the NPCs.
		UnitSetStat( pNPC, STATS_NPC_RANDOM_QUEST_REFRESH_TICK, dwNow + NPC_QUEST_REFRESH_TICKS );  
		while( UnitGetStat( pNPC, STATS_NPC_RANDOM_QUEST_SEED ) <= 0 )					//random number could be zero.
		{
			int nRandomSeed = RandGetNum( pGame, 0, 0xFFFF );
			UnitSetStat( pNPC, STATS_NPC_RANDOM_QUEST_SEED, nRandomSeed );
		}	
	}
	else 
	{
		int dwLastBrowseTick = (int)UnitGetStat( pNPC, STATS_NPC_RANDOM_QUEST_REFRESH_TICK );
		dwLastBrowseTick -= (int)dwNow;
		if( dwLastBrowseTick <= 0 )
		{						
			UnitSetStat( pNPC, STATS_NPC_RANDOM_QUEST_SEED, 0 );			
			SetNPCRandomSeed( pNPC );	//reset the random seed.
		}

	}
}


//For talking to NPC's
QUEST_MESSAGE_RESULT s_QuestTalkToNPC( UNIT *pPlayer,
									   UNIT *pNPC,
									   int nQuestID  )
{
	QUEST_MESSAGE_RESULT eResult = QMR_INVALID;
	if( !pPlayer,
		!pNPC )
		return eResult;
	GAME *pGame = UnitGetGame( pPlayer );
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask( NULL );
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskUpdateWorld( NULL );
	const QUEST_TASK_DEFINITION_TUGBOAT *pSecondaryQuestTaskUpdate( NULL );
	int SecondaryDialogID( INVALID_ID );
	//if the quest ID is invalid then we need to figure out how many different things 
	//they can say or do..
	UnitSetStat( pPlayer, STATS_QUEST_CURRENT, nQuestID );
	//updates the NPC's random seed for quests. This gets sent to the player
	SetNPCRandomSeed( pNPC );
	
	if( nQuestID == INVALID_ID )
	{
		
		//this does the magic of showing other dialogs if there are...
		int nQuests = QuestGetPossibleTasks( pPlayer, pNPC );
		if( nQuests > 1 )
		{

			// init message
			MSG_SCMD_AVAILABLE_QUESTS tMessage;
			tMessage.nDialog = INVALID_ID;						
			// set giver
			tMessage.idQuestGiver = UnitGetId( pNPC );
			tMessage.nQuest = INVALID_ID;
			tMessage.nQuestTask = INVALID_ID;

			// send it
			GAMECLIENTID idClient = UnitGetClientId( pPlayer );
			s_SendMessage( pGame, idClient, SCMD_AVAILABLE_QUESTS, &tMessage );
			// we did talking, yay!
			eResult = QMR_NPC_TALK;
			return eResult;
		}
		//Either no dialogs or just one..
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskArray[MAX_NPC_QUEST_TASK_AVAILABLE];
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskSecondaryArray[MAX_NPC_QUEST_TASK_AVAILABLE];
		int nQuestTaskCount = QuestGetAvailableNPCQuestTasks( pGame,
													pPlayer,
													pNPC,
													pQuestTaskArray,
													TRUE,
													0 );

		int nQuestTaskSecondaryCount = QuestGetAvailableNPCSecondaryDialogQuestTasks( pGame,
																					 pPlayer,
																					 pNPC,
																					 pQuestTaskSecondaryArray,
																					 0 );
		//Saying nothing. Might as well return.
		if( nQuestTaskCount <= 0 &&
			nQuestTaskSecondaryCount <= 0 )
		{
			return eResult; //invalid			
		}
		else if( nQuestTaskSecondaryCount > 0 )
		{
			pSecondaryQuestTaskUpdate = pQuestTaskSecondaryArray[ 0 ];			
			SecondaryDialogID = s_QuestNPCGetSecondaryDialogInActiveTask( pGame, pPlayer, pNPC, pSecondaryQuestTaskUpdate );			
			pQuestTaskUpdateWorld = pSecondaryQuestTaskUpdate;

		}
		else
		{
			pQuestTask = pQuestTaskArray[ 0 ];
			pQuestTaskUpdateWorld = pQuestTask;
			QuestStampRandomQuestSeed( pPlayer, pNPC, pQuestTaskUpdateWorld ); //lets set the random seed for the quest
		}
	}
	else
	{
		//sometimes a secondary NPC completes the quest so we have to grab a secondary dialog for the NPC
		pQuestTaskUpdateWorld = pSecondaryQuestTaskUpdate = QuestGetActiveTask( pPlayer, nQuestID );
		SecondaryDialogID = s_QuestNPCGetSecondaryDialogInActiveTask( pGame, pPlayer, pNPC, pSecondaryQuestTaskUpdate );

	}
	//this can happen when a random quest giver first has a normal quest and then you complete it and it THEN gives you a random one.
	if( UnitGetStat( pPlayer, STATS_QUEST_RANDOM_SEED_TUGBOAT ) == 0 &&
		isQuestRandom( pQuestTaskUpdateWorld ) &&
		pQuestTaskUpdateWorld )
	{
		QuestStampRandomQuestSeed( pPlayer, pNPC, pQuestTaskUpdateWorld ); //lets set the random seed for the quest
	}



	if( SecondaryDialogID != INVALID_ID )
	{
		BOOL bComplete;
		s_QuestUpdate( pGame, pPlayer, pNPC, pSecondaryQuestTaskUpdate->nParentQuest, bComplete  );	//update the tasks so that it might compelte		
		if( pSecondaryQuestTaskUpdate &&
			pSecondaryQuestTaskUpdate->bQuestSecondaryNPCDialogToParty )
		{
			s_NPCTalkStartToTeam( pPlayer, pNPC, bComplete ? NPC_DIALOG_OK_REWARD : NPC_DIALOG_OK, SecondaryDialogID);
		}
		else
		{
			s_NPCTalkStart( pPlayer, pNPC, bComplete ? NPC_DIALOG_OK_REWARD : NPC_DIALOG_OK, SecondaryDialogID, INVALID_LINK, GI_INVALID, pQuestTaskUpdateWorld ? pQuestTaskUpdateWorld->nTaskIndexIntoTable : INVALID_LINK );
		}
		eResult = QMR_NPC_TALK;			
	}
	else if( pQuestTask != NULL ||
		     nQuestID != INVALID_ID )
	{
		//first check for quest
		if( pQuestTask == NULL )
		{
			pQuestTask = QuestGetActiveTask( pPlayer, nQuestID );
		}
		if( pQuestTaskUpdateWorld == NULL )
		{
			pQuestTaskUpdateWorld = pQuestTask;
		}

#if !ISVERSION(CLIENT_ONLY_VERSION)
		// make any reward items
		s_QuestDialogMakeRewardItemsAvailableToClient( pPlayer, pQuestTaskUpdateWorld, FALSE );
		s_QuestCreateRewardItems( pPlayer, pQuestTaskUpdateWorld);
#endif
		//First we must check to see if it's a dialog from the quest
		if( pQuestTask &&
			QuestUnitIsQuestUnit( pNPC ) &&
			pQuestTask->nQuestDialogOnTaskCompleteAnywhere != INVALID_LINK )
		{			
			if( pQuestTask->bQuestNPCDialogToParty )
			{
				s_NPCTalkStartToTeam( pPlayer, pNPC, NPC_DIALOG_OK, pQuestTask->nQuestDialogOnTaskCompleteAnywhere);
			}
			else
			{
				s_NPCTalkStart( pPlayer, pNPC, NPC_DIALOG_OK, pQuestTask->nQuestDialogOnTaskCompleteAnywhere, INVALID_LINK, GI_INVALID, pQuestTask ? pQuestTask->nTaskIndexIntoTable : INVALID_LINK );
			}
			eResult = QMR_NPC_TALK;
		}else if( pQuestTask )
		{
			const QUEST_TASK_DEFINITION_TUGBOAT *pPrevQuestTask = pQuestTask;
			BOOL bComplete;
			pQuestTask = s_QuestUpdate( pGame, pPlayer, pNPC, pQuestTask->nParentQuest, bComplete );	
			if( pQuestTask != NULL )
			{
				int nDialog( QuestGetDialogID( pGame, pPlayer, pQuestTask ) );
				
				//when can this happen? When the player just needs to talk
				//to the NPC. THe npc doesn't necasarly need to say anything..
				if( nDialog != INVALID_ID )
				{
					// talk to player
					NPC_DIALOG_TYPE nDialogType = NPC_DIALOG_OK;
					if( QuestPlayerHasTask( pPlayer, pQuestTask ) )
					{
						if( QuestHasRewards( pQuestTask ) && 
							s_QuestIsWaitingToGiveRewards( pPlayer, pQuestTask ))
						{
							nDialogType = NPC_DIALOG_OK_REWARD;
						}
					}
					else
					{
						nDialogType = NPC_DIALOG_OK_CANCEL;	//This is saying we don't have the quest. So we should be able to accept it or not
					}

					if( pQuestTask->bQuestNPCDialogToParty )
					{
						s_NPCTalkStartToTeam( pPlayer, pNPC, nDialogType, nDialog);
					}
					else
					{
						s_NPCTalkStart( pPlayer, pNPC, nDialogType, nDialog, INVALID_LINK, GI_INVALID, pQuestTask ? pQuestTask->nTaskIndexIntoTable : INVALID_LINK );
					}

					// we did talking, yay!
					eResult = QMR_NPC_TALK;
				}
				/*
				}
				else if( pQuestTask &&
						 introDialogID >= 0 ) //we need to accept the Quest first
				{
					// init message
					MSG_SCMD_AVAILABLE_QUESTS tMessage;
					tMessage.nDialog = introDialogID;						
					// set giver
					tMessage.idQuestGiver = UnitGetId( pNPC );
					tMessage.nQuest = pQuestTask->nParentQuest;
					tMessage.nQuestTask = pQuestTask->nTaskIndexIntoTable;

					// send it
					GAMECLIENTID idClient = UnitGetClientId( pPlayer );
					s_SendMessage( pGame, idClient, SCMD_AVAILABLE_QUESTS, &tMessage );
					// we did talking, yay!
					eResult = QMR_NPC_TALK;
				}
				*/
				
			}
			else	//the NPC doesn't have a Dialog to say so lets
			{
				//Some NPC's Do have things to say but you could have to many
				//quests or your to low level. So lets check that....
				int iLevel = UnitGetExperienceLevel( pPlayer );
				//we are giving a new quest but we need to make sure the level of the player is below the max level the quest will work for
				const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pPrevQuestTask->nParentQuest ); 				
				if( pQuest->nQuestPlayerStartLevel != INVALID_ID &&
					iLevel < pQuest->nQuestPlayerStartLevel )
				{
					//To Low of level
					if( UnitIsGodMessanger( pNPC ) )
					{
						s_NPCTalkStart( pPlayer, pNPC, NPC_DIALOG_OK, DIALOG_QUEST_TO_LOW_LEVEL_GOD_MESSANGER );
					}
					else
					{
						if( s_GossipHasGossip( pPlayer, pNPC ) == FALSE )
						{
							s_NPCTalkStart( pPlayer, pNPC, NPC_DIALOG_OK, DIALOG_QUEST_TO_LOW_LEVEL );
						}
						else
						{
							return QMR_INVALID;
						}
					}
					eResult = QMR_NPC_TALK;
				}
				else if( pPrevQuestTask &&
						 !QuestCanGiveItemsOnAccept( pPlayer, pPrevQuestTask ) )
				{
					s_NPCTalkStart( pPlayer, pNPC, NPC_DIALOG_OK, DIALOG_QUEST_TO_MANY_ITEMS );
					eResult = QMR_NPC_TALK;			
				}
				else //if( pQuest->nQuestPlayerEndLevel != INVALID_ID &&
					//	 iLevel <= pQuest->nQuestPlayerEndLevel )
				{
					//if you get here the player has to many quest
					s_NPCTalkStart( pPlayer, pNPC, NPC_DIALOG_OK, DIALOG_QUEST_TO_MANY );
					eResult = QMR_NPC_TALK;
				}
				
			}
		}

	}
	if( eResult == QMR_NPC_TALK )
	{		
		ASSERTX_RETVAL( pQuestTaskUpdateWorld, eResult, "Quest task to update world after talking to NPC was null in function s_QuestTalkToNPC" );
		QuestUpdateWorld( pGame, pPlayer, UnitGetLevel( pPlayer ), pQuestTaskUpdateWorld, QUEST_WORLD_UPDATE_QSTATE_TALK_NPC, pNPC );
	}

	return eResult;
}

////////////////////////////////////////////
//This function checks and sets the states on a npc if they have a 
//quest for the player to do or not. Most of the logic via the state setting
//was copied from the function sUpdateNPCs. Which is where this function should
//be getting called from.
/////////////////////////////////////////////
BOOL s_QuestUpdateQuestNPCs( GAME *pGame,
							 UNIT *pPlayer,
							 UNIT *pNPC )
{
	ASSERTX_RETFALSE( IS_SERVER( pGame ), "Calling s_QuestUpdateQuestNPCs from client." );
	// server has to tell the clients - doesn't matter if the server sets these states,
	// as the client will never see them.
	if( IS_SERVER( pGame ) )
	{
		if( pPlayer == NULL  || pNPC == NULL )
		{
			return FALSE; //must have a player
		}
		// tell this client that npc states have changed, and they need to update
		GAMECLIENTID idClient = UnitGetClientId( pPlayer );
		MSG_SCMD_QUEST_NPC_UPDATE tMessage;	
		tMessage.idNPC = UnitGetId( pNPC );
		s_SendMessage( UnitGetGame( pPlayer ), idClient, SCMD_QUEST_NPC_UPDATE, &tMessage );
		return TRUE;
	}

	return FALSE;
}