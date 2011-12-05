//----------------------------------------------------------------------------
// FILE: s_quests.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __SQUESTS_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __SQUESTS_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef __GLOBALINDEX_H_
#include "globalindex.h"
#endif //ifndef __GLOBALINDEX_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
enum NPC_DIALOG_TYPE;
struct QUEST;
enum QUEST_STATE_VALUE;
enum QUEST_STATUS;
struct ITEMSPAWN_RESULT;
enum INTERACT_INFO;
enum UNIT_INT3ERACT;
struct QUEST_TASK_DEFINITION_TUGBOAT;
enum UNIT_INTERACT;

//----------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------

#define MAX_QUEST_GIVEN_ITEMS		4

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

typedef void FP_QUEST_OPERATE_DELAY( QUEST * pQuest, UNITID idUnit );

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void s_QuestsUpdate(
	GAME *pGame);

void s_QuestAddCastMember(
	QUEST *pQuest,
	GENUS eGenus,
	int nClass);
	
void s_QuestUpdateCastInfo(
	QUEST *pQuest);

void s_QuestUpdateAvailability(
	QUEST *pQuest);

void s_QuestsUpdateAvailability(
	UNIT *pPlayer);
	
void s_QuestDelayNext(
	QUEST * pQuest,
	int nDurationTicks );

BOOL s_QuestAllStatesComplete(
	QUEST *pQuest,
	BOOL bIgnoreLastState);
		
void s_QuestStateSetForAll(
	QUEST *pQuest, 
	int nState,
	QUEST_STATE_VALUE eValue,
	BOOL bUpdateCastInfo = TRUE);
	
void s_QuestStateSetFromClient(
	UNIT * player,
	int nQuest,
	int nState,
	QUEST_STATE_VALUE eValue,
	BOOL bUpdate );
	
BOOL s_QuestStateRestore( 
	QUEST *pQuest, 
	int nState,
	QUEST_STATE_VALUE eValue,
	BOOL bSend = FALSE);
	
BOOL s_QuestActivate(
	QUEST *pQuest,
	BOOL bAllowTrackingChanges = TRUE);

void s_QuestDeactivate(
	QUEST *pQuest,
	BOOL bAllowTrackingChanges = TRUE);

void s_QuestReactivate(
	QUEST *pQuest);
	
void s_QuestActivateForAll(
	QUEST *pQuest );
	
BOOL s_QuestActivateFromClient(
	UNIT * player,
	int nQuest);

BOOL s_QuestIsPrereqComplete(
	QUEST *pQuest);
	
BOOL s_QuestComplete(
	QUEST *pQuest);

BOOL s_QuestOffering(
	QUEST *pQuest);
	
BOOL s_QuestClose(
	QUEST *pQuest);
	
BOOL s_QuestFail(
	QUEST *pQuest);
	
UNIT *s_QuestGetGiver( 
	QUEST *pQuest);
	
UNIT *s_QuestGetRewarder(
	QUEST *pQuest);


void s_QuestAccept(
	GAME* pGame,
	UNIT* pPlayer,
	UNIT* pNPC,
	int nQuestID );

void s_QuestEventMonsterDying(
	UNIT *pKiller,
	UNIT *pVictim);
	
void s_QuestEventMonsterKill(
	UNIT *pKiller,
	UNIT *pVictim);

void s_QuestEventPlayerDie(
	UNIT * pPlayer,
	UNIT * pKiller);

void s_QuestEventLevelCreated( 
	GAME *pGame, 
	LEVEL *pLevel );

void s_QuestEventTransitionLevel(
	UNIT* pPlayer,
	LEVEL* pLevelOld,
	LEVEL* pLevelNew);

void s_QuestEventAboutToLeaveLevel(
	UNIT * pPlayer);

void s_QuestEventLeaveLevel(
	UNIT * pPlayer,
	LEVEL * pLevelDest);

void s_QuestEventTransitionSubLevel(
	UNIT *pPlayer,
	LEVEL *pLevel,
	SUBLEVELID idSubLevelOld,
	SUBLEVELID idSubLevelNew,
	ROOM * pOldRoom );
	
void s_QuestEventRoomEntered(
	GAME *pGame,
	UNIT *pPlayer,
	ROOM *pRoom);

void s_QuestEventRoomActivated( 
	ROOM * pRoom, 
	UNIT * pActivator );

void s_QuestEventObjectOperated(
	UNIT *pOperator,
	UNIT *pTarget,
	BOOL bDoTrigger );
	
void s_QuestEventLootSpawned(
	UNIT *pSpawner,
	ITEMSPAWN_RESULT *pLoot);
	
BOOL s_QuestEventCanPickup(
	UNIT *pPlayer,
	UNIT *pAttemptingToPickedUp );

