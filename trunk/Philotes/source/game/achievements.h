//----------------------------------------------------------------------------
// achievements.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _ACHIEVEMENTS_H_
#define _ACHIEVEMENTS_H_

#ifndef __QUESTSTRUCTS_H_
#include "QuestStructs.h"
#endif
//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define MAX_ACHV_UNIT_TYPES		10
//----------------------------------------------------------------------------
enum ACHIEVEMENT_TYPE
{
	ACHV_KILL,
	ACHV_DIE,
	ACHV_WEAPON_KILL,
	ACHV_EQUIP,
	ACHV_SKILL_KILL,
	ACHV_TIMED_KILLS,
	ACHV_SUMMON,
	ACHV_STAT_VALUE,
	ACHV_CUBE_USE,  //For tugboat puzzlebox use

	ACHV_COLLECT,
	ACHV_QUEST_COMPLETE,
	ACHV_MINIGAME_WIN,
	ACHV_ITEM_USE,
	ACHV_ITEM_MOD,
	ACHV_ITEM_CRAFT,
	ACHV_ITEM_UPGRADE,
	ACHV_ITEM_IDENTIFY,
	ACHV_ITEM_DISMANTLE,
	ACHV_LEVELING_TIME,
	ACHV_SKILL_LEVEL,
	ACHV_SKILL_TREE_COMPLETE,
	ACHV_LEVEL_FIND,
	ACHV_PARTY_INVITE,
	ACHV_PARTY_ACCEPT,
	ACHV_INVENTORY_FILL,
};

enum ACHIEVEMENT_REVEAL_TYPE
{
	REVEAL_ACHV_ALWAYS,
	REVEAL_ACHV_AMOUNT,
	REVEAL_ACHV_COMPLETE,
	REVEAL_ACHV_PARENT_COMPLETE
};

//----------------------------------------------------------------------------
// STRUCT
//----------------------------------------------------------------------------
struct ACHIEVEMENT_UNLOCK_PLAYER_DATA
{
	char					szName[DEFAULT_INDEX_SIZE];
	WORD					wCode;	
	int						nPlayerLevel;
	PCODE					codeConditional;
};
//----------------------------------------------------------------------------
struct ACHIEVEMENT_DATA
{
	int						nIndex;
	char					szName[DEFAULT_INDEX_SIZE];
	WORD					wCode;							// code

	int						nNameString;
	int						nDescripString;
	int						nDetailsString;
	int						nRewardTypeString;
	char					szIcon[DEFAULT_XML_FRAME_NAME_SIZE];

	ACHIEVEMENT_REVEAL_TYPE	eRevealCondition;
	int						nRevealValue;
	int						nRevealParentID;
	int						nPlayerType[ MAX_ACHV_UNIT_TYPES ];
	ACHIEVEMENT_TYPE		eAchievementType;	
	int						nActiveWhenParentIDIsComplete;
	int						nCompleteNumber;
	int						nParam1;

	int						nUnitType[ MAX_ACHV_UNIT_TYPES ];	
	int						nQuestTaskTugboat;
	int						nRandomQuests;
	int						nCraftingFailures;
	int						nMonsterClass;
	int						nObjectClass;
	int						nItemUnittype[1];
	int						nItemQuality;
	int						nSkill;
	int						nLevel;
	int						nStat;

