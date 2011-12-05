//----------------------------------------------------------------------------
// FILE: quest_abyss.cpp
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_quests.h"
#include "game.h"
#include "globalindex.h"
#include "npc.h"
#include "quest_abyss.h"
#include "quests.h"
#include "quest.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "inventory.h"
#include "party.h"
#include "states.h"
#include "combat.h"
#include "room_path.h"
#include "uix.h"
#include "quest_template.h"


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

enum
{
	QUEST_ABYSS_NUM_HELLRIFTS_DESTROYED_STATES_AVAILABLE = 5,
	HELLRIFT_BOMB_FUSE_IN_SECONDS = 30			
};

#define HELLRIFT_RESET_DELAY_INITIAL_IN_TICKS	(GAME_TICKS_PER_SECOND * 60 * 2)
#define HELLRIFT_RESET_DELAY_IN_TICKS			(GAME_TICKS_PER_SECOND * 10)

//----------------------------------------------------------------------------
enum
{
	QUEST_ABYSS_MINIBOSS_INVALID = -1,

	QUEST_ABYSS_MINIBOSS_TOXIC,
	QUEST_ABYSS_MINIBOSS_SPECTRAL,
	QUEST_ABYSS_MINIBOSS_ELECTRICAL,
	QUEST_ABYSS_MINIBOSS_PHYSICAL,
	QUEST_ABYSS_MINIBOSS_FIRE,

	NUM_QUEST_ABYSS_MINI_BOSSES,

	// quest data stat indexes
	QUEST_ABYSS_HELLRIFTSDESTROYED_TOXIC = 0,
	QUEST_ABYSS_HELLRIFTSDESTROYED_SPECTRAL,
	QUEST_ABYSS_HELLRIFTSDESTROYED_ELECTRICAL,
	QUEST_ABYSS_HELLRIFTSDESTROYED_PHYSICAL,
	QUEST_ABYSS_HELLRIFTSDESTROYED_FIRE,

	QUEST_ABYSS_MINIBOSSKILLS_TOXIC, 
	QUEST_ABYSS_MINIBOSSKILLS_SPECTRAL,
	QUEST_ABYSS_MINIBOSSKILLS_ELECTRICAL,
	QUEST_ABYSS_MINIBOSSKILLS_PHYSICAL,
	QUEST_ABYSS_MINIBOSSKILLS_FIRE,
};

//----------------------------------------------------------------------------
struct QUEST_DATA_ABYSS
{
	// monster classes
	int		nMiniBoss[NUM_QUEST_ABYSS_MINI_BOSSES];

	// level definition indexes
	int		nHellriftSublevel[NUM_QUEST_ABYSS_MINI_BOSSES];
	int		nLevelAbyss;

	// object classes of hellrift portals
	int		nHellrift[NUM_QUEST_ABYSS_MINI_BOSSES];

	// object classes to give essences or heads to to open portals below
	int		nHellriftPedestal[NUM_QUEST_ABYSS_MINI_BOSSES];

	// item classes to give to the headstones
	int		nItemMiniBossFightTicket[NUM_QUEST_ABYSS_MINI_BOSSES];

	BOOL	bEnteredHellrift;	// TODO make sure this works with multiple
	int		nExplosionStatic;
	int		nExplosionMonster;
	int		nBombFuseTime;

	int		nQueuedVideoMessageElement;
	BOOL	bQueuedVideoMessageBossSpawned;
};

static int sGetBossTypeToSpawn( QUEST *pQuest );

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_ABYSS * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_ABYSS *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sCanOperateObject( 
	QUEST *pQuest,
	UNIT *pObject)
{
	//const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	const QUEST_DATA_ABYSS *pData = sQuestDataGet( pQuest );
	ROOM * room = UnitGetRoom(pObject);
	ASSERT_RETVAL(room, QMR_OK);
	SUBLEVEL *pSubLevel = RoomGetSubLevel( room );
	ASSERT_RETVAL(pSubLevel, QMR_OK);

	// figure out which mini boss this object should open the portal for
	for (int i = 0; i < NUM_QUEST_ABYSS_MINI_BOSSES; ++i)
	{
		if (pSubLevel->nSubLevelDef == pData->nHellriftSublevel[i])			
		{
			if (UnitIsObjectClass( pObject, pData->nHellriftPedestal[i] ))
			{
				return (QuestStateGetValue( pQuest, QUEST_STATE_ABYSS_USE_PEDESTAL ) == QUEST_STATE_ACTIVE) ? QMR_OPERATE_OK : QMR_OPERATE_FORBIDDEN;
			}
		}
		else if (UnitIsObjectClass( pObject, pData->nHellrift[i]))
		{
			// disable hellrift if they have the state that means it's exploding
			return (QuestStateGetValue( pQuest, QUEST_STATE_ABYSS_EXIT_HELLRIFT ) == QUEST_STATE_ACTIVE) ? QMR_OPERATE_FORBIDDEN : QMR_OPERATE_OK;
		}
	}


	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageCanOperateObject( 
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pObject = UnitGetById( pGame, pMessage->idObject );
	return sCanOperateObject( pQuest, pObject );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	if (UnitGetGenus( pSubject ) == GENUS_OBJECT )
	{
		return (!ObjectIsWarp( pSubject ) && sCanOperateObject( pQuest, pSubject ) == QMR_OPERATE_OK) ?
			QMR_NPC_INFO_NEW : QMR_OK;
	}
	
	if (s_QuestIsGiver( pQuest, pSubject ))
	{
		// check givers
		if (s_QuestCanBeActivated( pQuest ))
		{
			// there is new info available, a new quest				
			return QMR_NPC_INFO_NEW;
		}
	}
	
	if (s_QuestIsRewarder( pQuest, pSubject ))
	{
		// check rewarders
		if (QuestIsActive( pQuest ))
		{
			if (QuestStateIsActive(pQuest, QUEST_STATE_ABYSS_GET_REWARD))
			{
				// start the next phase of the quest
				return QMR_NPC_INFO_RETURN;
			}
			else
			{
				// the quest is active, the npc is waiting for you to do the quest tasks				
				return QMR_NPC_INFO_WAITING;
			}
		}
		else if (QuestIsOffering( pQuest ))
		{
			return QMR_NPC_INFO_WAITING;
		}						

	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );

	if (QuestIsInactive( pQuest ))
	{
		if (s_QuestIsGiver( pQuest, pSubject ) &&
			s_QuestCanBeActivated( pQuest ))
		{
			// Activate the quest			
			s_QuestDisplayDialog(
				pQuest,
				pSubject,
				NPC_DIALOG_OK_CANCEL,
				pQuestDefinition->nDescriptionDialog );

			return QMR_NPC_TALK;
		}
	}
	else if (QuestIsActive( pQuest ))
	{
		if (QuestStateIsActive(pQuest, QUEST_STATE_ABYSS_GET_REWARD))
		{
			if ( s_QuestIsRewarder( pQuest,  pSubject ) )
			{
				// Reward: tell the player good job, and offer rewards
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK_CANCEL,
					pQuestDefinition->nRewardDialog );

				return QMR_NPC_TALK;
			}
		}
		else
		{
			if ( s_QuestIsRewarder( pQuest,  pSubject ) )
			{
				// Incomplete: tell the player to go kill the monster
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK,
					pQuestDefinition->nIncompleteDialog );

			return QMR_NPC_TALK;
			}
		}
	}