void s_QuestEventPickup(
	UNIT *pPlayer,
	UNIT *pPickedUp);

void s_QuestEventItemCrafted(
	UNIT *pPlayer,
	UNIT *pCrafted);
	
void s_QuestEventApproachPlayer(
	UNIT *pPlayer,
	UNIT *pTarget);
	
void s_QuestEventUnitStateChange(
	UNIT * pPlayer,
	int nState,
	BOOL bStateSet );

void s_QuestEventClientQuestStateChange(
	int nQuest,
	int nQuestState,
	QUEST_STATE_VALUE eValue,
	UNIT *pPlayer);
	
UNIT_INTERACT s_QuestEventInteract(
	UNIT *pPlayer,
	UNIT *pSubject,
	int nQuestID = INVALID_ID );
	
void s_QuestEventInteractGeneric(
	UNIT *pPlayer,
	UNIT *pSubject,
	UNIT_INTERACT eInteract,
	int nQuestID = INVALID_ID );

void s_QuestEventTalkStopped(
	UNIT *pPlayer,
	UNIT *pTalkingTo,
	int nDialog,
	int nRewardSelection );

void s_QuestEventTalkCancelled(
	UNIT *pPlayer,
	UNIT *pTalkingTo,
	int nDialog );

void s_QuestEventDialogClosed(
	UNIT *pPlayer,
	UNIT *pTalkingTo,
	int nDialog);

void s_QuestEventTipStopped(
	UNIT *pPlayer,
	int nTip );

void s_QuestEventStatus( 
	QUEST *pQuest, 
	QUEST_STATUS eOldStatus );

void s_QuestEventActivated(
	QUEST *pQuest);

void s_QuestEventTogglePortal( 
	UNIT * pPlayer, 
	UNIT * pPortal );

INTERACT_INFO s_QuestsGetInteractInfo(
	UNIT *pPlayer,
	UNIT *pSubject);
	
UNIT * s_QuestFindFirstItemOfType( 
	UNIT *player, 
	int nItemClass,
	DWORD dwFlags = NONE );

BOOL s_QuestCheckForItem( 
	UNIT * player, 
	int nItemClass,
	DWORD dwFlags = NONE);
	
BOOL s_QuestRemoveItem( 
	UNIT * player, 
	int nItemClass,
	DWORD dwFlags = NONE,
	int nQuantity = -1);

void s_QuestRemoveItemForAll( 
	QUEST *pQuest,
	int nItemClass,
	DWORD dwFlags = NONE);
	
BOOL s_QuestRemoveQuestItems( 
	UNIT * pPlayer, 
	QUEST * pQuest,
	DWORD dwFlags = NONE);

USE_RESULT s_QuestsUseItem(
	UNIT * pPlayer,
	UNIT * pItem );
	
BOOL s_QuestCanBeActivated(
	QUEST *pQuest);

void s_QuestSendKnownQuestsToClient( 
	UNIT *pPlayer);

BOOL s_QuestDoSetup(
	UNIT *pPlayer,
	BOOL bFirstGameJoin);
	
void s_QuestCompleteToStart(
	UNIT *pPlayer);
	
void s_QuestPlayerJoinGame(
	UNIT *pPlayer);
	
void s_QuestPlayerLeaveGame(
	UNIT *pPlayer);
		
void s_QuestClearAllStates(
	QUEST *pQuest,
	BOOL bSendMessage = FALSE );
	
void s_QuestOpenWarp(
	GAME * pGame,
	UNIT * pPlayer,
	int nWarpClass );
	
void s_QuestCloseWarp(
	GAME * pGame,
	UNIT * pPlayer,
	int nWarpClass );

void s_QuestAbortCurrent( 
	UNIT * player);
		
//void s_QuestSkipTutorial(
//	UNIT * pPlayer );

BOOL s_QuestStatusAllowsPlayerInteraction(
	QUEST_STATUS eStatus);

void s_QuestDoOnActivateStates(
	QUEST *pQuest,
	BOOL bRestoring);

//----------------------------------------------------------------------------
enum QUEST_GIVE_ITEM_FLAGS
{
	QGIF_IGNORE_GLOBAL_THEME_RULES_BIT,		// ignore global theme rules and spawn item because we said so!
};

BOOL s_QuestGiveItem(
	QUEST * pQuest,
	GLOBAL_INDEX eTreasure,
	DWORD dwQuestGiveItemFlags = 0);		// see QUEST_GIVE_ITEM_FLAGS

void s_QuestGiveGold(
	QUEST * pQuest,
	int nAmount );
	
void s_QuestVideoToAll(
	QUEST * pQuest,
	GLOBAL_INDEX eNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog);
	