	int						nRewardAchievementPoints;
	int						nRewardTreasureClass;
	int						nRewardXP;
	int						nRewardSkill;
	int						nRewardTitleString;
	PCODE					codeRewardScript;
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
BOOL AchievementsExcelPostProcess(
	EXCEL_TABLE * table);

void s_AchievementsSendKillUndirected(
	UNIT *pPlayer,
	UNIT *pDefender);

BOOL s_AchievementSendCubeUse(
	  UNIT *pItemResult,
	  UNIT *pPlayer);

BOOL s_AchievementsSendDie(
	UNIT *pDefender,
	UNIT *pAttacker,
	UNIT *pAttacker_ultimate_source);

BOOL s_AchievementsSendKill(
	int nAchievement,
	UNIT *pAttacker,
	UNIT *pDefender,
	UNIT *pAttacker_ultimate_source,	
	UNIT *pDefender_ultimate_source,	
	UNIT *pWeapons[ MAX_WEAPONS_PER_UNIT ]);

BOOL s_AchievementsSendSkillKill(
	int nAchievement,
	UNIT *pAttacker,
	UNIT *pDefender,
	int nSkill);

BOOL s_AchievementsSendItemPickup(
	int nAchievement,
	UNIT *pItem,
	UNIT *pPlayer);

BOOL s_AchievementsSendItemEquip(
	int nAchievement,
	UNIT *pItem,
	UNIT *pPlayer);

BOOL s_AchievementsSendItemUse(
	int nAchievement,
	UNIT *pItem,
	UNIT *pPlayer);

BOOL s_AchievementsSendItemMod(
	UNIT *pItem,
	UNIT *pPlayer);

BOOL s_AchievementsSendSummon(
	UNIT *pPet,
	UNIT *pPlayer);

BOOL s_AchievementsSendItemCraft(
	UNIT *pItem,
	UNIT *pPlayer);

BOOL s_AchievementsSendItemCraft(
	UNIT *pItem,
	UNIT *pPlayer,
	BOOL bSuccess);

BOOL s_AchievementsSendItemUpgrade(
	UNIT *pItem,
	UNIT *pPlayer);

BOOL s_AchievementsSendItemIdentify(
	UNIT *pItem,
	UNIT *pPlayer);

BOOL s_AchievementsSendItemDismantle(
	UNIT *pItem,
	UNIT *pPlayer);

BOOL s_AchievementsSendInvMoveUndirected(
	UNIT* pNewContainer, 
	struct INVLOC_HEADER *pLocNew, 
	UNIT* pOldContainer, 
	struct INVLOC_HEADER *pLocOld );

BOOL s_AchievementsSendItemCollect(
	int nAchievement,
	UNIT *pItem,
	UNIT *pPlayer);

BOOL s_AchievementsSendPlayerLevelUp(
	UNIT *pPlayer);

BOOL s_AchievementsSendPlayerRankUp(
	UNIT *pPlayer);

BOOL s_AchievementsSendMinigameWin(
	UNIT *pPlayer);

BOOL s_AchievementsSendSkillLevelUp(
	UNIT *pPlayer,
	int nSkill);

BOOL s_AchievementsSendLevelEnter(
	int nAchievement,
	UNIT *pPlayer,
	int nLevel);

// this could be generalized to AchievementsSendQuestMessage
//    and send an enum about what kind of quest action happened
BOOL s_AchievementsSendQuestComplete(
	int nQuest,
	UNIT *pPlayer);

BOOL s_AchievementsSendQuestComplete(
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
	UNIT *pPlayer);

BOOL c_AchievementIsRevealed(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement);

BOOL c_AchievementGetName(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement,
	WCHAR *szBuf,
	int	nBufLen);

BOOL c_AchievementGetDescription(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement,
	WCHAR *szBuf,
	int	nBufLen);

BOOL c_AchievementGetDetails(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement,
	WCHAR *szBuf,
	int	nBufLen);

BOOL c_AchievementGetRewardName(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement,
	WCHAR *szBuf,
	int	nBufLen);

BOOL c_AchievementGetAP(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement,
	WCHAR *szBuf,
	int	nBufLen,
	BOOL bFull);

BOOL c_AchievementGetProgress(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement,
	WCHAR *szBuf,
	int	nBufLen);

BOOL x_AchievementIsComplete(
	UNIT *pPlayer,
	const ACHIEVEMENT_DATA *pAchievement);

BOOL x_AchievementCanDo(
	UNIT *pPlayer,
	const ACHIEVEMENT_DATA *pAchievement);

void s_AchievementStatchangeCallback(
	UNIT *pPlayer,
	int wStat,
	DWORD dwParam,
	int nOldValue,
	int nNewValue,
	const struct STATS_CALLBACK_DATA & data,
	BOOL bSend);

void s_AchievementRequirmentTest( UNIT *pPlayer, 
								  UNIT *pActivator,
								  UNIT *pTarget, 
								  UNIT * weapons[ MAX_WEAPONS_PER_UNIT ] );


BOOL x_AchievementIsCompleteBySkillID( UNIT *pPlayer,
							    int nSkillID );
#if ISVERSION(DEVELOPMENT)
void s_CheatCheatCheatAchievementProgress(
	UNIT *pPlayer,
	int nAchievement,
	BOOL bClear,
	BOOL bComplete,
	int nAmount = 0);
#endif

int AchievementGetSlotCount();
BOOL x_AchievementPlayerSelect( UNIT *pPlayer, int nAchievementID, int nSlotIndex );
void s_AchievementRemoveBySlot( UNIT *pPlayer, int nSlotIndex );
BOOL AchievementSlotIsValid( UNIT *pPlayer, int nSlotIndex, int nSkill = INVALID_ID, int nSkillLevel = INVALID_ID );
void s_AchievementSetStats( UNIT *pPlayer );
void s_AchievementsInitialize( UNIT *pPlayer );
const ACHIEVEMENT_DATA * AchievementGetBySlotIndex( UNIT *pPlayer, int nSlotIndex );
//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------

#endif // _ACHIEVEMENTS_H_