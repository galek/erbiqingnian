//----------------------------------------------------------------------------
// FILE: quest_stonehenge.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_quests.h"
#include "game.h"
#include "globalindex.h"
#include "npc.h"
#include "quest_stonehenge.h"
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


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum
{
	QUEST_STONEHENGE_MINIBOSS_BEAST,
	QUEST_STONEHENGE_MINIBOSS_SPECTRAL,
	QUEST_STONEHENGE_MINIBOSS_NECRO,
	QUEST_STONEHENGE_MINIBOSS_DEMON,

	NUM_QUEST_STONEHENGE_MINI_BOSSES
};

//----------------------------------------------------------------------------
enum
{
	QUEST_STONEHENGE_LEVEL_BOSS_LEADUP_1,
	QUEST_STONEHENGE_LEVEL_BOSS_LEADUP_2,
	QUEST_STONEHENGE_LEVEL_BOSS,

	NUM_QUEST_STONEHENGE_LEVELS_BOSS
};

//----------------------------------------------------------------------------
// 32 bit max for stat (STATS_STONEHENGE_QUEST_FLAGS)
enum STONEHENGE_QUEST_FLAGS
{
	IFF_OPEN_BEAST_MINIBOSS_PORTAL_BIT,		
	IFF_OPEN_SPECTRAL_MINIBOSS_PORTAL_BIT,		
	IFF_OPEN_NECRO_MINIBOSS_PORTAL_BIT,		
	IFF_OPEN_DEMON_MINIBOSS_PORTAL_BIT,		
	IFF_OPEN_BOSS_PORTAL_BIT,		

	IFF_KILLED_BEAST_MINIBOSS_BIT,		
	IFF_KILLED_SPECTRAL_MINIBOSS_BIT,		
	IFF_KILLED_NECRO_MINIBOSS_BIT,		
	IFF_KILLED_DEMON_MINIBOSS_BIT,		
	IFF_KILLED_BOSS_BIT,		
};

//----------------------------------------------------------------------------
struct QUEST_DATA_STONEHENGE
{
	// monster classes
	int		nMiniBoss[NUM_QUEST_STONEHENGE_MINI_BOSSES];
	int		nBoss;

	// level definition indexes
	int		nLevelMiniBoss[NUM_QUEST_STONEHENGE_MINI_BOSSES];
	int		nLevelLeadupMiniBoss[NUM_QUEST_STONEHENGE_MINI_BOSSES];	// level leading up to mini boss level
	int		nLevelsBoss[NUM_QUEST_STONEHENGE_LEVELS_BOSS];			// levels leading up to, and including boss level
	int		nLevelStonehenge;
	int		nLevelDarkWoods;

	// object classes to give essences or heads to to open portals below
	int		nHeadstoneMiniBoss[NUM_QUEST_STONEHENGE_MINI_BOSSES];
	int		nHeadstoneBoss;

	// portal objects opened by the headstones
	int		nPortalMiniBoss[NUM_QUEST_STONEHENGE_MINI_BOSSES];
	int		nPortalBoss;