void s_QuestVideoFromClient(
	UNIT * player,
	int nQuest,
	GLOBAL_INDEX eNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog);
		
void s_QuestTakeAllItemsAndClose(
	void *pUserData);

#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_QuestDisplayDialog( 
	QUEST *pQuest,
	UNIT *pDialogActivator,
	NPC_DIALOG_TYPE eType,
	int nDialog,
	int nDialogReward = INVALID_LINK,
	GLOBAL_INDEX eSecondNPC = GI_INVALID,
	BOOL bLeaveUIOpenOnOk = FALSE);





#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

BOOL s_QuestOfferReward(
	QUEST *pQuest,
	UNIT *pOfferer,
	BOOL bCreateOffer = TRUE);
	
BOOL s_QuestIsGiver(
	QUEST *pQuest,
	UNIT *pUnit);
	
BOOL s_QuestIsRewarder(
	QUEST *pQuest,
	UNIT *pUnit);

ROOM_LAYOUT_GROUP *s_QuestNodeSetGetNode( 
	ROOM * pRoom,
	ROOM_LAYOUT_GROUP * set, 
	ROOM_LAYOUT_SELECTION_ORIENTATION ** pOrientationOut, 
	int node );
	
ROOM_LAYOUT_GROUP *s_QuestNodeSetGetNodeByName( 
	ROOM_LAYOUT_GROUP * nodeset, 
	char * name );
	
ROOM_LAYOUT_GROUP *s_QuestNodeNext( 
	ROOM * pRoom,
	ROOM_LAYOUT_GROUP * nodeset, 
	ROOM_LAYOUT_SELECTION_ORIENTATION ** pOrientationOut, 
	ROOM_LAYOUT_GROUP * node );
	
void s_QuestNodeGetPosAndDir( 
	ROOM * room, 
	ROOM_LAYOUT_SELECTION_ORIENTATION * node, 
	VECTOR * pos, 
	VECTOR * dir, 
	float * angle, 
	float * pitch );
	
BOOL s_QuestNodeGetPathPosition( 
	ROOM * room, 
	char * szPathName, 
	int nPathNode, 
	VECTOR & vPosition );
	
#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
void s_QuestForceComplete(
	UNIT * pPlayer,
	int nQuest);
#endif

void s_QuestForceStatesComplete(
	UNIT * pPlayer,
	int nQuest,
	BOOL bMakeQuestStatusComplete);

// #if ISVERSION(CHEATS_ENABLED)  -- phu: currently in use by release app to skip tutorial
void s_QuestForceActivate(
	UNIT * pPlayer,
	int nQuest);
// #endif
			
#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
void s_QuestForceReset(
	UNIT * pPlayer,
	int nQuest);
#endif

#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
void s_QuestAdvance(
	QUEST *pQuest);
#endif

void s_QuestAdvanceTo( 
	QUEST *pQuest, 
	int nQuestState);

void s_QuestInactiveAdvanceTo(
	QUEST *pQuest,
	int nQuestState);
	
BOOL s_QuestCanRunDRLGRule(
	UNIT * player,
	LEVEL * level,
	char * pszRuleName );

void s_QuestMapReveal(
	QUEST * pQuest,
	GLOBAL_INDEX eGILevel );

int s_QuestAttemptToSpawnMissingObjectiveObjects(
	QUEST* pQuest,
	LEVEL* pLevel);

UNIT *s_QuestSpawnObject( 
	int nObjectClass, 
	UNIT *pLocation, 
	UNIT *pFacing);

void s_QuestOperateDelay(
	QUEST * pQuest,
	FP_QUEST_OPERATE_DELAY * pfCallback,
	UNITID idUnit,
	int nTicks );

void s_QuestNonPlayerOperateDelay(
	QUEST * pQuest,
	UNIT * pUnit,
	FP_QUEST_OPERATE_DELAY * pfCallback,
	UNITID idUnitOperated,
	int nTicks );

UNIT *s_QuestCastMemberGetInstanceBySpecies( 
	QUEST *pQuest,
	SPECIES spMember);

void s_QuestsDroppedItem( 
	UNIT *pDropper, 
	int nItemClass);

void s_QuestEndTrainRide(
	GAME * pGame );

void s_QuestAIUpdate(
	UNIT * pMonster,
	int nQuestIndex );

void s_QuestSkillNotify(
	UNIT * pPlayer,
	int nQuestIndex,
	int nSkillIndex );

void s_QuestAbandon(
	QUEST * pQuest );

void s_QuestTrack( 
	QUEST * pQuest, 
	BOOL bEnableTracking);

void s_QuestBackFromMovie(
	UNIT * pPlayer );

void s_QuestCleanupForSave(
	QUEST *pQuest);

void s_QuestVersionPlayer(
	UNIT *pPlayer);
	
	
#endif