#endif		
	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sAdvanceToBossKilledState( QUEST *pQuest )
{
	int nBossIndex = sGetBossTypeToSpawn(pQuest);
	if (nBossIndex != QUEST_ABYSS_MINIBOSS_INVALID)
	{
		UNIT *pPlayer = QuestGetPlayer(pQuest);
		s_QuestAdvanceTo(pQuest, QUEST_STATE_ABYSS_GET_REWARD);
		int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
		int nHellriftsDestroyed = UnitGetStat( pPlayer, STATS_ABYSS_QUEST_DATA, QUEST_ABYSS_HELLRIFTSDESTROYED_TOXIC + nBossIndex, nDifficulty);
		nHellriftsDestroyed = MAX(0, nHellriftsDestroyed - pQuest->pQuestDef->nObjectiveCount);
		UnitSetStat( pPlayer, STATS_ABYSS_QUEST_DATA, QUEST_ABYSS_HELLRIFTSDESTROYED_TOXIC + nBossIndex, nDifficulty, nHellriftsDestroyed);

		int nKills = UnitGetStat( pPlayer, STATS_ABYSS_QUEST_DATA, QUEST_ABYSS_MINIBOSSKILLS_TOXIC + nBossIndex, nDifficulty );
		UnitSetStat( pPlayer, STATS_ABYSS_QUEST_DATA, QUEST_ABYSS_MINIBOSSKILLS_TOXIC + nBossIndex, nDifficulty, nKills + 1 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPlayerEarnedBossFight( UNIT *pPlayer, int nBossType, int nRequiredHellrifts, int nDifficulty )
{
	// player logged out before receiving message
	return ( UnitGetStat( pPlayer, STATS_ABYSS_QUEST_DATA, QUEST_ABYSS_HELLRIFTSDESTROYED_TOXIC + nBossType, nDifficulty ) >= nRequiredHellrifts);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPlayerEarnedBossFight( UNIT *pPlayer, int nBossType, int nRequiredHellrifts )
{
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	return sPlayerEarnedBossFight( pPlayer, nBossType, nRequiredHellrifts, nDifficulty );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct  QuestAbyssGiveBossKillCreditData
{
	int nBossType;
	int nRequiredHellrifts;
	int nAccessAllowedCount;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sPlayerGiveBossKillCredit( UNIT *pPlayer, void *pUserData )
{
	QuestAbyssGiveBossKillCreditData *pData = (QuestAbyssGiveBossKillCreditData*)pUserData;
	ASSERT_RETURN(pData && pData->nBossType >= 0 && pData->nBossType < NUM_QUEST_ABYSS_MINI_BOSSES );
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	ASSERT_RETURN(nDifficulty >= 0);
	if (sPlayerEarnedBossFight(pPlayer, pData->nBossType, pData->nRequiredHellrifts, nDifficulty))
	{
		++pData->nAccessAllowedCount;
		QUEST *pQuest = QuestGetQuestByDifficulty(pPlayer, QUEST_ABYSS, nDifficulty);
		if (pQuest)
			s_sAdvanceToBossKilledState(pQuest);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetBossTypeToSpawn( QUEST *pQuest )
{
	UNIT *pPlayer = QuestGetPlayer(pQuest);
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	// player logged out before receiving message
	for (int i = 0; i < NUM_QUEST_ABYSS_MINI_BOSSES; ++i)
	{
		if ( sPlayerEarnedBossFight( pPlayer, i, pQuest->pQuestDef->nObjectiveCount, nDifficulty ) )
		{
			return i;
		}
	}
	return QUEST_ABYSS_MINIBOSS_INVALID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sSaveBossIndexToFight( QUEST *pQuest, int nBossIndex )
{
	ASSERT_RETURN(nBossIndex != QUEST_ABYSS_MINIBOSS_INVALID);
	UNIT *pPlayer = QuestGetPlayer(pQuest);
	int nDifficultyCurrent = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	QUEST_DATA_ABYSS *pData = sQuestDataGet( pQuest );
	ASSERT_RETURN(nBossIndex >= 0 && nBossIndex < arrsize(pData->nMiniBoss));
	int nCustomBoss = pData->nMiniBoss[nBossIndex];
	UnitSetStat( pPlayer, STATS_QUEST_CUSTOM_BOSS_CLASS, pQuest->nQuest, nDifficultyCurrent, nCustomBoss );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );

	if ( nDialogStopped != INVALID_LINK )
	{
		if ( nDialogStopped == pQuestDefinition->nDescriptionDialog )
		{
			if ( QuestIsInactive( pQuest ) &&
				 s_QuestIsGiver( pQuest, pTalkingTo ) &&
				 s_QuestCanBeActivated( pQuest ))
			{
				// activate quest
				if (s_QuestActivate( pQuest ))
				{

				}
			}
		}
		else if ( nDialogStopped == pQuestDefinition->nRewardDialog )
		{
			// The boss quests give any extra rewards. This quest never completes
			if (s_QuestIsRewarder( pQuest, pTalkingTo ) )
			{
				// complete once for each boss kill

				if ( s_QuestComplete( pQuest ) )
				{								
					// offer reward
					if ( s_QuestOfferReward( pQuest, pTalkingTo, FALSE ) )
					{
						return QMR_OFFERING;
					}
				}					
			}
		}
		else if ( nDialogStopped == DIALOG_PATCH_2_ABYSS_VIDEO_PROMPT_BOSS_VAGUE )
		{
			s_QuestAdvanceTo(pQuest, QUEST_STATE_ABYSS_FIGHT_BOSS);
		}
	}

	return QMR_OK;		

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkCancelled(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_CANCELLED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	if ( nDialogStopped == DIALOG_PATCH_2_ABYSS_VIDEO_PROMPT_BOSS_VAGUE )
	{
		s_QuestAdvanceTo(pQuest, QUEST_STATE_ABYSS_FIGHT_BOSS);
	}

	return QMR_OK;		

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int s_sGiveBossKillCreditToQualifyingPartyMembers( GAME *pGame, UNIT *pPlayer, int nBossType, int nRequiredHellrifts )
{
	ASSERT_RETZERO(IS_SERVER( pGame ));
	QuestAbyssGiveBossKillCreditData tData;
	tData.nBossType = nBossType;
	tData.nRequiredHellrifts = nRequiredHellrifts;
	tData.nAccessAllowedCount = 0;
	s_sPlayerGiveBossKillCredit( pPlayer, &tData );

#if !ISVERSION(CLIENT_ONLY_VERSION)
	PARTYID idParty = UnitGetPartyId( pPlayer );
	if (idParty != INVALID_ID)
		s_PartyIterateCachedPartyMembersInGame( pGame, idParty, s_sPlayerGiveBossKillCredit, &tData );

#endif //!ISVERSION(CLIENT_ONLY_VERSION)

	return tData.nAccessAllowedCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageMonsterDying( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_DYING *pMessage)
{
	QUEST_DATA_ABYSS* pData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	ASSERT_RETVAL( IS_SERVER( pGame ), QMR_OK );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	UNIT *pPlayer = QuestGetPlayer(pQuest);

	// no loot if the guy who gave access to the boss isn't there, to avoid exploits
	for (int i=0; i < NUM_QUEST_ABYSS_MINI_BOSSES; ++i)
	{
		if (UnitIsMonsterClass( pVictim, pData->nMiniBoss[i] ) &&
			pData->nLevelAbyss == UnitGetLevelDefinitionIndex( pVictim ) &&
			UnitGetStat( pVictim, STATS_LOOT_BONUS_PERCENT) <= 0)
		{
			// check quest data of people in game and turn off loot if no one has the quest to kill the dude.
			int nNumPartyMembersWithBossAccess = s_sGiveBossKillCreditToQualifyingPartyMembers( pGame, pPlayer, i, pQuest->pQuestDef->nObjectiveCount );
			if (!nNumPartyMembersWithBossAccess)
			{
				UnitClearFlag( pVictim, UNITFLAG_GIVESLOOT );
			}
			else if (nNumPartyMembersWithBossAccess > 1)
			{
				UnitSetStat( pVictim, STATS_LOOT_BONUS_PERCENT, pQuest->pQuestDef->nLootBonusPerPlayerWithQuest * (nNumPartyMembersWithBossAccess-1) );
			}
		}
	}
	
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessagePartyKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_PARTY_KILL *pMessage)
{
	if (!QuestIsActive(pQuest))
		return QMR_OK;

	QUEST_DATA_ABYSS* pData = sQuestDataGet( pQuest );
	ROOM * room = UnitGetRoom(pMessage->pVictim);
	ASSERT_RETVAL(room, QMR_OK);
	SUBLEVEL *pSubLevel = RoomGetSubLevel( room );
	ASSERT_RETVAL(pSubLevel, QMR_OK);
	LEVEL *pLevel = RoomGetLevel( room );
	ASSERT_RETVAL(pLevel, QMR_OK);

	// figure out which mini boss this object should open the portal for
	for (int i = 0; i < NUM_QUEST_ABYSS_MINI_BOSSES; ++i)
	{
		if (pSubLevel->nSubLevelDef == pData->nHellriftSublevel[i])
		{
			// if the player killed all the monsters in the Hellrift sublevel, advance their quest state so that they can operate the pedestal and destroy the hellrift
			if (!s_SubLevelHasALiveMonster(pSubLevel))
			{
				// disable the shield on the pedestal
				TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
				UNIT *pPedestal = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pData->nHellriftPedestal[i] );
				s_StateClear( pPedestal, UnitGetId( pPedestal ), STATE_PEDESTAL_SHIELD, 0 );

				QuestStateSet( pQuest, QUEST_STATE_ABYSS_CLEAR_HELLRIFT, QUEST_STATE_COMPLETE ); 

				QuestStateSet( pQuest, QUEST_STATE_ABYSS_USE_PEDESTAL, QUEST_STATE_HIDDEN);
				QuestStateSet( pQuest, QUEST_STATE_ABYSS_USE_PEDESTAL, QUEST_STATE_ACTIVE);
			}
			break;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sStartQueuedVideoMessage(QUEST *pQuest, UNIT *pPlayer, QUEST_DATA_ABYSS *pQuestData)
{
	if (pQuestData->nQueuedVideoMessageElement == QUEST_ABYSS_MINIBOSS_INVALID &&
		QuestStateIsActive(pQuest, QUEST_STATE_ABYSS_FIGHT_BOSS))
	{
		pQuestData->bQueuedVideoMessageBossSpawned = TRUE;
		// TODO determine the element of the last rift destroyed, or just play the vauge message in this case, if I decide to use the precise messages
	}

	s_NPCVideoStart( 
		UnitGetGame( pPlayer ), 
		pPlayer, 
		GI_MONSTER_TITUS, 
		NPC_VIDEO_INSTANT_INCOMING, 
		pQuestData->bQueuedVideoMessageBossSpawned ? 
			DIALOG_PATCH_2_ABYSS_VIDEO_PROMPT_BOSS_VAGUE : 
			DIALOG_PATCH_2_ABYSS_VIDEO_PROMPT_HELLRIFT_VAGUE );

	pQuestData->nQueuedVideoMessageElement = QUEST_ABYSS_MINIBOSS_INVALID;
	pQuestData->bQueuedVideoMessageBossSpawned = FALSE;
	QuestStateSet( pQuest, QUEST_STATE_ABYSS_INCOMING_VIDEO, QUEST_STATE_HIDDEN ); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageEnterLevel( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL *pMessageEnterLevel )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	if (!QuestIsActive(pQuest))
		return QMR_OK;

	QUEST_DATA_ABYSS *pQuestData = sQuestDataGet( pQuest );

	if (QuestStateIsActive(pQuest, QUEST_STATE_ABYSS_INCOMING_VIDEO))
	{
		s_sStartQueuedVideoMessage( pQuest, QuestGetPlayer( pQuest ), pQuestData );
	}

	if (!QuestStateIsActive(pQuest, QUEST_STATE_ABYSS_INCOMING_VIDEO) &&
		!QuestStateIsActive( pQuest, QUEST_STATE_ABYSS_FIGHT_BOSS ) &&
		!QuestStateIsActive( pQuest, QUEST_STATE_ABYSS_GET_REWARD ))
	{
		QuestStateSet( pQuest, QUEST_STATE_ABYSS_FIND_HELLRIFT, QUEST_STATE_HIDDEN ); 
		QuestStateSet( pQuest, QUEST_STATE_ABYSS_FIND_HELLRIFT, QUEST_STATE_ACTIVE ); 
		QuestStateSet( pQuest, QUEST_STATE_ABYSS_CLEAR_HELLRIFT, QUEST_STATE_HIDDEN ); 
		QuestStateSet( pQuest, QUEST_STATE_ABYSS_USE_PEDESTAL, QUEST_STATE_HIDDEN ); 
		QuestStateSet( pQuest, QUEST_STATE_ABYSS_EXIT_HELLRIFT, QUEST_STATE_HIDDEN ); 
	}

	// spawn the boss if the player has the quest state to kill him
	if (pQuestData->nLevelAbyss == pMessageEnterLevel->nLevelNewDef)
	{
		UNIT *pPlayer = QuestGetPlayer( pQuest );
		LEVEL *pLevel = UnitGetLevel( pPlayer );

		// first check if any of the bosses are already in the level, if so, abort
		for (int i = 0; i < NUM_QUEST_ABYSS_MINI_BOSSES; ++i)
		{
			TARGET_TYPE eBadTargetTypes[] = { TARGET_BAD, TARGET_DEAD, TARGET_INVALID };
			if (LevelFindFirstUnitOf( pLevel, eBadTargetTypes, GENUS_MONSTER, pQuestData->nMiniBoss[i] ))
				return QMR_OK;
		}

		// no bosses in level yet, spawn the first available
		int i = sGetBossTypeToSpawn( pQuest );
		if ( i != QUEST_ABYSS_MINIBOSS_INVALID )
		{
			ROOM * pSpawnRoom;

			// TODO use a spawn point

			// spawn nearby
			ROOM * pRoom = UnitGetRoom( pPlayer );
			ROOM_LIST tNearbyRooms;
			RoomGetNearbyRoomList(pRoom, &tNearbyRooms);
			if ( tNearbyRooms.nNumRooms)
			{
				int index = RandGetNum( pGame->rand, tNearbyRooms.nNumRooms - 1 );
				pSpawnRoom = tNearbyRooms.pRooms[index];
			}
			else
			{
				pSpawnRoom = pRoom;
			}

			// and spawn boss
			int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );
			// TODO make sure the boss spawns
			ASSERT_RETVAL(nNodeIndex >= 0, QMR_OK);

			// init location
			SPAWN_LOCATION tLocation;
			SpawnLocationInit( &tLocation, pSpawnRoom, nNodeIndex );

			MONSTER_SPEC tSpec;
			tSpec.nClass = pQuestData->nMiniBoss[i];
			tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 32 );
			tSpec.pRoom = pSpawnRoom;
			tSpec.vPosition = tLocation.vSpawnPosition;
			tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
			tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
			s_MonsterSpawn( pGame, tSpec );

			// just spawn one at a time
		}
	}

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageSubLevelTransition( 
	QUEST *pQuest,
	const QUEST_MESSAGE_SUBLEVEL_TRANSITION *pMessage)
{
	if (!QuestIsActive(pQuest))
		return QMR_OK;

	QUEST_DATA_ABYSS *pQuestData = sQuestDataGet( pQuest );
	SUBLEVEL *pSubLevelNew = pMessage->pSubLevelNew;

	if (QuestStateIsActive( pQuest, QUEST_STATE_ABYSS_FIND_HELLRIFT))
	{
		for (int i = 0; i < NUM_QUEST_ABYSS_MINI_BOSSES; ++i)
		{
			if (pSubLevelNew->nSubLevelDef == pQuestData->nHellriftSublevel[i])
			{
				// save that we've entered the hellrift
				pQuestData->bEnteredHellrift = TRUE;
				QuestStateSet( pQuest, QUEST_STATE_ABYSS_FIND_HELLRIFT, QUEST_STATE_COMPLETE ); 
				QuestStateSet( pQuest, QUEST_STATE_ABYSS_CLEAR_HELLRIFT, QUEST_STATE_HIDDEN ); 
				QuestStateSet( pQuest, QUEST_STATE_ABYSS_CLEAR_HELLRIFT, QUEST_STATE_ACTIVE ); 

			}
		}

		for (int i = 0; i < NUM_QUEST_ABYSS_MINI_BOSSES; ++i)
		{
			if (pSubLevelNew->nSubLevelDef == pQuestData->nHellriftSublevel[i])
			{
				// if the player killed all the monsters in the Hellrift sublevel, advance their quest state so that they can operate the pedestal and destroy the hellrift
				if (!s_SubLevelHasALiveMonster(pSubLevelNew))
				{
					s_QuestAdvanceTo(pQuest, QUEST_STATE_ABYSS_USE_PEDESTAL);
				}
				break;
			}
		}
	}

	for (int i = 0; i < NUM_QUEST_ABYSS_MINI_BOSSES; ++i)
	{
		// exiting hellrift
		SUBLEVEL *pSubLevelOld = pMessage->pSubLevelOld;
		if (pSubLevelOld && pSubLevelOld->nSubLevelDef == pQuestData->nHellriftSublevel[i])
		{
			if (QuestStateIsActive( pQuest, QUEST_STATE_ABYSS_EXIT_HELLRIFT ))
			{
				QuestStateSet( pQuest, QUEST_STATE_ABYSS_EXIT_HELLRIFT, QUEST_STATE_COMPLETE ); 
			}
			else if (QuestStateIsHidden( pQuest, QUEST_STATE_ABYSS_EXIT_HELLRIFT ))
			{
				QuestStateSet( pQuest, QUEST_STATE_ABYSS_FIND_HELLRIFT, QUEST_STATE_HIDDEN ); 
				QuestStateSet( pQuest, QUEST_STATE_ABYSS_FIND_HELLRIFT, QUEST_STATE_ACTIVE ); 
				QuestStateSet( pQuest, QUEST_STATE_ABYSS_CLEAR_HELLRIFT, QUEST_STATE_HIDDEN ); 
				QuestStateSet( pQuest, QUEST_STATE_ABYSS_USE_PEDESTAL, QUEST_STATE_HIDDEN ); 
			}
		}
	}
	
	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sExplosionDeath( UNIT * pMonster, void * pCallbackData )
{
	UnitDie( pMonster, NULL );
	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRoomIsInHellrift(
	ROOM *pRoom,
	int nLevelDefHellrift)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	LEVEL *pLevel = RoomGetLevel( pRoom );
	ASSERTX_RETFALSE( pLevel, "Expected level" );

	if (LevelGetDefinitionIndex( pLevel ) == nLevelDefHellrift)
	{	
		const SUBLEVEL *pSubLevel = RoomGetSubLevel( pRoom );
		const SUBLEVEL_DEFINITION *pSubLevelDef = SubLevelGetDefinition( pSubLevel );
		if (pSubLevelDef->eType == ST_HELLRIFT)
		{
			return TRUE;
		}

	}

	return FALSE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define NUM_STATIC_EXPLOSIONS			8
static void sSpawnHellriftExplode( 
	QUEST * pQuest,
	SUBLEVEL *pExplodingSublevel)
{
	ASSERT_RETURN( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	QUEST_DATA_ABYSS * pQuestData = sQuestDataGet( pQuest );

	// already spawned?
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	if ( LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nExplosionStatic ) )
	{
		return;
	}

	GAME * pGame = UnitGetGame( pPlayer );
	for ( ROOM * pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ) )
	{
		// is this a room in the hellrift sublevel?
		if (RoomGetSubLevel( pRoom ) == pExplodingSublevel)
		{

			// first kill all the monsters
			RoomIterateUnits( pRoom, eTargetTypes, sExplosionDeath, NULL );

			// spawn some explosions!
			for ( int i = 0; i < NUM_STATIC_EXPLOSIONS; i++ )
			{
				int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pRoom, 0 );
				if (nNodeIndex < 0)
					return;

				// init location
				SPAWN_LOCATION tLocation;
				SpawnLocationInit( &tLocation, pRoom, nNodeIndex );

				MONSTER_SPEC tSpec;
				tSpec.nClass = pQuestData->nExplosionStatic;
				tSpec.nExperienceLevel = 1;
				tSpec.pRoom = pRoom;
				tSpec.vPosition = tLocation.vSpawnPosition;
				tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
				tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
				s_MonsterSpawn( pGame, tSpec );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sBombExploded(
	UNIT *pPlayer,
	void *pCallbackData)
{	
	QUEST *pQuest = QuestGetQuest( pPlayer, QUEST_ABYSS );
	QUEST_DATA_ABYSS *pQuestData = sQuestDataGet( pQuest );
	SUBLEVEL *pExplodingSublevel = (SUBLEVEL *)pCallbackData;
	ASSERT_RETURN(pExplodingSublevel);

	// complete quest if it's active and we've been into the hellrift
	if (pQuest && QuestIsActive( pQuest ) && pQuestData->bEnteredHellrift == TRUE)
	{
		int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
		UNIT *pPlayer = QuestGetPlayer(pQuest);
		for (int i = 0; i < NUM_QUEST_ABYSS_MINI_BOSSES; ++i)
		{
			if ( pQuestData->nHellriftSublevel[i] == pExplodingSublevel->nSubLevelDef )
			{
				int nNumHellriftsDestroyed = UnitGetStat( pPlayer, STATS_ABYSS_QUEST_DATA, QUEST_ABYSS_HELLRIFTSDESTROYED_TOXIC + i, nDifficulty );

				ASSERTX_RETURN(nNumHellriftsDestroyed >= 0,"Hellrifts destroyed index out of bounds");
				++nNumHellriftsDestroyed;
				UnitSetStat( pPlayer, STATS_ABYSS_QUEST_DATA, QUEST_ABYSS_HELLRIFTSDESTROYED_TOXIC + i, nDifficulty, nNumHellriftsDestroyed );

				s_sSaveBossIndexToFight( pQuest, i );

				pQuestData->nQueuedVideoMessageElement = i;
				pQuestData->bQueuedVideoMessageBossSpawned = (nNumHellriftsDestroyed >= pQuest->pQuestDef->nObjectiveCount);
				QuestStateSet( pQuest, QUEST_STATE_ABYSS_INCOMING_VIDEO, QUEST_STATE_ACTIVE ); 
			}
		}
	}

	// check if they are in the hellrift that is exploding when the bomb goes off...
	if (UnitGetSubLevel( pPlayer) == pExplodingSublevel)
	{
		// kill them...
		UnitDie( pPlayer, NULL );		
	}
	else if (QuestStateIsActive(pQuest, QUEST_STATE_ABYSS_INCOMING_VIDEO))
	{
		s_sStartQueuedVideoMessage(pQuest, pPlayer, pQuestData);
	}

	// kill stuff in the hellrift
	sSpawnHellriftExplode( pQuest, pExplodingSublevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAlivePlayersInHellrift( 
	GAME *pGame, 
	LEVEL *pLevel)
{

	// go through players
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame ); pClient; pClient = ClientGetNextInGame( pClient ))
	{
		UNIT *pPlayer = ClientGetControlUnit( pClient );
		ROOM *pRoom = UnitGetRoom( pPlayer );
		if (RoomIsHellrift( pRoom ))
		{
			if (IsUnitDeadOrDying( pPlayer ) == FALSE)
			{
				return TRUE;  // a player is alive in the hellrift
			}
		}
	}
	return FALSE;  // no players are alive in the hellrift for this level
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sResetSubLevel(
	GAME *pGame,
	UNIT *pUnit,
	const EVENT_DATA &tEventData)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	LEVELID idLevel = (LEVELID)tEventData.m_Data1;
	LEVEL *pLevel = LevelGetByID( pGame, idLevel );
	SUBLEVELID idSubLevel = (SUBLEVELID)tEventData.m_Data2;

	// check to see if all players in the hellrift rooms are dead
	BOOL bAlivePlayersInHellrift = sAlivePlayersInHellrift( pGame, pLevel );
	if (bAlivePlayersInHellrift == TRUE)
	{

		// schedule event in the future to check again
		GameEventRegister( 
			pGame, 
			sResetSubLevel, 
			NULL, 
			NULL, 
			&tEventData, 
			HELLRIFT_RESET_DELAY_IN_TICKS);

	}
	else
	{

		// this hellrift level will allow resurrection again
		SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
		ASSERTX_RETTRUE( pSubLevel, "Expected sublevel" );
		SubLevelSetStatus( pSubLevel, SLS_RESURRECT_RESTRICTED_BIT, FALSE );

		// go reset sublevel rooms
		for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
		{

			if (RoomGetSubLevelID( pRoom ) == idSubLevel)
			{

				// clear cannot reset flag
				RoomSetFlag( pRoom, ROOM_CANNOT_RESET_BIT, FALSE );

				// reset
				s_RoomReset( pGame, pRoom );

			}

		}

	}

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFreezeAndResetHellrift( 
	GAME *pGame,
	LEVEL *pLevel,
	SUBLEVELID idSubLevelHellrift)
{	

	// freeze all rooms in the hellrift
	for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
		if (RoomGetSubLevelID( pRoom ) == idSubLevelHellrift)
		{
			RoomSetFlag( pRoom, ROOM_CANNOT_RESET_BIT );
		}

	}

	// this hellrift will be able to be playable again in a couple of minutes, that should
	// be enough time for everybody left in the room to die and restart
	EVENT_DATA tEventData;
	tEventData.m_Data1 = LevelGetID( pLevel );
	tEventData.m_Data2 = idSubLevelHellrift;
	GameEventRegister( 
		pGame, 
		sResetSubLevel, 
		NULL, 
		NULL, 
		&tEventData, 
		HELLRIFT_RESET_DELAY_INITIAL_IN_TICKS);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sBombTimer( 
	GAME *pGame, 
	UNIT *pUnit, 
	const struct EVENT_DATA &tEventData )
{
	ASSERT_RETFALSE( pGame && pUnit );
	ASSERT( IS_SERVER( pGame ) );

	// this quest is complete for everybody who entered the hellrift at all
	PlayersIterate( pGame, s_sBombExploded, UnitGetSubLevel( pUnit ) );

	// the hellrift sublevel is the sublevel with the relic device that is blowing up
	SUBLEVEL *pSubLevelHellrift = UnitGetSubLevel( pUnit );
	SUBLEVELID idSubLevelHellrift = SubLevelGetId( pSubLevelHellrift );

	// close the and exit portals to the hellrift sub level 
	// note that we will recreate them both later when the hellrift resets
	LEVEL *pLevel = UnitGetLevel( pUnit );
	const SUBLEVEL_DOORWAY *pSubLevelEntrance = s_SubLevelGetDoorway( pSubLevelHellrift, SD_ENTRANCE );
	ASSERTX_RETFALSE( pSubLevelEntrance, "Expected sublevel entrance" );
	UNIT *pEntrance = UnitGetById( pGame, pSubLevelEntrance->idDoorway );
	if (pEntrance)
	{
		s_StateSet( pEntrance, pEntrance, STATE_QUEST_HIDDEN, 0 );
	}

	const SUBLEVEL_DOORWAY *pSubLevelExit = s_SubLevelGetDoorway( pSubLevelHellrift, SD_EXIT );
	ASSERTX_RETFALSE( pSubLevelExit, "Expected sublevel exit" );
	UNIT *pExit = UnitGetById( pGame, pSubLevelExit->idDoorway );
	if (pExit)
	{
		s_StateSet( pExit, pExit, STATE_QUEST_HIDDEN, 0 );
	}

	// freeze all sublevel rooms and start timer to respawn after a little bit in
	// multiplayer only
	if (AppIsMultiplayer())
	{
		sFreezeAndResetHellrift( pGame, pLevel, idSubLevelHellrift );
	}

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sHellriftUpdateFuseDisplay( QUEST * pQuest )
{
#if !ISVERSION(SERVER_VERSION)

	QUEST_DATA_ABYSS * data = sQuestDataGet( pQuest );
	const int MAX_STRING = 1024;
	WCHAR timestr[ MAX_STRING ] = L"\0";


	if ( data->nBombFuseTime > 1 )
	{
		const WCHAR *puszFuseString = GlobalStringGet( GS_HELLRIFT_FUSE_MULTI_SECOND );

		PStrPrintf( 
			timestr, 
			MAX_STRING, 
			puszFuseString,
			//L"%c%cEscape! Exit portal will collapse in: %i seconds", 
			data->nBombFuseTime );
	}
	else
	{
		PStrCopy(timestr, GlobalStringGet( GS_HELLRIFT_FUSE_SINGLE_SECOND ), MAX_STRING);
	}

	// show on screen
	UIShowQuickMessage( timestr );
#endif //!ISVERSION(SERVER_VERSION)


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL c_sFuseUpdate( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );
	ASSERT( IS_CLIENT( game ) );

	QUEST * pQuest = ( QUEST * )tEventData.m_Data1;
	QUEST_DATA_ABYSS * data = sQuestDataGet( pQuest );
	data->nBombFuseTime--;
	if ( data->nBombFuseTime )
	{
		c_sHellriftUpdateFuseDisplay( pQuest );
		UnitRegisterEventTimed( unit, c_sFuseUpdate, &tEventData, GAME_TICKS_PER_SECOND );
	}
	else
	{
#if !ISVERSION(SERVER_VERSION)
		UIHideQuickMessage();
#endif// !ISVERSION(SERVER_VERSION)
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT c_sClientQuestMesssageStateChanged( 
	QUEST *pQuest,
	const QUEST_MESSAGE_STATE_CHANGED *pMessage)
{
	if (!QuestIsActive(pQuest))
		return QMR_OK;

	int nQuestState = pMessage->nQuestState;
	QUEST_STATE_VALUE eValue = 	pMessage->eValueNew;
	QUEST_DATA_ABYSS * pQuestData = sQuestDataGet( pQuest );

	if ( ( nQuestState == QUEST_STATE_ABYSS_EXIT_HELLRIFT) &&
		 ( eValue == QUEST_STATE_ACTIVE ) )
	{
		// set display timer
		pQuestData->nBombFuseTime = HELLRIFT_BOMB_FUSE_IN_SECONDS;

		c_sHellriftUpdateFuseDisplay( pQuest );
		UnitRegisterEventTimed( 
			pQuest->pPlayer, 
			c_sFuseUpdate, 
			&EVENT_DATA( (DWORD_PTR)pQuest ), 
			GAME_TICKS_PER_SECOND);

	}
	
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define NUM_EXPLOSIONS			8

static void sSpawnExplosions( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	QUEST_DATA_ABYSS * pQuestData = sQuestDataGet( pQuest );
	ROOM * pRoom = UnitGetRoom( pQuest->pPlayer );
	GAME * pGame = RoomGetGame( pRoom );
	for ( int i = 0; i < NUM_EXPLOSIONS; i++ )
	{
		// spawn nearby
		ROOM * pSpawnRoom = NULL;	
		ROOM_LIST tNearbyRooms;
		RoomGetNearbyRoomList(pRoom, &tNearbyRooms);
		if ( tNearbyRooms.nNumRooms)
		{
			int index = RandGetNum( pGame->rand, tNearbyRooms.nNumRooms - 1 );
			pSpawnRoom = tNearbyRooms.pRooms[index];
		}
		else
		{
			pSpawnRoom = pRoom;
		}

		// and spawn the explosions
		int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );
		if (nNodeIndex < 0)
			return;

		// init location
		SPAWN_LOCATION tLocation;
		SpawnLocationInit( &tLocation, pSpawnRoom, nNodeIndex );

		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nExplosionMonster;
		tSpec.nExperienceLevel = 1;
		tSpec.pRoom = pSpawnRoom;
		tSpec.vPosition = tLocation.vSpawnPosition;
		tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		s_MonsterSpawn( pGame, tSpec );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestObjectOperated(
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage )
{
	ASSERT_RETVAL( IS_SERVER( QuestGetGame( pQuest ) ), QMR_OK );
	UNIT *pObject = pMessage->pTarget;

	if (!QuestIsActive(pQuest))
		return QMR_OK;

	if (!sCanOperateObject(pQuest, pObject))
		return QMR_OK;

	const QUEST_DATA_ABYSS *pData = sQuestDataGet( pQuest );
	
	ROOM * room = UnitGetRoom(pObject);
	ASSERT_RETVAL(room, QMR_OK);
	SUBLEVEL *pSubLevel = RoomGetSubLevel( room );
	ASSERT_RETVAL(pSubLevel, QMR_OK);

	// figure out which mini boss this object should set the state for
	for (int i = 0; i < NUM_QUEST_ABYSS_MINI_BOSSES; ++i)
	{
		if (pSubLevel->nSubLevelDef == pData->nHellriftSublevel[i] &&
			UnitIsObjectClass( pObject, pData->nHellriftPedestal[i] ))
		{
			s_QuestStateSetForAll( pQuest, QUEST_STATE_ABYSS_USE_PEDESTAL, QUEST_STATE_COMPLETE);

			s_StateSet( pObject, pObject, STATE_OPERATE_OBJECT_FX, 0 );

			// this hellrift sublevel no longer allows resurrects (because people will get
			// stuck in them after the entrance goes away)
			SubLevelSetStatus( pSubLevel, SLS_RESURRECT_RESTRICTED_BIT, TRUE );

			// the bomb has been activated, set this state for all who have the quest active
			s_QuestStateSetForAll( pQuest, QUEST_STATE_ABYSS_EXIT_HELLRIFT, QUEST_STATE_HIDDEN );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_ABYSS_EXIT_HELLRIFT, QUEST_STATE_ACTIVE );

			// set timer for blow-up
			UnitRegisterEventTimed( pObject, s_sBombTimer, NULL, HELLRIFT_BOMB_FUSE_IN_SECONDS * GAME_TICKS_PER_SECOND );
			 
			// spawn explosion monsters
			sSpawnExplosions( pQuest );

			break;
		}
	}
	
	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessagePlayerRespawn( QUEST *pQuest )
{
	QUEST_DATA_ABYSS *pData = sQuestDataGet( pQuest );
	if (QuestStateIsActive(pQuest, QUEST_STATE_ABYSS_INCOMING_VIDEO))
	{
		s_sStartQueuedVideoMessage( pQuest, QuestGetPlayer( pQuest ), pData );
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageHandler(
	QUEST *pQuest,
	QUEST_MESSAGE_TYPE eMessageType,
	const void *pMessage)
{

	switch (eMessageType)
	{
	//----------------------------------------------------------------------------
	case QM_SERVER_INTERACT_INFO:
		{
			const QUEST_MESSAGE_INTERACT_INFO *pHasInfoMessage = (const QUEST_MESSAGE_INTERACT_INFO *)pMessage;
			return s_sQuestMessageUnitHasInfo( pQuest, pHasInfoMessage );
		}

	//----------------------------------------------------------------------------
	case QM_SERVER_INTERACT:
		{
			const QUEST_MESSAGE_INTERACT *pInteractMessage = (const QUEST_MESSAGE_INTERACT *)pMessage;
			return sQuestMessageInteract( pQuest, pInteractMessage );
		}

	//----------------------------------------------------------------------------
	case QM_SERVER_TALK_STOPPED:
		{
			const QUEST_MESSAGE_TALK_STOPPED *pTalkStoppedMessage = (const QUEST_MESSAGE_TALK_STOPPED *)pMessage;
			return sQuestMessageTalkStopped( pQuest, pTalkStoppedMessage );
		}

	//----------------------------------------------------------------------------
	case QM_SERVER_TALK_CANCELLED:
		{
			const QUEST_MESSAGE_TALK_CANCELLED *pTalkCancelledMessage = (const QUEST_MESSAGE_TALK_CANCELLED *)pMessage;
			return sQuestMessageTalkCancelled( pQuest, pTalkCancelledMessage );
		}

	//----------------------------------------------------------------------------
	case QM_CAN_OPERATE_OBJECT:
		{
			const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pCanObjectiveObjectMessage = (const QUEST_MESSAGE_CAN_OPERATE_OBJECT *)pMessage;
			return sQuestMessageCanOperateObject( pQuest, pCanObjectiveObjectMessage );
		}

	//----------------------------------------------------------------------------
	case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return s_sQuestObjectOperated( pQuest, pObjectOperatedMessage );
		}

	//----------------------------------------------------------------------------
	case QM_SERVER_MONSTER_DYING:
		{
			const QUEST_MESSAGE_MONSTER_DYING *pMonsterDyingMessage = (const QUEST_MESSAGE_MONSTER_DYING *)pMessage;
			return s_sQuestMessageMonsterDying( pQuest, pMonsterDyingMessage );
		}

	//----------------------------------------------------------------------------
	case QM_SERVER_PARTY_KILL:
		{
			const QUEST_MESSAGE_PARTY_KILL *pPartyKillMessage = (const QUEST_MESSAGE_PARTY_KILL *)pMessage;
			return sQuestMessagePartyKill( pQuest, pPartyKillMessage );
		}

	//----------------------------------------------------------------------------
	case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pMessageEnterLevel = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return s_sQuestMessageEnterLevel( 
				UnitGetGame( QuestGetPlayer( pQuest ) ), 
				pQuest, 
				pMessageEnterLevel );
		}

	//----------------------------------------------------------------------------
	case QM_SERVER_SUB_LEVEL_TRANSITION:
		{
			const QUEST_MESSAGE_SUBLEVEL_TRANSITION *pSubLevelMessage = (const QUEST_MESSAGE_SUBLEVEL_TRANSITION *)pMessage;
			return sQuestMessageSubLevelTransition( pQuest, pSubLevelMessage );
		}

	//----------------------------------------------------------------------------
	case QM_SERVER_PLAYER_RESPAWN:
		{
			return s_sQuestMessagePlayerRespawn( pQuest );
		}

	//----------------------------------------------------------------------------
	case QM_CLIENT_QUEST_STATE_CHANGED:
		{
			const QUEST_MESSAGE_STATE_CHANGED *pStateMessage = (const QUEST_MESSAGE_STATE_CHANGED *)pMessage;
			return c_sClientQuestMesssageStateChanged( pQuest, pStateMessage );
		}
		
	//----------------------------------------------------------------------------
	case QM_SERVER_QUEST_STATUS:
	case QM_CLIENT_QUEST_STATUS:
		{
			const QUEST_MESSAGE_STATUS * pMessageStatus = ( const QUEST_MESSAGE_STATUS * )pMessage;
			QuestTemplateUpdatePlayerFlagPartyKill( pQuest, QuestGetPlayer( pQuest ), pMessageStatus->eStatusOld );
			return QMR_OK;
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeAbyss(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;

	// free quest data
	ASSERTX_RETURN( pQuest->pQuestData, "Expected quest data" );
	GFREE( UnitGetGame( pQuest->pPlayer ), pQuest->pQuestData );
	pQuest->pQuestData = NULL;	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataInit(
	QUEST *pQuest,
	QUEST_DATA_ABYSS * pQuestData)
{
	// monster classes
	pQuestData->nMiniBoss[QUEST_ABYSS_MINIBOSS_TOXIC] = QuestUseGI( pQuest, GI_MONSTER_ABYSS_MINI_BOSS_TOXIC );
	pQuestData->nMiniBoss[QUEST_ABYSS_MINIBOSS_SPECTRAL] = QuestUseGI( pQuest, GI_MONSTER_ABYSS_MINI_BOSS_SPECTRAL );
	pQuestData->nMiniBoss[QUEST_ABYSS_MINIBOSS_ELECTRICAL] = QuestUseGI( pQuest, GI_MONSTER_ABYSS_MINI_BOSS_ELECTRICAL );
	pQuestData->nMiniBoss[QUEST_ABYSS_MINIBOSS_PHYSICAL] = QuestUseGI( pQuest, GI_MONSTER_ABYSS_MINI_BOSS_PHYSICAL );
	pQuestData->nMiniBoss[QUEST_ABYSS_MINIBOSS_FIRE] = QuestUseGI( pQuest, GI_MONSTER_ABYSS_MINI_BOSS_FIRE );

	// level definition indexes
	pQuestData->nHellriftSublevel[QUEST_ABYSS_MINIBOSS_TOXIC] = QuestUseGI( pQuest, GI_SUBLEVEL_ABYSS_HELLRIFT_TOXIC );
	pQuestData->nHellriftSublevel[QUEST_ABYSS_MINIBOSS_SPECTRAL] = QuestUseGI( pQuest, GI_SUBLEVEL_ABYSS_HELLRIFT_SPECTRAL );
	pQuestData->nHellriftSublevel[QUEST_ABYSS_MINIBOSS_ELECTRICAL] = QuestUseGI( pQuest, GI_SUBLEVEL_ABYSS_HELLRIFT_ELECTRICAL );
	pQuestData->nHellriftSublevel[QUEST_ABYSS_MINIBOSS_PHYSICAL] = QuestUseGI( pQuest, GI_SUBLEVEL_ABYSS_HELLRIFT_PHYSICAL );
	pQuestData->nHellriftSublevel[QUEST_ABYSS_MINIBOSS_FIRE] = QuestUseGI( pQuest, GI_SUBLEVEL_ABYSS_HELLRIFT_FIRE );

	pQuestData->nHellrift[QUEST_ABYSS_MINIBOSS_TOXIC] = QuestUseGI( pQuest, GI_OBJECT_ABYSS_HELLRIFT_TOXIC );
	pQuestData->nHellrift[QUEST_ABYSS_MINIBOSS_SPECTRAL] = QuestUseGI( pQuest, GI_OBJECT_ABYSS_HELLRIFT_SPECTRAL );
	pQuestData->nHellrift[QUEST_ABYSS_MINIBOSS_ELECTRICAL] = QuestUseGI( pQuest, GI_OBJECT_ABYSS_HELLRIFT_ELECTRICAL );
	pQuestData->nHellrift[QUEST_ABYSS_MINIBOSS_PHYSICAL] = QuestUseGI( pQuest, GI_OBJECT_ABYSS_HELLRIFT_PHYSICAL );
	pQuestData->nHellrift[QUEST_ABYSS_MINIBOSS_FIRE] = QuestUseGI( pQuest, GI_OBJECT_ABYSS_HELLRIFT_FIRE );

	// object classes to give essences or heads to to open portals below
	pQuestData->nHellriftPedestal[QUEST_ABYSS_MINIBOSS_TOXIC] = QuestUseGI( pQuest, GI_OBJECT_ABYSS_HELLRIFT_PEDESTAL_TOXIC );
	pQuestData->nHellriftPedestal[QUEST_ABYSS_MINIBOSS_SPECTRAL] = QuestUseGI( pQuest, GI_OBJECT_ABYSS_HELLRIFT_PEDESTAL_SPECTRAL );
	pQuestData->nHellriftPedestal[QUEST_ABYSS_MINIBOSS_ELECTRICAL] = QuestUseGI( pQuest, GI_OBJECT_ABYSS_HELLRIFT_PEDESTAL_ELECTRICAL );
	pQuestData->nHellriftPedestal[QUEST_ABYSS_MINIBOSS_PHYSICAL] = QuestUseGI( pQuest, GI_OBJECT_ABYSS_HELLRIFT_PEDESTAL_PHYSICAL );
	pQuestData->nHellriftPedestal[QUEST_ABYSS_MINIBOSS_FIRE] = QuestUseGI( pQuest, GI_OBJECT_ABYSS_HELLRIFT_PEDESTAL_FIRE );

	// item classes to give to the headstones
	pQuestData->nItemMiniBossFightTicket[QUEST_ABYSS_MINIBOSS_TOXIC] = QuestUseGI( pQuest, GI_ITEM_ABYSS_HELLRIFT_REWARD_TOXIC );
	pQuestData->nItemMiniBossFightTicket[QUEST_ABYSS_MINIBOSS_SPECTRAL] = QuestUseGI( pQuest, GI_ITEM_ABYSS_HELLRIFT_REWARD_SPECTRAL );
	pQuestData->nItemMiniBossFightTicket[QUEST_ABYSS_MINIBOSS_ELECTRICAL] = QuestUseGI( pQuest, GI_ITEM_ABYSS_HELLRIFT_REWARD_ELECTRICAL );
	pQuestData->nItemMiniBossFightTicket[QUEST_ABYSS_MINIBOSS_PHYSICAL] = QuestUseGI( pQuest, GI_ITEM_ABYSS_HELLRIFT_REWARD_PHYSICAL );
	pQuestData->nItemMiniBossFightTicket[QUEST_ABYSS_MINIBOSS_FIRE] = QuestUseGI( pQuest, GI_ITEM_ABYSS_HELLRIFT_REWARD_FIRE );

	pQuestData->nLevelAbyss = QuestUseGI( pQuest, GI_LEVEL_ABYSS_PSQUARE );

	pQuestData->nExplosionStatic	= QuestUseGI( pQuest, GI_MONSTER_EXPLOSION_STATIC );
	pQuestData->nExplosionMonster	= QuestUseGI( pQuest, GI_MONSTER_EXPLOSION );

	pQuestData->nBombFuseTime = HELLRIFT_BOMB_FUSE_IN_SECONDS;
	pQuestData->bEnteredHellrift = FALSE;

	pQuestData->nQueuedVideoMessageElement = QUEST_ABYSS_MINIBOSS_INVALID;
	pQuestData->bQueuedVideoMessageBossSpawned = FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitAbyss(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	

	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_ABYSS * pQuestData = ( QUEST_DATA_ABYSS * )GMALLOCZ( pGame, sizeof( QUEST_DATA_ABYSS ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// quest message flags
	//pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS );

	// this quest requires monster drops, so party kill message on
	SETBIT( pQuest->dwQuestFlags, QF_PARTY_KILL );

	for (int i=0; i < NUM_QUEST_ABYSS_MINI_BOSSES; ++i)
		s_QuestAddCastMember( pQuest, GENUS_OBJECT, pQuestData->nHellriftPedestal[i] );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestAbyssOnEnterGame(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	if (QuestIsActive(pQuest))
	{
		int nBossIndex = sGetBossTypeToSpawn(pQuest);
		if (nBossIndex != QUEST_ABYSS_MINIBOSS_INVALID)
		{
			s_QuestAdvanceTo(pQuest, QUEST_STATE_ABYSS_FIGHT_BOSS);
			s_sSaveBossIndexToFight( pQuest, nBossIndex );
		}
	}
}