	// item classes to give to the headstones
	int		nItemEssence[NUM_QUEST_STONEHENGE_MINI_BOSSES];
	int		nItemMiniBossHead[NUM_QUEST_STONEHENGE_MINI_BOSSES];
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_STONEHENGE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_STONEHENGE *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCountQuestItems(
	UNIT *pPlayer,
	GAME* pGame,
	QUEST* pQuest,
	int nItemClass)
{
	// what are we looking for
	SPECIES spItem = MAKE_SPECIES( GENUS_ITEM, nItemClass );

	// it must actually be on their person
	DWORD dwFlags = 0;
	SETBIT( dwFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
	SETBIT( dwFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );

	return UnitInventoryCountUnitsOfType( pPlayer, spItem, dwFlags );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHasAllMiniBossHeads(
	UNIT *pPlayer,
	GAME* pGame,
	QUEST* pQuest)
{
	const QUEST_DATA_STONEHENGE *pQuestData = sQuestDataGet( pQuest );

	for (int i = 0; i < NUM_QUEST_STONEHENGE_MINI_BOSSES; ++i)
	{
		if (sCountQuestItems( pPlayer, pGame, pQuest, pQuestData->nItemMiniBossHead[i] ) <= 0) 
		{
			return FALSE;
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHasEnoughOfAtLeastOneTypeOfEssence(
	UNIT *pPlayer,
	GAME* pGame,
	QUEST* pQuest)
{
	//const QUEST_DATA_STONEHENGE *pQuestData = sQuestDataGet( pQuest );

	for (int i = 0; i < NUM_QUEST_STONEHENGE_MINI_BOSSES; ++i)
	{
		if (QuestStateGetValue( pQuest, QUEST_STATE_STONEHENGE_COLLECT_ESSENCE0 + i ) == QUEST_STATE_COMPLETE)
			return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRemoveMiniBossHeads(
	UNIT *pPlayer,
	GAME* pGame,
	QUEST* pQuest)
{
	const QUEST_DATA_STONEHENGE *pQuestData = sQuestDataGet( pQuest );
	DWORD dwRemoveFlags = 0;
	SETBIT( dwRemoveFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
	SETBIT( dwRemoveFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );

	for (int i = 0; i < NUM_QUEST_STONEHENGE_MINI_BOSSES; ++i)
	{
		if (!s_QuestRemoveItem( pPlayer, pQuestData->nItemMiniBossHead[i], dwRemoveFlags, 1 ))
			return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sPlayerSetQuestFlag( GAME *pGame, UNIT *pPlayer, DWORD dwFlagBit, BOOL bFlagValue )
{
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	ASSERT_RETURN(nDifficulty >= 0);
	DWORD dwQuestFlags = UnitGetStat( pPlayer, STATS_STONEHENGE_QUEST_FLAGS, nDifficulty );
	SETBIT( dwQuestFlags, dwFlagBit, bFlagValue );
	UnitSetStat( pPlayer, STATS_STONEHENGE_QUEST_FLAGS, nDifficulty, dwQuestFlags );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct  PlayerStonehengeQuestCheckFlagData
{
	DWORD dwFlagBit;
	BOOL bFlagValue;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerCheckQuestFlag( UNIT *pPlayer, void *pUserData )
{
	PlayerStonehengeQuestCheckFlagData *pData = (PlayerStonehengeQuestCheckFlagData *)pUserData;
	ASSERT_RETURN(pData);
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	ASSERT_RETURN(nDifficulty >= 0);
	DWORD dwQuestFlags = UnitGetStat( pPlayer, STATS_STONEHENGE_QUEST_FLAGS, nDifficulty );
	if (TESTBIT( dwQuestFlags, pData->dwFlagBit ))
		pData->bFlagValue = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPartyMemberHasQuestFlag( GAME *pGame, UNIT *pPlayer, DWORD dwFlagBit )
{
	PlayerStonehengeQuestCheckFlagData tData;
	tData.dwFlagBit = dwFlagBit;
	tData.bFlagValue = FALSE;
	sPlayerCheckQuestFlag( pPlayer, &tData );
	if (tData.bFlagValue)
		return TRUE;

#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT( pGame ))
	{
		int nPartyMemberCount = c_PlayerGetPartyMemberCount();
		for (int i = 0; i < nPartyMemberCount; ++i)
		{
			UNITID idPartyMember = c_PlayerGetPartyMemberUnitId( i );
			if (idPartyMember != INVALID_ID)
			{
				UNIT *pPartyMember = UnitGetById( pGame, idPartyMember );
				if (pPartyMember)
				{
					sPlayerCheckQuestFlag( pPartyMember, &tData );
					if (tData.bFlagValue)
						return TRUE;
				}
			}
		}
	}
#endif //!ISVERSION(SERVER_VERSION)

#if !ISVERSION(CLIENT_ONLY_VERSION)
	if (IS_SERVER( pGame ))
	{
		PARTYID idParty = UnitGetPartyId( pPlayer );
		if (idParty != INVALID_ID)
			s_PartyIterateCachedPartyMembersInGame( pGame, idParty, sPlayerCheckQuestFlag, &tData );
	}

#endif //!ISVERSION(CLIENT_ONLY_VERSION)

	return tData.bFlagValue;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sPlayerOpenWarp( UNIT *pPlayer, void *pUserData )
{
	int *pWarpClass = (int*)pUserData;
	s_QuestOpenWarp( UnitGetGame( pPlayer ), pPlayer, *pWarpClass );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sOpenWarpForAllPartyMembers( GAME *pGame, UNIT *pPlayer, int nWarpClass )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(IS_SERVER( pGame ));

	PARTYID idParty = UnitGetPartyId( pPlayer );

	if (idParty == INVALID_ID)
		s_sPlayerOpenWarp( pPlayer, &nWarpClass);
	else
		s_PartyIterateCachedPartyMembersInGame( pGame, idParty, s_sPlayerOpenWarp, &nWarpClass );

#endif //!ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sCanOperateObject( 
	QUEST *pQuest,
	UNIT *pObject)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	//const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	const QUEST_DATA_STONEHENGE *pData = sQuestDataGet( pQuest );

	// figure out which mini boss this object should open the portal for
	if (UnitIsObjectClass( pObject, pData->nHeadstoneBoss ))
	{
		return (!sPartyMemberHasQuestFlag( pGame, pPlayer, IFF_OPEN_BOSS_PORTAL_BIT ) &&
				pData->nLevelStonehenge == UnitGetLevelDefinitionIndex( pObject ) &&
				QuestStateGetValue( pQuest, QUEST_STATE_STONEHENGE_COLLECT_HEADS ) ==  QUEST_STATE_COMPLETE) ?
					QMR_OPERATE_OK : QMR_OPERATE_FORBIDDEN;
	}	
	else if (UnitIsObjectClass( pObject, pData->nPortalBoss ))
	{
		return (sPartyMemberHasQuestFlag( pGame, pPlayer, IFF_OPEN_BOSS_PORTAL_BIT ) && 
				pData->nLevelStonehenge == UnitGetLevelDefinitionIndex( pObject )) ?
					QMR_OPERATE_OK : QMR_OPERATE_FORBIDDEN;
	}
	else
	{
		// figure out which mini boss this object should open the portal for
		for (int i = 0; i < NUM_QUEST_STONEHENGE_MINI_BOSSES; ++i)
		{
			BOOL bPartyMemberHasAccessToPortal = sPartyMemberHasQuestFlag( pGame, pPlayer, i );
			if (UnitIsObjectClass( pObject, pData->nHeadstoneMiniBoss[i] ))
			{
				return (!bPartyMemberHasAccessToPortal &&
						pData->nLevelDarkWoods == UnitGetLevelDefinitionIndex( pObject ) &&
						QuestStateGetValue( pQuest, QUEST_STATE_STONEHENGE_COLLECT_ESSENCE0 + i ) ==  QUEST_STATE_COMPLETE) ?
							QMR_OPERATE_OK : QMR_OPERATE_FORBIDDEN;
			}
			else if (UnitIsObjectClass( pObject, pData->nPortalMiniBoss[i] ))
			{
				return (bPartyMemberHasAccessToPortal && 
						pData->nLevelDarkWoods == UnitGetLevelDefinitionIndex( pObject )) ?
							QMR_OPERATE_OK : QMR_OPERATE_FORBIDDEN;
			}
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
	else if (s_QuestIsGiver( pQuest, pSubject ))
	{
		// check givers
		if (s_QuestCanBeActivated( pQuest ))
		{
			// there is new info available, a new quest				
			return QMR_NPC_INFO_NEW;
		}
		else if (QuestIsActive( pQuest ))
		{
			// the quest is active, the npc is waiting for you to do the quest tasks				
			return QMR_NPC_INFO_WAITING;
		}
		else if (QuestIsOffering( pQuest ))
		{
			return QMR_NPC_INFO_WAITING;
		}	
	}
	else if (s_QuestIsRewarder( pQuest, pSubject ))
	{
		// check rewarders
		if (QuestIsActive( pQuest ))
		{
			if (QuestStateGetValue( pQuest, QUEST_STATE_STONEHENGE_MASTER ) == QUEST_STATE_HIDDEN)
			{
				// start the next phase of the quest
				return QMR_NPC_INFO_NEW;
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
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest->nQuest );
	ASSERT_RETVAL( pQuestDefinition, QMR_OK );

	// Quest giver
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
				sHasEnoughOfAtLeastOneTypeOfEssence( pPlayer, pGame, pQuest ) ? DIALOG_STONEHENGE_HASENOUGHESSENCES : DIALOG_STONEHENGE_NOTENOUGHESSENCES );

			return QMR_NPC_TALK;
		}
	}
	else if (QuestIsActive( pQuest ))
	{
		if (s_QuestIsGiver( pQuest, pSubject ))
		{
			// explain what to do in wake hollow		
			s_QuestDisplayDialog(
				pQuest,
				pSubject,
				NPC_DIALOG_OK_CANCEL,
				sHasEnoughOfAtLeastOneTypeOfEssence( pPlayer, pGame, pQuest ) ? DIALOG_STONEHENGE_HASENOUGHESSENCES : DIALOG_STONEHENGE_NOTENOUGHESSENCES );

			return QMR_NPC_TALK;
		}
		else if (s_QuestIsRewarder( pQuest,  pSubject ))
		{
			// explain the head collection stuff
			s_QuestDisplayDialog(
				pQuest,
				pSubject,
				NPC_DIALOG_OK,
				QuestStateGetValue( pQuest, QUEST_STATE_STONEHENGE_COLLECT_HEADS ) ==  QUEST_STATE_COMPLETE 
					? DIALOG_STONEHENGE_HASENOUGHHEADS : DIALOG_STONEHENGE_NOTENOUGHHEADS );

			return QMR_NPC_TALK;
		}
	}
#endif		
	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	int nDialogStopped = pMessage->nDialog;
	//const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );
	//UNIT *pPlayer = QuestGetPlayer( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );

	switch (nDialogStopped)
	{
	case DIALOG_STONEHENGE_NOTENOUGHESSENCES:
	case DIALOG_STONEHENGE_HASENOUGHESSENCES:
		if (s_QuestIsGiver( pQuest, pTalkingTo ) &&
			s_QuestCanBeActivated( pQuest ))
		{
			// activate quest
			if (s_QuestActivate( pQuest ))
			{
				QuestStateSet( pQuest, QUEST_STATE_STONEHENGE_ESSENCE, QUEST_STATE_ACTIVE );

				if (nDialogStopped != DIALOG_STONEHENGE_NOTENOUGHESSENCES)
				{
					QuestStateSet( pQuest, QUEST_STATE_STONEHENGE_ESSENCE, QUEST_STATE_COMPLETE );
					QuestStateSet( pQuest, QUEST_STATE_STONEHENGE_LEADER, QUEST_STATE_ACTIVE );
				}
			}
		}
		break;

	case DIALOG_STONEHENGE_NOTENOUGHHEADS:
		if (s_QuestIsRewarder( pQuest, pTalkingTo ) )
		{
			if (QuestStateGetValue( pQuest, QUEST_STATE_STONEHENGE_MASTER ) == QUEST_STATE_HIDDEN)
				QuestStateSet( pQuest, QUEST_STATE_STONEHENGE_MASTER, QUEST_STATE_ACTIVE );
		}
		break;

	case DIALOG_STONEHENGE_HASENOUGHHEADS:
		if (s_QuestIsRewarder( pQuest, pTalkingTo ) )
		{
			// complete quest
			if (QuestStateGetValue( pQuest, QUEST_STATE_STONEHENGE_MASTER ) != QUEST_STATE_COMPLETE)
				QuestStateSet( pQuest, QUEST_STATE_STONEHENGE_MASTER, QUEST_STATE_COMPLETE );
		}
		break;
	}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

		return QMR_OK;		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageMonsterDying( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_DYING *pMessage)
{
	QUEST_DATA_STONEHENGE* pData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	ASSERT_RETVAL( IS_SERVER( pGame ), QMR_OK );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	UNIT *pPlayer = QuestGetPlayer( pQuest );

	// no loot if the guy who gave access to the boss isn't there, to avoid exploits
	if (UnitIsMonsterClass( pVictim, pData->nBoss ) &&
		pData->nLevelsBoss[QUEST_STONEHENGE_LEVEL_BOSS] == UnitGetLevelDefinitionIndex( pVictim ) )
	{
		if (!sPartyMemberHasQuestFlag( pGame, pPlayer, IFF_OPEN_BOSS_PORTAL_BIT ))
		{
			UnitClearFlag( pVictim, UNITFLAG_GIVESLOOT );
		}
	}
	else
	{
		for (int i=0; i < NUM_QUEST_STONEHENGE_MINI_BOSSES; ++i)
		{
			if (UnitIsMonsterClass( pVictim, pData->nMiniBoss[i] ) &&
				pData->nLevelMiniBoss[i] == UnitGetLevelDefinitionIndex( pVictim ) )
			{
				if (!sPartyMemberHasQuestFlag( pGame, pPlayer, i ))
				{
					UnitClearFlag( pVictim, UNITFLAG_GIVESLOOT );
				}
			}
		}
	}
	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_STONEHENGE* pData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	ASSERT_RETVAL( IS_SERVER( pGame ), QMR_OK );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	UNIT *pPlayer = QuestGetPlayer( pQuest );

	if (UnitIsMonsterClass( pVictim, pData->nBoss ) &&
		pData->nLevelsBoss[QUEST_STONEHENGE_LEVEL_BOSS] == UnitGetLevelDefinitionIndex( pVictim ) )
	{
		int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
		DWORD dwQuestFlags = UnitGetStat( pPlayer, STATS_STONEHENGE_QUEST_FLAGS, nDifficulty );
		if (TESTBIT( dwQuestFlags, IFF_OPEN_BOSS_PORTAL_BIT ))
		{
			// do something special when they kill the main boss?
			CLEARBIT( dwQuestFlags, IFF_OPEN_BOSS_PORTAL_BIT );
			SETBIT( dwQuestFlags, IFF_KILLED_BOSS_BIT );
			UnitSetStat( pPlayer, STATS_STONEHENGE_QUEST_FLAGS, nDifficulty, dwQuestFlags );

			s_QuestComplete( pQuest );
		}
	}
	else
	{
		for (int i=0; i < NUM_QUEST_STONEHENGE_MINI_BOSSES; ++i)
		{
			if (UnitIsMonsterClass( pVictim, pData->nMiniBoss[i] ) &&
				pData->nLevelMiniBoss[i] == UnitGetLevelDefinitionIndex( pVictim ) )
			{
				int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
				DWORD dwQuestFlags = UnitGetStat( pPlayer, STATS_STONEHENGE_QUEST_FLAGS, nDifficulty );
				if (TESTBIT( dwQuestFlags, i ))
				{
					//spawns the item from the defender
					UNIT *pCreated = 
						s_SpawnItemFromUnit( 
							pVictim,
							pPlayer,
							pData->nItemMiniBossHead[i]);

					ASSERTV( pCreated != NULL, "quest \"%s\": failed to collection item", pQuest->pQuestDef->szName );
					if( pCreated != NULL )
					{
						// Allow the player to pick it up, but not the party.
						// A monster kill by a party member can result in a quest
						// items spawned for another party member.
						UnitSetRestrictedToGUID( 
							pCreated, 
							UnitGetGuid( pPlayer ) );

						CLEARBIT( dwQuestFlags, i );
						SETBIT( dwQuestFlags, i + IFF_KILLED_BEAST_MINIBOSS_BIT );
						UnitSetStat( pPlayer, STATS_STONEHENGE_QUEST_FLAGS, nDifficulty, dwQuestFlags );
					}
				}

			}
		}
	}
	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestUpdateCollectionStates( 
	GAME* pGame,
	QUEST* pQuest,
	int nItemClass = INVALID_LINK )
{
	ASSERT_RETVAL( IS_SERVER( pGame ), QMR_OK );
	UNIT* pPlayer = QuestGetPlayer( pQuest );

	// check for completing the collection
	const QUEST_DATA_STONEHENGE *pData = sQuestDataGet( pQuest );
	//BOOL bCheckHeads = TRUE;
	for (int i = 0; i < NUM_QUEST_STONEHENGE_MINI_BOSSES; ++i)
	{
		if (nItemClass == INVALID_LINK || pData->nItemEssence[i] == nItemClass)
		{
			QuestStateSet( pQuest, QUEST_STATE_STONEHENGE_COLLECT_ESSENCE0 + i, QUEST_STATE_HIDDEN, TRUE, TRUE );
			QuestStateSet( pQuest, QUEST_STATE_STONEHENGE_COLLECT_ESSENCE0 + i, QUEST_STATE_ACTIVE, TRUE, TRUE );
			if (sCountQuestItems( pPlayer, pGame, pQuest, pData->nItemEssence[i] ) >= pQuest->pQuestDef->nObjectiveCount)
			{
				QuestStateSet( pQuest, QUEST_STATE_STONEHENGE_COLLECT_ESSENCE0 + i, QUEST_STATE_COMPLETE, TRUE, TRUE );
			}

			if (nItemClass != INVALID_LINK)
				return QMR_OK;
		}
	}

	QuestStateSet( pQuest, QUEST_STATE_STONEHENGE_COLLECT_HEADS, QUEST_STATE_HIDDEN, TRUE, TRUE );	
	QuestStateSet( pQuest, QUEST_STATE_STONEHENGE_COLLECT_HEADS, QUEST_STATE_ACTIVE, TRUE, TRUE );	
	if (sHasAllMiniBossHeads( pPlayer, pGame, pQuest ))
	{
		QuestStateSet( pQuest, QUEST_STATE_STONEHENGE_COLLECT_HEADS, QUEST_STATE_COMPLETE, TRUE, TRUE );
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestObjectOperated(
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage )
{
	ASSERT_RETVAL( IS_SERVER( QuestGetGame( pQuest ) ), QMR_OK );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pObject = pMessage->pTarget;
	UNIT *pPlayer = QuestGetPlayer( pQuest );

	if (!sCanOperateObject(pQuest, pObject))
		return QMR_OK;

	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	const QUEST_DATA_STONEHENGE *pData = sQuestDataGet( pQuest );

	// figure out which mini boss this object should open the portal for
	if (UnitIsObjectClass( pObject, pData->nHeadstoneBoss ))
	{
		if (sRemoveMiniBossHeads( pPlayer, pGame, pQuest ))
		{
			s_sPlayerSetQuestFlag( pGame, pPlayer, IFF_OPEN_BOSS_PORTAL_BIT, TRUE );
			s_sQuestUpdateCollectionStates( pGame, pQuest );
			s_sOpenWarpForAllPartyMembers( pGame, pPlayer, pData->nPortalBoss );
		}

		return QMR_OK;
	}	
	else
	{
		// figure out which mini boss this object should open the portal for
		for (int i = 0; i < NUM_QUEST_STONEHENGE_MINI_BOSSES; ++i)
		{
			if (UnitIsObjectClass( pObject, pData->nHeadstoneMiniBoss[i] ))
			{
				DWORD dwRemoveFlags = 0;
				SETBIT( dwRemoveFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
				SETBIT( dwRemoveFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );
				if (s_QuestRemoveItem( pPlayer, pData->nItemEssence[i], dwRemoveFlags, pQuestDef->nObjectiveCount ))
				{
					s_sPlayerSetQuestFlag( pGame, pPlayer, i, TRUE );
					s_sQuestUpdateCollectionStates( pGame, pQuest );
					s_sOpenWarpForAllPartyMembers( pGame, pPlayer, pData->nPortalMiniBoss[i] );
				}

				return QMR_OK;
			}
		}
	}

	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessagePickup( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_PICKUP* pMessage)
{
	UNIT* pPlayer = pMessage->pPlayer;
	ASSERTX_RETVAL( pPlayer == QuestGetPlayer( pQuest ), QMR_OK, "Player should be the owner of this quest" );
	ASSERTX_RETVAL( pMessage->pPickedUp, QMR_OK, "expected picked up item" );
	return s_sQuestUpdateCollectionStates( pGame, pQuest, UnitGetClass( pMessage->pPickedUp ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageDroppingItem( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_DROPPED_ITEM* pMessage)
{
	UNIT* pPlayer = pMessage->pPlayer;
	ASSERTX_RETVAL( pPlayer == QuestGetPlayer( pQuest ), QMR_OK, "Player should be the owner of this quest" );
	return s_sQuestUpdateCollectionStates( pGame, pQuest, pMessage->nItemClass );
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
	case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return s_sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}


	//----------------------------------------------------------------------------
	case QM_SERVER_PICKUP:
		{
			const QUEST_MESSAGE_PICKUP *pMessagePickup = (const QUEST_MESSAGE_PICKUP*)pMessage;
			return s_sQuestMessagePickup( 
				UnitGetGame( QuestGetPlayer( pQuest ) ), 
				pQuest, 
				pMessagePickup);
		}

	//----------------------------------------------------------------------------
	case QM_SERVER_DROPPED_ITEM:
		{
			const QUEST_MESSAGE_DROPPED_ITEM *pMessageDropping = (const QUEST_MESSAGE_DROPPED_ITEM*)pMessage;
			return s_sQuestMessageDroppingItem( 
				UnitGetGame( QuestGetPlayer( pQuest ) ), 
				pQuest, 
				pMessageDropping);
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeStonehenge(
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
	QUEST_DATA_STONEHENGE * pQuestData)
{
	// monster classes
	pQuestData->nMiniBoss[QUEST_STONEHENGE_MINIBOSS_BEAST] = QuestUseGI( pQuest, GI_MONSTER_STONEHENGE_MINI_BOSS_BEAST );
	pQuestData->nMiniBoss[QUEST_STONEHENGE_MINIBOSS_SPECTRAL] = QuestUseGI( pQuest, GI_MONSTER_STONEHENGE_MINI_BOSS_SPECTRAL );
	pQuestData->nMiniBoss[QUEST_STONEHENGE_MINIBOSS_NECRO] = QuestUseGI( pQuest, GI_MONSTER_STONEHENGE_MINI_BOSS_NECRO );
	pQuestData->nMiniBoss[QUEST_STONEHENGE_MINIBOSS_DEMON] = QuestUseGI( pQuest, GI_MONSTER_STONEHENGE_MINI_BOSS_DEMON );

	pQuestData->nBoss = QuestUseGI( pQuest, GI_MONSTER_STONEHENGE_BOSS );

	// level definition indexes
	pQuestData->nLevelMiniBoss[QUEST_STONEHENGE_MINIBOSS_BEAST] = QuestUseGI( pQuest, GI_LEVEL_WOODS_BOSS_BEAST );
	pQuestData->nLevelMiniBoss[QUEST_STONEHENGE_MINIBOSS_SPECTRAL] = QuestUseGI( pQuest, GI_LEVEL_WOODS_BOSS_SPECTRAL );
	pQuestData->nLevelMiniBoss[QUEST_STONEHENGE_MINIBOSS_NECRO] = QuestUseGI( pQuest, GI_LEVEL_WOODS_BOSS_NECRO );
	pQuestData->nLevelMiniBoss[QUEST_STONEHENGE_MINIBOSS_DEMON] = QuestUseGI( pQuest, GI_LEVEL_WOODS_BOSS_DEMON );

	pQuestData->nLevelLeadupMiniBoss[QUEST_STONEHENGE_MINIBOSS_BEAST] = QuestUseGI( pQuest, GI_LEVEL_WOODS_CAVE_BEAST );
	pQuestData->nLevelLeadupMiniBoss[QUEST_STONEHENGE_MINIBOSS_SPECTRAL] = QuestUseGI( pQuest, GI_LEVEL_WOODS_CAVE_SPECTRAL );
	pQuestData->nLevelLeadupMiniBoss[QUEST_STONEHENGE_MINIBOSS_NECRO] = QuestUseGI( pQuest, GI_LEVEL_WOODS_CAVE_NECRO );
	pQuestData->nLevelLeadupMiniBoss[QUEST_STONEHENGE_MINIBOSS_DEMON] = QuestUseGI( pQuest, GI_LEVEL_WOODS_CAVE_DEMON );


	pQuestData->nLevelsBoss[QUEST_STONEHENGE_LEVEL_BOSS_LEADUP_1] = QuestUseGI( pQuest, GI_LEVEL_THE_MAZE );
	pQuestData->nLevelsBoss[QUEST_STONEHENGE_LEVEL_BOSS_LEADUP_2] = QuestUseGI( pQuest, GI_LEVEL_MOLOCHS_STEP );
	pQuestData->nLevelsBoss[QUEST_STONEHENGE_LEVEL_BOSS] = QuestUseGI( pQuest, GI_LEVEL_STONEHENGE_BOSS );

	pQuestData->nLevelStonehenge = QuestUseGI( pQuest, GI_LEVEL_STONEHENGE );
	pQuestData->nLevelDarkWoods = QuestUseGI( pQuest, GI_LEVEL_DARK_WOODS );

	// object classes to give essences or heads to to open portals below
	pQuestData->nHeadstoneMiniBoss[QUEST_STONEHENGE_MINIBOSS_BEAST] = QuestUseGI( pQuest, GI_OBJECT_WOODS_HEADSTONE_BEAST );
	pQuestData->nHeadstoneMiniBoss[QUEST_STONEHENGE_MINIBOSS_SPECTRAL] = QuestUseGI( pQuest, GI_OBJECT_WOODS_HEADSTONE_SPECTRAL );
	pQuestData->nHeadstoneMiniBoss[QUEST_STONEHENGE_MINIBOSS_NECRO] = QuestUseGI( pQuest, GI_OBJECT_WOODS_HEADSTONE_NECRO );
	pQuestData->nHeadstoneMiniBoss[QUEST_STONEHENGE_MINIBOSS_DEMON] = QuestUseGI( pQuest, GI_OBJECT_WOODS_HEADSTONE_DEMON );

	pQuestData->nHeadstoneBoss = QuestUseGI( pQuest, GI_OBJECT_STONEHENGE_HEADSTONE_MAZE );

	// portal objects opened by the headstones
	pQuestData->nPortalMiniBoss[QUEST_STONEHENGE_MINIBOSS_BEAST] = QuestUseGI( pQuest, GI_OBJECT_WOODS_PORTAL_BEAST );
	pQuestData->nPortalMiniBoss[QUEST_STONEHENGE_MINIBOSS_SPECTRAL] = QuestUseGI( pQuest, GI_OBJECT_WOODS_PORTAL_SPECTRAL );
	pQuestData->nPortalMiniBoss[QUEST_STONEHENGE_MINIBOSS_NECRO] = QuestUseGI( pQuest, GI_OBJECT_WOODS_PORTAL_NECRO );
	pQuestData->nPortalMiniBoss[QUEST_STONEHENGE_MINIBOSS_DEMON] = QuestUseGI( pQuest, GI_OBJECT_WOODS_PORTAL_DEMON );

	pQuestData->nPortalBoss = QuestUseGI( pQuest, GI_OBJECT_STONEHENGE_PORTAL_MAZE );

	// item classes to give to the headstones
	pQuestData->nItemEssence[QUEST_STONEHENGE_MINIBOSS_BEAST] = QuestUseGI( pQuest, GI_ITEM_ESSENCE_BEAST );
	pQuestData->nItemEssence[QUEST_STONEHENGE_MINIBOSS_SPECTRAL] = QuestUseGI( pQuest, GI_ITEM_ESSENCE_SPECTRAL );
	pQuestData->nItemEssence[QUEST_STONEHENGE_MINIBOSS_NECRO] = QuestUseGI( pQuest, GI_ITEM_ESSENCE_NECRO );
	pQuestData->nItemEssence[QUEST_STONEHENGE_MINIBOSS_DEMON] = QuestUseGI( pQuest, GI_ITEM_ESSENCE_DEMON );

	pQuestData->nItemMiniBossHead[QUEST_STONEHENGE_MINIBOSS_BEAST] = QuestUseGI( pQuest, GI_ITEM_BOSS_HEAD_BEAST );
	pQuestData->nItemMiniBossHead[QUEST_STONEHENGE_MINIBOSS_SPECTRAL] = QuestUseGI( pQuest, GI_ITEM_BOSS_HEAD_SPECTRAL );
	pQuestData->nItemMiniBossHead[QUEST_STONEHENGE_MINIBOSS_NECRO] = QuestUseGI( pQuest, GI_ITEM_BOSS_HEAD_NECRO );
	pQuestData->nItemMiniBossHead[QUEST_STONEHENGE_MINIBOSS_DEMON] = QuestUseGI( pQuest, GI_ITEM_BOSS_HEAD_DEMON );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitStonehenge(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	

	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_STONEHENGE * pQuestData = ( QUEST_DATA_STONEHENGE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_STONEHENGE ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// quest message flags
	//pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS );

	for (int i=0; i < NUM_QUEST_STONEHENGE_MINI_BOSSES; ++i)
		s_QuestAddCastMember( pQuest, GENUS_OBJECT, pQuestData->nHeadstoneMiniBoss[i] );

	s_QuestAddCastMember( pQuest, GENUS_OBJECT, pQuestData->nHeadstoneBoss );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestStonehengeOnEnterGame(
	const QUEST_FUNCTION_PARAM &tParam)
{
	UNIT *pPlayer = tParam.pPlayer;
	QUEST *pQuest = tParam.pQuest;

	// reset item count stat
	GAME *pGame = UnitGetGame( pPlayer );

	s_sQuestUpdateCollectionStates( pGame, pQuest );
}