//----------------------------------------------------------------------------
// achievements.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "excel_private.h"
#include "globalindex.h"
#include "achievements.h"
#include "unit_priv.h" // also includes units.h, which includes game.h
#include "performance.h"
#include "stringtable.h"
#include "language.h"
#include "monsters.h"
#include "items.h"
#include "skills.h"
#include "imports.h"
#include "quests.h"
#include "states.h"
#include "c_message.h"
#include "skills.h"
#include "script.h"
#include "skill_strings.h"
#include "quest.h"

//----------------------------------------------------------------------------
// LOCAL DATA
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
enum
{
	UND_WANT_KILL = 0,
	UND_WANT_INV_MOVE,
	NUM_WANT_UNDIRECTED
};
ACHIEVEMENT_DATA * spWantUndirected[NUM_WANT_UNDIRECTED][100];
static int snNumWantUndirected[NUM_WANT_UNDIRECTED];

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
BOOL c_AchievementIsRevealed(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement)
{
	ASSERT_RETFALSE(pPlayer);
	ASSERT_RETFALSE(UnitIsPlayer(pPlayer));
	ASSERT_RETFALSE	(pAchievement);

#if ISVERSION(DEVELOPMENT)
	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(pGame)
	if (GameGetDebugFlag(pGame, DEBUGFLAG_REVEAL_ALL_ACHIEVEMENTS))
		return TRUE;
#endif

	if (!x_AchievementCanDo(pPlayer, pAchievement))
	{
		return FALSE;
	}

	switch (pAchievement->eRevealCondition)
	{
	case REVEAL_ACHV_PARENT_COMPLETE:
		{
			ASSERT_RETFALSE( pAchievement->nRevealParentID != INVALID_ID );
			ACHIEVEMENT_DATA *pAchievementParent = (ACHIEVEMENT_DATA *) ExcelGetData(NULL, DATATABLE_ACHIEVEMENTS, pAchievement->nRevealParentID);
			ASSERT_RETFALSE	(pAchievementParent);
			return x_AchievementIsComplete( pPlayer, pAchievementParent );
		}
	case REVEAL_ACHV_ALWAYS:
		{
			return TRUE;
		}
	case REVEAL_ACHV_AMOUNT:
		{
			int nProgress = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex);

			return nProgress >= pAchievement->nRevealValue;
		}
	case REVEAL_ACHV_COMPLETE:
		{
			int nProgress = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex);

			return (nProgress == pAchievement->nCompleteNumber);
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL x_AchievementIsComplete(
	UNIT *pPlayer,
	const ACHIEVEMENT_DATA *pAchievement)
{
	ASSERT_RETFALSE(pPlayer);
	ASSERT_RETFALSE(UnitIsPlayer(pPlayer));

	ASSERT_RETFALSE	(pAchievement);

	int nProgress = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex);

	return (nProgress == pAchievement->nCompleteNumber);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL x_AchievementCanDo(
	UNIT *pPlayer,
	const ACHIEVEMENT_DATA *pAchievement)
{
	ASSERT_RETFALSE(pPlayer);
	ASSERT_RETFALSE(UnitIsPlayer(pPlayer));

	ASSERT_RETFALSE(pPlayer);
	ASSERT_RETFALSE(UnitIsPlayer(pPlayer));
	if( pAchievement->nActiveWhenParentIDIsComplete != INVALID_ID )
	{
		ACHIEVEMENT_DATA *pAchievementParent = (ACHIEVEMENT_DATA *) ExcelGetData(NULL, DATATABLE_ACHIEVEMENTS, pAchievement->nActiveWhenParentIDIsComplete );
		ASSERT_RETFALSE	(pAchievementParent);
		if( x_AchievementIsComplete( pPlayer, pAchievementParent ) == FALSE )
			return FALSE;
	}
	if (pAchievement->nPlayerType[0] != 0)
	{
		if (!IsAllowedUnit(pPlayer, pAchievement->nPlayerType, arrsize(pAchievement->nPlayerType)))
			return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAchievementGetString(
	int nString,
	ACHIEVEMENT_DATA *pAchievement,
	UNIT *pPlayer,
	WCHAR *szBuf,
	int	nBufLen)
{
	ASSERT_RETFALSE(pPlayer);
	ASSERT_RETFALSE(UnitIsPlayer(pPlayer));
	GAME *pGame = UnitGetGame(pPlayer);

	ASSERT_RETFALSE	(pAchievement);

	if( AppIsHellgate() ||
		pAchievement->nRewardSkill == INVALID_ID )
	{
		switch(nString)
		{
		case 1: PStrCopy(szBuf, StringTableGetStringByIndex(pAchievement->nNameString), nBufLen);
			break;
		case 2: PStrCopy(szBuf, StringTableGetStringByIndex(pAchievement->nDescripString), nBufLen);
			break;
		case 3: PStrCopy(szBuf, StringTableGetStringByIndex(pAchievement->nDetailsString), nBufLen);
			break;
		case 4: PStrCopy(szBuf, StringTableGetStringByIndex(pAchievement->nRewardTypeString), nBufLen);
			break;
		}
	}
	else if( AppIsTugboat() ) 
	{
		//this is a bit of a hack but for how Mouse overs work and how strings work we don't want to have to add text entries into all the achievements and skills - lets just do the skills
		//nRewardSkill won't be invalid here
		const SKILL_DATA *pSkillData = SkillGetData( pGame, pAchievement->nRewardSkill );
		ASSERT_RETFALSE( pSkillData );
		switch(nString)
		{
		case 1: PStrCopy(szBuf, StringTableGetStringByIndex( pSkillData->nDisplayName), nBufLen);
			break;
		case 2: 			
			SkillGetString( pAchievement->nRewardSkill, pPlayer, SKILL_STRING_DESCRIPTION, STT_CURRENT_LEVEL, szBuf, nBufLen, SKILL_STRING_MASK_DESCRIPTION, FALSE );			
			break;
		case 3: 
			{
				SkillGetString( pAchievement->nRewardSkill, pPlayer, SKILL_STRING_EFFECT, STT_CURRENT_LEVEL, szBuf, nBufLen, 0, FALSE );			
				for( int t = 0; t < nBufLen; t++ ) //this is so ghettooooooooooooo
				{
					if( szBuf[t] == 10 )
						szBuf[t] = L',';
				}
				WCHAR tmpChar[2] = { 10, 0 };
				PStrReplaceToken(szBuf, nBufLen, tmpChar, L","); //this doesn't work
			}
			break;
		case 4: PStrCopy(szBuf, StringTableGetStringByIndex(pAchievement->nRewardTypeString), nBufLen);
			break;
		}
	}

	if (pAchievement->nMonsterClass != INVALID_LINK)
	{
		const UNIT_DATA *pMonsterData = MonsterGetData(pGame, pAchievement->nMonsterClass);
		ASSERT_RETFALSE	(pMonsterData);

		GRAMMAR_NUMBER eNumber = pAchievement->nCompleteNumber > 1 ? PLURAL : SINGULAR;
		DWORD dwAttributes = StringTableGetGrammarNumberAttribute( eNumber );
		const WCHAR *puszString = StringTableGetStringByIndexVerbose(pMonsterData->nString, dwAttributes, 0, NULL, NULL);

		PStrReplaceToken(szBuf, nBufLen, L"[monster]", puszString);
	}
					// vvv only checking one now - shouldn't be mixing types anyway
	else if (UnitIsA(pAchievement->nUnitType[0], UNITTYPE_MONSTER))
	{
		const UNITTYPE_DATA* pUnitTypeData = (UNITTYPE_DATA*)ExcelGetData( pGame, DATATABLE_UNITTYPES, pAchievement->nUnitType[0] );
		GRAMMAR_NUMBER eNumber = pAchievement->nCompleteNumber > 1 ? PLURAL : SINGULAR;
		DWORD dwAttributes = StringTableGetGrammarNumberAttribute( eNumber );
		const WCHAR *puszString = StringTableGetStringByIndexVerbose(pUnitTypeData->nDisplayName, dwAttributes, 0, NULL, NULL);

		PStrReplaceToken(szBuf, nBufLen, L"[monster]", puszString);
	}

	if (pAchievement->nItemUnittype[0] != INVALID_LINK)
	{
		const UNITTYPE_DATA *pItemData = (UNITTYPE_DATA *)ExcelGetData(pGame, DATATABLE_UNITTYPES, pAchievement->nItemUnittype[0]);
		//ASSERT_RETFALSE	(pItemData);
		GRAMMAR_NUMBER eNumber = pAchievement->nCompleteNumber > 1 ? PLURAL : SINGULAR;
		DWORD dwAttributes = StringTableGetGrammarNumberAttribute( eNumber );
		const WCHAR *puszString = StringTableGetStringByIndexVerbose(pItemData->nDisplayName, dwAttributes, 0, NULL, NULL);




		PStrReplaceToken(szBuf, nBufLen, L"[item]", puszString);
	}
					// vvv only checking one now - shouldn't be mixing types anyway
	else if (UnitIsA(pAchievement->nUnitType[0], UNITTYPE_ITEM))
	{
		const UNITTYPE_DATA* pUnitTypeData = (UNITTYPE_DATA*)ExcelGetData( pGame, DATATABLE_UNITTYPES, pAchievement->nUnitType[0] );
		PStrReplaceToken(szBuf, nBufLen, L"[item]", StringTableGetStringByIndex(pUnitTypeData->nDisplayName));
	}

	if (pAchievement->nItemQuality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA *pQualityData = (ITEM_QUALITY_DATA *)ExcelGetData(pGame, DATATABLE_ITEM_QUALITY, pAchievement->nItemQuality);
		ASSERT_RETFALSE	(pQualityData);

		PStrReplaceToken(szBuf, nBufLen, L"[quality]", StringTableGetStringByIndex(pQualityData->nDisplayName));
	}

	int nProgress = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex);
	WCHAR szNumber[256];
	LanguageFormatIntString(szNumber, arrsize(szNumber), nProgress);
	PStrReplaceToken(szBuf, nBufLen, L"[progress]", szNumber);

	LanguageFormatIntString(szNumber, arrsize(szNumber), pAchievement->nCompleteNumber);
	PStrReplaceToken(szBuf, nBufLen, L"[completenum]", szNumber);

	LanguageFormatIntString(szNumber, arrsize(szNumber), pAchievement->nParam1);
	PStrReplaceToken(szBuf, nBufLen, L"[param1]", szNumber);

	return TRUE;
}




BOOL c_AchievementGetName(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement,
	WCHAR *szBuf,
	int	nBufLen)
{
	return sAchievementGetString(1, pAchievement, pPlayer, szBuf, nBufLen);
}

BOOL c_AchievementGetDescription(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement,
	WCHAR *szBuf,
	int	nBufLen)
{
	return sAchievementGetString(2, pAchievement, pPlayer, szBuf, nBufLen);
}

BOOL c_AchievementGetDetails(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement,
	WCHAR *szBuf,
	int	nBufLen)
{
	return sAchievementGetString(3, pAchievement, pPlayer, szBuf, nBufLen);
}

BOOL c_AchievementGetRewardName(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement,
	WCHAR *szBuf,
	int	nBufLen)
{
	return sAchievementGetString(4, pAchievement, pPlayer, szBuf, nBufLen);
}

BOOL c_AchievementGetAP(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement,
	WCHAR *szBuf,
	int	nBufLen,
	BOOL bFull)
{
	if (bFull)
	{
		PStrCopy(szBuf, GlobalStringGet(GS_ACHIEVEMENT_ACCOMPLISHED_POINTS_FMT), nBufLen);

		WCHAR szNumber[256];
		LanguageFormatIntString(szNumber, arrsize(szNumber), pAchievement->nRewardAchievementPoints);
		PStrReplaceToken(szBuf, nBufLen, L"[number]", szNumber);
	}
	else
	{
		LanguageFormatIntString(szBuf, nBufLen, pAchievement->nRewardAchievementPoints);
	}

	return TRUE;
}

BOOL c_AchievementGetProgress(
	UNIT *pPlayer,
	ACHIEVEMENT_DATA *pAchievement,
	WCHAR *szBuf,
	int	nBufLen)
{

	//WCHAR szNumber1[256];
	//WCHAR szNumber2[256];
	//LanguageFormatIntString(szNumber2, arrsize(szNumber2), pAchievement->nCompleteNumber);
	int nProgress = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex);
	//LanguageFormatIntString(szNumber1, arrsize(szNumber1), nProgress);
	//PStrPrintf(szBuf, nBufLen, L"%s/%s", szNumber1, szNumber2);

	ASSERT_RETFALSE(pAchievement->nCompleteNumber != 0);
	int nPercent = (nProgress * 100) / pAchievement->nCompleteNumber;
	PStrPrintf(szBuf, nBufLen, L"%d%%", nPercent);
	
	return TRUE;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void s_CheatCheatCheatAchievementProgress(
	UNIT *pPlayer,
	int nAchievement,
	BOOL bClear,
	BOOL bComplete,
	int nAmount /*= 0*/)
{
	ASSERT_RETURN(pPlayer);
	ASSERT_RETURN(UnitIsPlayer(pPlayer));
	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETURN(IS_SERVER(pGame));

	ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, nAchievement);
	ASSERT_RETURN	(pAchievement);

	int nProgress = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, nAchievement);

	if (bClear)
		nProgress = 0;
	else if (bComplete)
		nProgress = pAchievement->nCompleteNumber;
	else
		nProgress += nAmount;
	nProgress = PIN(nProgress, 0, pAchievement->nCompleteNumber);

	UnitSetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, nAchievement, nProgress);

	if (nProgress == pAchievement->nCompleteNumber)
	{
		UnitChangeStat(pPlayer, STATS_ACHIEVEMENT_POINTS_CUR,	pAchievement->nRewardAchievementPoints);	
		UnitChangeStat(pPlayer, STATS_ACHIEVEMENT_POINTS_TOTAL, pAchievement->nRewardAchievementPoints);	
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_AchievementCheckComplete(
	UNIT *pPlayer,
	const ACHIEVEMENT_DATA *pAchievement)
{
	int nProgress = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex);

	if (nProgress == pAchievement->nCompleteNumber)
	{		
		if( AppIsTugboat() ) //Hellgate could use this...
		{
			const STATE_DATA* state_data = StateGetData( NULL, STATE_ACHIEVEMENT_UNLOCKED);
			s_StateSet(pPlayer, pPlayer, STATE_ACHIEVEMENT_UNLOCKED, state_data->nDefaultDuration * GAME_TICKS_PER_SECOND / MSECS_PER_SEC);

		}

		UnitChangeStat(pPlayer, STATS_ACHIEVEMENT_POINTS_CUR,	pAchievement->nRewardAchievementPoints);	
		UnitChangeStat(pPlayer, STATS_ACHIEVEMENT_POINTS_TOTAL, pAchievement->nRewardAchievementPoints);	
		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_AchievementProgressSet(
	UNIT *pPlayer,
	const ACHIEVEMENT_DATA *pAchievement,
	int nAmount)
{
	ASSERT_RETURN(pPlayer);
	ASSERT_RETURN(UnitIsPlayer(pPlayer));
	ASSERT_RETURN(pAchievement);

	if (!x_AchievementCanDo(pPlayer, pAchievement))
	{
		return;
	}

	if (x_AchievementIsComplete(pPlayer, pAchievement))
		return;

	nAmount = PIN(nAmount, 0, pAchievement->nCompleteNumber);
	UnitSetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex, nAmount);

	s_AchievementCheckComplete(pPlayer, pAchievement);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_AchievementProgress(
	UNIT *pPlayer,
	const ACHIEVEMENT_DATA *pAchievement,
	int nDelta)
{
	ASSERT_RETURN(pPlayer);
	ASSERT_RETURN(UnitIsPlayer(pPlayer));
	ASSERT_RETURN(pAchievement);

	if (!x_AchievementCanDo(pPlayer, pAchievement))
	{
		return;
	}

	if (x_AchievementIsComplete(pPlayer, pAchievement))
		return;

	int nBefore = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex);
	int nAfter = MIN(nBefore+nDelta, pAchievement->nCompleteNumber);
	UnitSetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex, nAfter);

	s_AchievementCheckComplete(pPlayer, pAchievement);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL s_AchievementSendCubeUse(
	  UNIT *pItemResult,
	  UNIT *pPlayer)
{
	if(!pItemResult || !pPlayer)
		return false;

	if(!UnitIsA(pPlayer, UNITTYPE_PLAYER))
		return false;

	GAME *pGame = UnitGetGame(pPlayer);
	if(!pGame)
		return false;

	BOOL bResult = FALSE;

	unsigned int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);
	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{

		ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
		ASSERT_RETFALSE	(pAchievement);

		if (!x_AchievementCanDo(pPlayer, pAchievement))
		{
			continue;
		}

		if (pAchievement->eAchievementType == ACHV_CUBE_USE)
		{
			//check the unittype for the achivement if applicable
			if (pAchievement->nItemUnittype[0] != INVALID_LINK &&
				!UnitIsA(pItemResult, pAchievement->nItemUnittype[0]))
			{
				continue;
			}

			s_AchievementProgress(pPlayer, pAchievement, 1);
			bResult = TRUE;
		}
		


	}

	return bResult;




}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendDie(
	UNIT *pDefender,
	UNIT *pAttacker,
	UNIT *pAttacker_ultimate_source)
{
	BOOL bResult = FALSE;

	if(!pAttacker_ultimate_source)
		return FALSE;

	if(!UnitIsPlayer(pDefender))
		return FALSE;

	GAME *pGame = UnitGetGame(pDefender);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	unsigned int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);
	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{

		ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
		ASSERT_RETFALSE	(pAchievement);

		if (!x_AchievementCanDo(pDefender, pAchievement))
		{
			continue;
		}

		if (pAchievement->eAchievementType == ACHV_DIE)
		{

			if (pAchievement->nUnitType[0] != INVALID_LINK &&
				!UnitIsA(pAttacker_ultimate_source, pAchievement->nUnitType[0]))
			{
				continue;
			}

			s_AchievementProgress(pDefender, pAchievement, 1);
			bResult = TRUE;
		}
		


	}

	return bResult;


}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendKill(
	int nAchievement,
	UNIT *pAttacker,
	UNIT *pDefender,
	UNIT *pAttacker_ultimate_source,	
	UNIT *pDefender_ultimate_source,	
	UNIT *pWeapons[ MAX_WEAPONS_PER_UNIT ])
{
	ASSERT_RETFALSE(nAchievement > INVALID_LINK);
	if (!pAttacker_ultimate_source)
		return FALSE;

	if (!UnitIsPlayer(pAttacker_ultimate_source))
	{
		return FALSE;
	}

	GAME *pGame = UnitGetGame(pAttacker_ultimate_source);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, nAchievement);
	ASSERT_RETFALSE	(pAchievement);

	if (!x_AchievementCanDo(pAttacker_ultimate_source, pAchievement))
	{
		return FALSE;
	}

	if (pAchievement->eAchievementType == ACHV_KILL)
	{
		if (IsAllowedUnit(pDefender, pAchievement->nUnitType, arrsize(pAchievement->nUnitType)) || 
			( pAchievement->nMonsterClass != -1 && UnitGetClass(pDefender) == pAchievement->nMonsterClass ) ||
			( pAchievement->nObjectClass != -1 && UnitGetClass(pDefender) == pAchievement->nObjectClass) )
		{
			// no PK achievement for killing lower leveled players in tugboat
			if(AppIsTugboat() && UnitIsPlayer(pDefender))
			{
				if ( UnitGetExperienceLevel(pDefender) < UnitGetExperienceLevel(pAttacker_ultimate_source))
				{
					return FALSE;
				}
			}
			s_AchievementProgress(pAttacker_ultimate_source, pAchievement, 1);
			return TRUE;
			
		}
		else
		{
			char szMsg[1024];
			PStrPrintf(szMsg, arrsize(szMsg), "Target [%s] is not valid for achievement [%s]", pDefender->pUnitData->szName, pAchievement->szName);
			ASSERTX_RETFALSE(FALSE, szMsg);
			return FALSE;
		}
	}

	if (pAchievement->eAchievementType == ACHV_WEAPON_KILL)
	{
		BOOL bMatchesTypes = TRUE;
		ASSERT_RETFALSE(bMatchesTypes);
		if (pAchievement->nItemUnittype[0] != INVALID_LINK)
		{
			BOOL bFoundOne = FALSE;
			if (pWeapons)
			{
				for (int i=0; i < MAX_WEAPONS_PER_UNIT; i++)
				{
					if (pWeapons[i] && UnitIsA(pWeapons[i],pAchievement->nItemUnittype[0]))
					{
						bFoundOne = TRUE;
						break;
					}
				}
			}
			bMatchesTypes &= bFoundOne;

			if (pAchievement->nMonsterClass == INVALID_LINK &&
				pAchievement->nUnitType[0] != 0)
			{
				bMatchesTypes &= IsAllowedUnit(pDefender, pAchievement->nUnitType, arrsize(pAchievement->nUnitType));
			}
		}

		if (pAchievement->nMonsterClass != INVALID_LINK)
		{
			bMatchesTypes &= (UnitGetClass(pDefender) == pAchievement->nMonsterClass);

			if (pAchievement->nItemUnittype[0] == INVALID_LINK &&
				pAchievement->nUnitType[0] != 0)
			{
				BOOL bFoundOne = FALSE;
				if (pWeapons)
				{
					for (int i=0; i < MAX_WEAPONS_PER_UNIT; i++)
					{
						if (pWeapons[i] && IsAllowedUnit(pWeapons[i], pAchievement->nUnitType, arrsize(pAchievement->nUnitType)))
						{
							bFoundOne = TRUE;
							break;
						}
					}
				}
				bMatchesTypes &= bFoundOne;
			}

		}

		if (bMatchesTypes)
		{
			s_AchievementProgress(pAttacker_ultimate_source, pAchievement, 1);

			return TRUE;
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendSummon(
	UNIT *pPet,
	UNIT *pPlayer)
{
	BOOL bResult = FALSE;

	if (!pPlayer)
		return bResult;

	if (!UnitIsPlayer(pPlayer))
	{
		return bResult;
	}

	//make sure the player is the direct owner (no mind control widget pets)
	if(UnitGetOwnerUnit(pPet) != pPlayer)
	{
		return bResult;
	}

	//No hirelings
	if(UnitIsA(pPet,UNITTYPE_HIRELING))
	{
		return bResult;
	}



	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	
	unsigned int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);
	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{
	
		ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
		ASSERT_RETFALSE	(pAchievement);

		if (!x_AchievementCanDo(pPlayer, pAchievement))
		{
			continue;
		}

		if (pAchievement->eAchievementType == ACHV_SUMMON)
		{

			if (pAchievement->nUnitType[0] != INVALID_LINK &&
				!UnitIsA(pPet, pAchievement->nUnitType[0]))
			{
				continue;
			}

			s_AchievementProgress(pPlayer, pAchievement, 1);
			bResult = TRUE;
		}
	}

	return bResult;


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendItemPickup(
	int nAchievement,
	UNIT *pItem,
	UNIT *pPlayer)
{
	ASSERT_RETFALSE(nAchievement > INVALID_LINK);
	if (!pPlayer)
		return FALSE;

	if (!UnitIsPlayer(pPlayer))
	{
		return FALSE;
	}

	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, nAchievement);
	ASSERT_RETFALSE	(pAchievement);

	if (!x_AchievementCanDo(pPlayer, pAchievement))
	{
		return FALSE;
	}
	if (pAchievement->eAchievementType == ACHV_COLLECT)
	{
		
		if (pAchievement->nItemUnittype[0] != INVALID_LINK &&
			!UnitIsA(pItem, pAchievement->nItemUnittype[0]))
		{
			return FALSE;
		}

		if (pAchievement->nItemUnittype[0] == INVALID_LINK &&
			pAchievement->nUnitType[0] != 0)
		{
			if (!IsAllowedUnit(pItem, pAchievement->nUnitType, arrsize(pAchievement->nUnitType)))
			{
				return FALSE;
			}
		}
		int nQuality = ItemGetQuality(pItem);
		if (pAchievement->nItemQuality != INVALID_LINK &&
			nQuality != pAchievement->nItemQuality)
		{
			return FALSE;
		}

		//items created by people don't get credit for picking the item up
		if( ItemWasCreatedByPlayer( pItem, pPlayer ) )
		{
			return FALSE;
		}

		s_AchievementProgress(pPlayer, pAchievement, 1);

		return TRUE;
	}

	return FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendSkillKill(
	int nAchievement,
	UNIT *pAttacker,
	UNIT *pDefender,
	int nSkill)
{
	ASSERT_RETFALSE(nAchievement > INVALID_LINK);
	if (!pAttacker)
		return FALSE;

	if (!UnitIsPlayer(pAttacker))
	{
		return FALSE;
	}

	GAME *pGame = UnitGetGame(pAttacker);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, nAchievement);
	ASSERT_RETFALSE	(pAchievement);

	if (!x_AchievementCanDo(pAttacker, pAchievement))
	{
		return FALSE;
	}

	if (pAchievement->eAchievementType == ACHV_SKILL_KILL)
	{
		ASSERT_RETFALSE(pAchievement->nSkill == nSkill);

		if (pAchievement->nMonsterClass != INVALID_LINK &&
			UnitGetClass(pDefender) != pAchievement->nMonsterClass)
		{
			return FALSE;
		}

		if (pAchievement->nMonsterClass == INVALID_LINK &&
			pAchievement->nUnitType[0] != 0)
		{
			if (!IsAllowedUnit(pDefender, pAchievement->nUnitType, arrsize(pAchievement->nUnitType)))
			{
				return FALSE;
			}
		}

		s_AchievementProgress(pAttacker, pAchievement, 1);

		return TRUE;
	}

	return FALSE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_AchievementsSendKillUndirected(
	UNIT *pPlayer,
	UNIT *pDefender)
{
	for (int iAchievement = 0; iAchievement < snNumWantUndirected[UND_WANT_KILL]; iAchievement++)
	{
		ACHIEVEMENT_DATA *pAchievement = spWantUndirected[UND_WANT_KILL][iAchievement];

		if (pAchievement->eAchievementType == ACHV_TIMED_KILLS)
		{
			int nKilled = VMMonstersKilledNonTeam(pPlayer, pAchievement->nParam1);
			int nBefore = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex);

			if (nKilled > nBefore)
			{
				s_AchievementProgressSet(pPlayer, pAchievement, nKilled);
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendItemEquip(
	int nAchievement,
	UNIT *pItem,
	UNIT *pPlayer)
{
	if (!pPlayer || !pItem)
		return FALSE;

	if (!UnitIsPlayer(pPlayer))
	{
		return FALSE;
	}
	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	ASSERT_RETFALSE(nAchievement > INVALID_LINK);
	ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, nAchievement);
	ASSERT_RETFALSE	(pAchievement);

	if (!x_AchievementCanDo(pPlayer, pAchievement))
	{
		return FALSE;
	}

	if (pAchievement->eAchievementType != ACHV_EQUIP)
		return FALSE;

	DWORD dwFlags = 0;
	SETBIT( dwFlags, IIF_EQUIP_SLOTS_ONLY_BIT );
	int nCount = 0;

	UNIT * pItemCurr = UnitInventoryIterate( pPlayer, NULL, dwFlags);
	while (pItemCurr != NULL)
	{
		int nQuality = ItemGetQuality(pItemCurr);
	
		BOOL bOk = TRUE;
		if (pAchievement->nItemQuality != INVALID_LINK &&
			!ItemQualityIsEqualToOrLessThanQuality( pAchievement->nItemQuality, nQuality) )
		{
			bOk = FALSE;
		}

		if (pAchievement->nItemUnittype[0] != INVALID_LINK)
		{
			if (!UnitIsA(pItem,pAchievement->nItemUnittype[0]))
				bOk = FALSE;
		}
		else
		if (pAchievement->nUnitType[0] != 0)
		{
			if (!IsAllowedUnit(pItemCurr, pAchievement->nUnitType, arrsize(pAchievement->nUnitType)))
				bOk = FALSE;
		}

		if (bOk)
		{
			nCount++;
		}
		pItemCurr = UnitInventoryIterate( pPlayer, pItemCurr, dwFlags );
	}

	s_AchievementProgressSet(pPlayer, pAchievement, nCount);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAchievementsSendItemCommon(
	const ACHIEVEMENT_DATA *pAchievement,
	UNIT *pItem,
	UNIT *pPlayer)
{
	ASSERT_RETFALSE(pAchievement);

	if (!pItem)
		return FALSE;

	if (!UnitIsPlayer(pPlayer))
	{
		return FALSE;
	}
	if (pAchievement->nItemUnittype[0] != INVALID_LINK && 
		!UnitIsA(pItem,pAchievement->nItemUnittype[0]))
	{
		return FALSE;
	}
	else
	if (pAchievement->nUnitType[0] != 0 &&
		!IsAllowedUnit(pItem, pAchievement->nUnitType, arrsize(pAchievement->nUnitType)))
	{
		return FALSE;
	}

	s_AchievementProgress(pPlayer, pAchievement, 1);

	return TRUE;
}

BOOL s_AchievementsSendItemUse(
	int nAchievement,
	UNIT *pItem,
	UNIT *pPlayer)
{
	if (!pPlayer)
		return FALSE;

	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, nAchievement);
	ASSERT_RETFALSE(pAchievement);

	ASSERT_RETFALSE(pAchievement->eAchievementType == ACHV_ITEM_USE);

	return sAchievementsSendItemCommon(pAchievement, pItem, pPlayer);

}

static BOOL sAchievementItemFindCommmon(
	ACHIEVEMENT_TYPE eType,
	UNIT *pItem,
	UNIT *pPlayer)
{
	if (!pPlayer)
		return FALSE;

	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	unsigned int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);
	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{
		const ACHIEVEMENT_DATA * pAchievement = (ACHIEVEMENT_DATA *)ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
		if ( ! pAchievement )
			continue;

		if (pAchievement->eAchievementType == eType)
		{
			sAchievementsSendItemCommon(pAchievement, pItem, pPlayer);
		}
	}
	return TRUE;
}

BOOL s_AchievementsSendItemMod(
	UNIT *pItem,
	UNIT *pPlayer)
{
	ASSERT_RETFALSE(pItem);

	// make sure this is the first one
	int nCount = 0;
	UNIT *pContainedItem = NULL;
	while ((pContainedItem = UnitInventoryIterate(pItem, pContainedItem)) != NULL)
	{
		if (UnitIsA(pContainedItem, UNITTYPE_MOD))
		{
			if (++nCount > 1)
				return FALSE;
		}
	}

	return sAchievementItemFindCommmon(ACHV_ITEM_MOD, pItem, pPlayer);
}

BOOL s_AchievementsSendItemCraft(
	UNIT *pItem,
	UNIT *pPlayer)
{
	return s_AchievementsSendItemCraft(pItem,pPlayer,TRUE);
}

BOOL s_AchievementsSendItemCraft(
	UNIT *pItem,
	UNIT *pPlayer,
	BOOL bSuccess = TRUE)
{
	if(AppIsHellgate())
	{
		return sAchievementItemFindCommmon(ACHV_ITEM_CRAFT, pItem, pPlayer);
	}

	if(!pPlayer)
		return FALSE;
	if(!UnitIsPlayer(pPlayer))
		return FALSE;

	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));
	
	BOOL bResult(FALSE);
	unsigned int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);
	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{
		const ACHIEVEMENT_DATA * pAchievement = (ACHIEVEMENT_DATA *)ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
		if ( ! pAchievement )
			continue;

		//check the AchievementType
		if (pAchievement->eAchievementType != ACHV_ITEM_CRAFT)
			continue;
		//make sure the success type matches the craft success
		if(pAchievement->nCraftingFailures  == bSuccess)
			continue;

		//if we've got an item
		if(pItem)
		{
			// check the unittype if applicable
			if (pAchievement->nItemUnittype[0] != INVALID_LINK &&
				!UnitIsA(pItem, pAchievement->nItemUnittype[0]))
			{
				continue;
			}
		}

		//we made it though the checks, so increment the achievement
		 s_AchievementProgress(pPlayer, pAchievement, 1);
		 bResult = TRUE;
		
	}
	return bResult;

	
}



BOOL s_AchievementsSendItemUpgrade(
	UNIT *pItem,
	UNIT *pPlayer)
{
	return sAchievementItemFindCommmon(ACHV_ITEM_UPGRADE, pItem, pPlayer);
}

BOOL s_AchievementsSendItemIdentify(
	UNIT *pItem,
	UNIT *pPlayer)
{
	return sAchievementItemFindCommmon(ACHV_ITEM_IDENTIFY, pItem, pPlayer);
}

BOOL s_AchievementsSendItemDismantle(
	UNIT *pItem,
	UNIT *pPlayer)
{
	return sAchievementItemFindCommmon(ACHV_ITEM_DISMANTLE, pItem, pPlayer);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendInvMoveUndirected(
	UNIT* pNewContainer, 
	INVLOC_HEADER *pLocNew, 
	UNIT* pOldContainer, 
	INVLOC_HEADER *pLocOld )
{
	for (int iAchievement = 0; iAchievement < snNumWantUndirected[UND_WANT_INV_MOVE]; iAchievement++)
	{
		ACHIEVEMENT_DATA *pAchievement = spWantUndirected[UND_WANT_INV_MOVE][iAchievement];

		if (pAchievement->eAchievementType == ACHV_INVENTORY_FILL)
		{
			if (pLocNew->nLocation == INVLOC_STASH ||
				pLocNew->nLocation == INVLOC_BIGPACK)
			{
				if (UnitIsPlayer(pNewContainer))
				{
					if (UnitInventoryGridGetNumEmptySpaces(pNewContainer, INVLOC_STASH) == 0 &&
						UnitInventoryGridGetNumEmptySpaces(pNewContainer, INVLOC_BIGPACK) == 0)
					{
						s_AchievementProgressSet(pNewContainer, pAchievement, 1);
					}
				}
			}
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendPlayerLevelUp(
	UNIT *pPlayer)
{
	if (!pPlayer)
		return FALSE;

	ASSERT_RETFALSE(UnitIsPlayer(pPlayer));
	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	int nLevel = UnitGetStat(pPlayer, STATS_LEVEL);
	DWORD dwPlayedSeconds = UnitGetPlayedTime(pPlayer);
	unsigned int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);
	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{
		const ACHIEVEMENT_DATA * pAchievement = (ACHIEVEMENT_DATA *)ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
		if ( ! pAchievement )
			continue;

		if (pAchievement->eAchievementType == ACHV_LEVELING_TIME)
		{
			if (nLevel >= pAchievement->nCompleteNumber &&
				dwPlayedSeconds <= (DWORD)pAchievement->nParam1)
			{
				// pAchievement->nCompleteNumber is just a placeholder for the level number, mostly for the string
				//   Just set it to complete here.
				s_AchievementProgressSet(pPlayer, pAchievement, pAchievement->nCompleteNumber);
			}
		}
	}
	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendPlayerRankUp(
	UNIT *pPlayer)
{
	if (!pPlayer)
		return FALSE;

	ASSERT_RETFALSE(UnitIsPlayer(pPlayer));
	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	int nRank = UnitGetPlayerRank(pPlayer);
	DWORD dwPlayedSeconds = UnitGetPlayedTime(pPlayer);
	unsigned int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);
	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{
		const ACHIEVEMENT_DATA * pAchievement = (ACHIEVEMENT_DATA *)ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
		if ( ! pAchievement )
			continue;

		if (pAchievement->eAchievementType == ACHV_LEVELING_TIME)
		{
			if (nRank >= pAchievement->nCompleteNumber &&
				dwPlayedSeconds <= (DWORD)pAchievement->nParam1)
			{
				// pAchievement->nCompleteNumber is just a placeholder for the level number, mostly for the string
				//   Just set it to complete here.
				s_AchievementProgressSet(pPlayer, pAchievement, pAchievement->nCompleteNumber);
			}
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_AchievementStatchangeCallback(
	UNIT *pPlayer,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	ASSERT_RETURN(pPlayer);
	if (!UnitIsPlayer(pPlayer))
	{
		return;
	}
	
	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETURN(IS_SERVER(pGame));

	int nAchievement = (int)data.m_dwData1;
	ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, nAchievement);
	ASSERT_RETURN	(pAchievement);

	if (pAchievement->eAchievementType == ACHV_STAT_VALUE)
	{
		s_AchievementProgressSet(pPlayer, pAchievement, nNewValue);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendQuestComplete(
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
	UNIT *pPlayer)
{
	ASSERT_RETFALSE(pQuestTask);
	ASSERT_RETFALSE(pPlayer);	
	if (!UnitIsPlayer(pPlayer))
	{
		return FALSE;
	}

	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	unsigned int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);
	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{
		const ACHIEVEMENT_DATA * pData = (ACHIEVEMENT_DATA *)ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
		if ( ! pData )
			continue;

		if (pData->eAchievementType == ACHV_QUEST_COMPLETE)
		{
			if ((pData->nRandomQuests && !isQuestRandom(pQuestTask))) 
			{
				continue;
			}
			if(pData->nQuestTaskTugboat != pQuestTask->nTaskIndexIntoTable )
			{
				continue;
			}

			s_AchievementProgress(pPlayer, pData, 1);
		}
	}

	return TRUE;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendQuestComplete(
	int nQuest,
	UNIT *pPlayer)
{
	ASSERT_RETFALSE(pPlayer);
	if (!UnitIsPlayer(pPlayer))
	{
		return FALSE;
	}

	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	QUEST * pQuest = QuestGetQuest( pPlayer, nQuest );
	ASSERT_RETFALSE(pQuest);
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;

	unsigned int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);
	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{
		const ACHIEVEMENT_DATA * pData = (ACHIEVEMENT_DATA *)ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
		if ( ! pData )
			continue;

		if (pData->eAchievementType == ACHV_QUEST_COMPLETE)
		{
			if (pData->nParam1 == 1 &&
				pQuestDef->eStyle != QS_STORY)
			{
				continue;
			}
			s_AchievementProgress(pPlayer, pData, 1);
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendMinigameWin(
	UNIT *pPlayer)
{
	ASSERT_RETFALSE(pPlayer);
	if (!UnitIsPlayer(pPlayer))
	{
		return FALSE;
	}

	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	unsigned int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);
	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{
		const ACHIEVEMENT_DATA * pData = (ACHIEVEMENT_DATA *)ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
		if ( ! pData )
			continue;

		if (pData->eAchievementType == ACHV_MINIGAME_WIN)
		{
			s_AchievementProgress(pPlayer, pData, 1);
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendSkillLevelUp(
	UNIT *pPlayer,
	int nSkill)
{
	ASSERT_RETFALSE(pPlayer);
	if (!UnitIsPlayer(pPlayer))
	{
		return FALSE;
	}

	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	unsigned int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);
	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{
		const ACHIEVEMENT_DATA * pData = (ACHIEVEMENT_DATA *)ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
		if ( ! pData )
			continue;

		if (pData->eAchievementType == ACHV_SKILL_LEVEL)
		{
			if (pData->nSkill == INVALID_LINK ||
				pData->nSkill == nSkill)
			{
				int nSkillLevel = UnitGetStat(pPlayer, STATS_SKILL_LEVEL, nSkill);
				int nBefore = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pData->nIndex);
				if (nSkillLevel > nBefore)
				{
					s_AchievementProgressSet(pPlayer, pData, nSkillLevel);
				}
			}
		}

		if (pData->eAchievementType == ACHV_SKILL_TREE_COMPLETE)
		{
			int nSkillTab = pPlayer->pUnitData->nSkillTab[0];
			BOOL bOk = TRUE;
			unsigned int nRowCount = SkillsGetCount(pGame);
			for (unsigned int iRow = 0; iRow < nRowCount; iRow++)
			{
				const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, iRow);
				if (pSkillData &&
					pSkillData->nSkillTab == nSkillTab)
				{
					if (UnitGetStat(pPlayer, STATS_SKILL_LEVEL, iRow) <= 0)
					{
						bOk = FALSE;
						break;
					}
				}
			}

			if (bOk)
			{
				s_AchievementProgress(pPlayer, pData, 1);
			}
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_AchievementsSendLevelEnter(
	int nAchievement,
	UNIT *pPlayer,
	int nLevel)
{
	if (!pPlayer)
		return FALSE;

	if (!UnitIsPlayer(pPlayer))
	{
		return FALSE;
	}
	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	ASSERT_RETFALSE(nAchievement > INVALID_LINK);
	ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, nAchievement);
	ASSERT_RETFALSE	(pAchievement);

	if (pAchievement->eAchievementType == ACHV_LEVEL_FIND)
	{
		ASSERT_RETFALSE	(pAchievement->nLevel == nLevel);

		s_AchievementProgress(pPlayer, pAchievement, 1);
		return TRUE;
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddAchievementToDataCommon(
	int *&pnAchievements,
	USHORT &nNumAchievements,
	int nAchievement)
{
	ASSERT_RETURN(nAchievement > INVALID_LINK);

	if (nNumAchievements > 0)
	{
		ASSERT_RETURN(pnAchievements);
	}

	if (!pnAchievements)
	{
		pnAchievements = (int *)MALLOC(NULL, sizeof(int));
		ASSERT_RETURN(pnAchievements);
	}
	else
	{
		// make sure it's not already attached to an achievement
		if (nNumAchievements > 0)
		{
			for (int ii = 0; ii < nNumAchievements; ++ii)
			{
				if (pnAchievements[ii] == nAchievement)
					return;		// skip out
			}
		}

		pnAchievements = (int *)REALLOC(NULL, pnAchievements, sizeof(int) * (nNumAchievements + 1));
		ASSERT_RETURN(pnAchievements);
	}

	pnAchievements[nNumAchievements++] = nAchievement;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddAchievementToUnitData(
	UNIT_DATA *pUnitData,
	int nAchievement)
{
	ASSERT_RETURN(pUnitData);
	sAddAchievementToDataCommon(pUnitData->pnAchievements, pUnitData->nNumAchievements, nAchievement);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddAchievementToSkillData(
	SKILL_DATA *pSkillData,
	int nAchievement)
{
	ASSERT_RETURN(pSkillData);
	sAddAchievementToDataCommon(pSkillData->pnAchievements, pSkillData->nNumAchievements, nAchievement);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddAchievementToLevelData(
	LEVEL_DEFINITION *pLevelData,
	int nAchievement)
{
	ASSERT_RETURN(pLevelData);
	sAddAchievementToDataCommon(pLevelData->pnAchievements, pLevelData->nNumAchievements, nAchievement);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddAchievementToUndirected(
	int nUndirectedType,
	ACHIEVEMENT_DATA * pAchievementData)
{
	ASSERT_RETURN(pAchievementData);

	if (snNumWantUndirected[nUndirectedType] + 1 < arrsize(spWantUndirected[nUndirectedType]))
	{
		spWantUndirected[nUndirectedType][snNumWantUndirected[nUndirectedType]++] = pAchievementData;
	}
	else
	{
		WARN(snNumWantUndirected[nUndirectedType] + 1 < arrsize(spWantUndirected[nUndirectedType]));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AchievementsExcelPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	unsigned int nCount = ExcelGetCountPrivate(table);
	int nPlayerCount = ExcelGetCountPrivate(ExcelGetTableNotThreadSafe(DATATABLE_PLAYERS));
	int nMonsterCount = ExcelGetCountPrivate(ExcelGetTableNotThreadSafe(DATATABLE_MONSTERS));
	int nItemCount = ExcelGetCountPrivate(ExcelGetTableNotThreadSafe(DATATABLE_ITEMS));
	int nObjectCount = ExcelGetCountPrivate(ExcelGetTableNotThreadSafe(DATATABLE_OBJECTS));
//	int nSkillCount = ExcelGetCountPrivate(ExcelGetTableNotThreadSafe(DATATABLE_SKILLS));

	memclear(snNumWantUndirected, sizeof(int) * NUM_WANT_UNDIRECTED);

	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{
		ACHIEVEMENT_DATA * pData = (ACHIEVEMENT_DATA *)ExcelGetDataPrivate(table, iAchievement);
		if ( ! pData )
			continue;

		pData->nIndex = iAchievement;

		if( pData->eAchievementType == ACHV_KILL )
		{
			for (int iObject = 0; iObject < nObjectCount; ++iObject)
			{
				UNIT_DATA * pObjectData = (UNIT_DATA *)ExcelGetDataPrivate(ExcelGetTableNotThreadSafe(DATATABLE_OBJECTS), iObject);
				if ( !pObjectData )
					continue;
				if (iObject == pData->nObjectClass)
				{
					sAddAchievementToUnitData(pObjectData, iAchievement);
					continue;
				}
				if (IsAllowedUnit(pObjectData->nUnitType, pData->nUnitType, arrsize(pData->nUnitType)))
				{
					sAddAchievementToUnitData(pObjectData, iAchievement);
				}
			}
		
		}

		if (pData->eAchievementType == ACHV_KILL)
		{
			for (int iMonster = 0; iMonster < nMonsterCount; ++iMonster)
			{
				UNIT_DATA * pMonsterData = (UNIT_DATA *)ExcelGetDataPrivate(ExcelGetTableNotThreadSafe(DATATABLE_MONSTERS), iMonster);
				if ( ! pMonsterData )
					continue;

				if (iMonster == pData->nMonsterClass)
				{
					sAddAchievementToUnitData(pMonsterData, iAchievement);
					continue;
				}

				if (IsAllowedUnit(pMonsterData->nUnitType, pData->nUnitType, arrsize(pData->nUnitType)))
				{
					sAddAchievementToUnitData(pMonsterData, iAchievement);
				}
			}
		}
		//achievments for PVP
		if (pData->eAchievementType == ACHV_KILL)
		{
			for (int iPlayers = 0; iPlayers < nPlayerCount; ++iPlayers)
			{
				UNIT_DATA * pPlayerData = (UNIT_DATA *)ExcelGetDataPrivate(ExcelGetTableNotThreadSafe(DATATABLE_PLAYERS), iPlayers);
				if ( ! pPlayerData )
					continue;

				if (iPlayers == pData->nMonsterClass)
				{
					sAddAchievementToUnitData(pPlayerData, iAchievement);
					continue;
				}

				if (IsAllowedUnit(pPlayerData->nUnitType, pData->nUnitType, arrsize(pData->nUnitType)))
				{
					sAddAchievementToUnitData(pPlayerData, iAchievement);
				}
			}
		}

		if ((pData->eAchievementType == ACHV_SKILL_KILL ||
			 pData->eAchievementType == ACHV_SKILL_LEVEL) &&
			pData->nSkill != INVALID_LINK)
		{
			SKILL_DATA * pSkillData = (SKILL_DATA *)ExcelGetDataPrivate(ExcelGetTableNotThreadSafe(DATATABLE_SKILLS), pData->nSkill);
			if ( ! pSkillData )
				continue;

			sAddAchievementToSkillData(pSkillData, iAchievement);
		}

		if ((pData->eAchievementType == ACHV_LEVEL_FIND) &&
			pData->nLevel != INVALID_LINK)
		{
			LEVEL_DEFINITION * pLevelData = (LEVEL_DEFINITION *)ExcelGetDataPrivate(ExcelGetTableNotThreadSafe(DATATABLE_LEVEL), pData->nLevel);
			if ( ! pLevelData )
				continue;

			sAddAchievementToLevelData(pLevelData, iAchievement);
		}

		if (pData->eAchievementType == ACHV_COLLECT ||
			pData->eAchievementType == ACHV_EQUIP ||
			pData->eAchievementType == ACHV_WEAPON_KILL ||
			pData->eAchievementType == ACHV_ITEM_USE)
		{
			for (int iItem = 0; iItem < nItemCount; ++iItem)
			{
				UNIT_DATA * pItemData = (UNIT_DATA *)ExcelGetDataPrivate(ExcelGetTableNotThreadSafe(DATATABLE_ITEMS), iItem);
				if ( ! pItemData )
					continue;

				if (UnitIsA(pItemData->nUnitType, pData->nItemUnittype[0]))
				{
					sAddAchievementToUnitData(pItemData, iAchievement);
					continue;
				}

				if (IsAllowedUnit(pItemData->nUnitType, pData->nUnitType, arrsize(pData->nUnitType)))
				{
					sAddAchievementToUnitData(pItemData, iAchievement);
				}

			}
		}

		if( pData->eRevealCondition == REVEAL_ACHV_PARENT_COMPLETE )
		{
			ASSERTX_CONTINUE( pData->nRevealParentID != INVALID_ID, "Attempting to use reveal condition parent complete but parent isn't found" );
		}

		if (pData->eAchievementType == ACHV_TIMED_KILLS)
		{
			sAddAchievementToUndirected(UND_WANT_KILL, pData);
		}

		if (pData->eAchievementType == ACHV_INVENTORY_FILL)
		{
			sAddAchievementToUndirected(UND_WANT_INV_MOVE, pData);
		}

		if (pData->eAchievementType == ACHV_STAT_VALUE)
		{
			ASSERT_CONTINUE(pData->nStat != INVALID_LINK);
			
			STATS_CALLBACK_DATA tCallbackData;
			tCallbackData.m_dwData1 = iAchievement;
			StatsAddStatsChangeCallback(pData->nStat, s_AchievementStatchangeCallback, tCallbackData, TRUE, FALSE);

		}

	}

	return TRUE;
}

//this is such a ghettooooo method
BOOL x_AchievementIsCompleteBySkillID( 
	UNIT *pPlayer,
	int nSkillID )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( nSkillID != INVALID_ID );
	
	unsigned int nCount = ExcelGetCountPrivate(ExcelGetTableNotThreadSafe(DATATABLE_ACHIEVEMENTS));
	

	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{
		ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *)ExcelGetData( NULL, DATATABLE_ACHIEVEMENTS, iAchievement);
		if( pAchievement &&
			pAchievement->nRewardSkill == nSkillID )
		{
			return x_AchievementIsComplete( pPlayer, pAchievement );
		}
	}
	return FALSE;
}

//checks if the requirement is needed
inline void sAchievementRequirmentTest( 
	UNIT *pPlayer, 
	UNIT *pActivator,
	UNIT *pTarget, 
	UNIT * weapons[ MAX_WEAPONS_PER_UNIT ] )
{
	pActivator = ( pActivator == NULL )?pPlayer:pActivator;
	UNIT *pTargetOwner( ((!pTarget)?NULL:UnitGetUltimateOwner(pTarget)) );
	GAME *pGame = UnitGetGame( pPlayer );
	if (IS_SERVER(pGame))
	{
		// Check for achievements
		// -----------------------------------------------		
		// items are a bit different
		if( pTarget && UnitGetGenus( pTarget ) == GENUS_ITEM )
		{
			//we want to make sure we only count the item once.
			if( UnitGetStat( pTarget, STATS_ITEM_PICKEDUP ) == 0 )
			{
				for (int iAchievement = 0; iAchievement < pTarget->pUnitData->nNumAchievements; ++iAchievement)
				{
					s_AchievementsSendItemPickup( pTarget->pUnitData->pnAchievements[iAchievement], pTarget, pPlayer );
				}			
			}
			return;
		}

		//objects and monsters both can be killed...
		if (pTarget && pTarget->pUnitData && pTarget->pUnitData->pnAchievements)
		{			
			for (int iAchievement = 0; iAchievement < pTarget->pUnitData->nNumAchievements; ++iAchievement)
			{
				s_AchievementsSendKill(pTarget->pUnitData->pnAchievements[iAchievement], pActivator, pTarget, pPlayer, pTargetOwner, weapons);
			}
		}

		if ( weapons )
		{
			for (int iWeapon = 0; iWeapon < MAX_WEAPONS_PER_UNIT; ++iWeapon)
			{
				if (weapons[iWeapon] && weapons[iWeapon]->pUnitData && weapons[iWeapon]->pUnitData->pnAchievements)
				{
					for (int iAchievement = 0; iAchievement < weapons[iWeapon]->pUnitData->nNumAchievements; ++iAchievement)
					{
						s_AchievementsSendKill(weapons[iWeapon]->pUnitData->pnAchievements[iAchievement], pActivator, pTarget, pPlayer, pTargetOwner, weapons);
					}
				}
			}
		}

		int nSkills[20];
		SkillsGetSkillsOn(pPlayer, nSkills, arrsize(nSkills));
		for (int iSkill=0; iSkill < arrsize(nSkills); iSkill++)
		{
			if (nSkills[iSkill] == INVALID_LINK)
				break;

			const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkills[iSkill] );
		
			if (pSkillData && pSkillData->pnAchievements)
			{
				for (int iAchievement = 0; iAchievement < pSkillData->nNumAchievements; ++iAchievement)
				{
					s_AchievementsSendSkillKill(pSkillData->pnAchievements[iAchievement], pPlayer, pTarget, nSkills[iSkill]);
				}
			}

		}

		s_AchievementsSendKillUndirected(pPlayer, pTarget);

	}
}

//checks if the requirement is needed
void s_AchievementRequirmentTest( UNIT *pPlayer, 
	UNIT *pActivator,
	UNIT *pTarget, 
	UNIT * weapons[ MAX_WEAPONS_PER_UNIT ] )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( UnitIsA(pPlayer, UNITTYPE_PLAYER) );
	if( AppIsHellgate() )
	{
		sAchievementRequirmentTest( pPlayer, pActivator, pTarget, weapons );
		return;
	}
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	if( pLevel &&
		pTarget &&
		UnitGetGenus( pTarget ) == GENUS_MONSTER )
	{
		int nQuality = MonsterGetQuality( pTarget );
		if( nQuality > 0 )  //if a monster has better then normal quality the kill goes for the party
		{
			for (GAMECLIENT * client = ClientGetFirstInLevel(pLevel); client; client = ClientGetNextInLevel(client, pLevel))
			{
				UNIT * unit = ClientGetPlayerUnit(client);
				if (!unit ||
					(unit && UnitIsInLimbo( unit ) ))
				{
					continue;
				}
				//we use the same achievement for the weapons here because the weapon did the kill. Allow for 
				//groups to have a given person who must kill the boss.
				sAchievementRequirmentTest( unit, pActivator, pTarget, weapons );
			}
			return;			
		}
	}
	sAchievementRequirmentTest( pPlayer, pActivator, pTarget, weapons );
}


int AchievementGetSlotCount()
{ 
	return (int)ExcelGetCount( EXCEL_CONTEXT(), DATATABLE_ACHIEVEMENT_SLOTS ); 
}

//returns if the slot is a valid slot for the player
BOOL AchievementSlotIsValid( 
	UNIT *pPlayer, 
	int nSlotIndex, 
	int nSkill, 
	int nSkillLevel )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( UnitGetGenus( pPlayer ) == GENUS_PLAYER );
	ASSERT_RETFALSE( nSlotIndex >= 0 && nSlotIndex < AchievementGetSlotCount() );
	const ACHIEVEMENT_UNLOCK_PLAYER_DATA *pAchievementSlotData = (ACHIEVEMENT_UNLOCK_PLAYER_DATA *)ExcelGetData(NULL, DATATABLE_ACHIEVEMENT_SLOTS, nSlotIndex );
	ASSERT_RETFALSE( pAchievementSlotData );
	int nPlayerLevel = UnitGetExperienceLevel( pPlayer );
	if( pAchievementSlotData->nPlayerLevel >= 0 &&
		nPlayerLevel < pAchievementSlotData->nPlayerLevel )
	{
		return FALSE;	//not the correct level to allow slot
	}
	if( pAchievementSlotData->codeConditional != NULL_CODE )
	{
		int code_len( 0 );	
		BYTE * code_ptr = ExcelGetScriptCode( UnitGetGame( pPlayer ), DATATABLE_ACHIEVEMENT_SLOTS, pAchievementSlotData->codeConditional, &code_len);		
		if (code_ptr)
		{				
			if( VMExecI( UnitGetGame( pPlayer ), pPlayer, NULL, NULL, nSkill, nSkillLevel, nSkill, nSkillLevel, INVALID_ID, code_ptr, code_len) <= 0 )
				return FALSE;
		}		
	}
	return TRUE;
}

//returns the achievement based off of the achievement slot index
const ACHIEVEMENT_DATA * AchievementGetBySlotIndex( 
	UNIT *pPlayer, 
	int nSlotIndex )
{
	ASSERT_RETNULL( pPlayer );
	ASSERT_RETNULL( nSlotIndex >= 0 && nSlotIndex < AchievementGetSlotCount()  );
	int nAchievementID = UnitGetStat( pPlayer, STATS_ACHIEVEMENT_SLOTS, nSlotIndex ); //Get the achievement that was put in this slot
	if( nAchievementID != INVALID_ID )
	{
		const ACHIEVEMENT_DATA * pAchievementData = (ACHIEVEMENT_DATA *)ExcelGetData(NULL, DATATABLE_ACHIEVEMENTS, nAchievementID);
		ASSERT_RETNULL( pAchievementData ); //invalid id for achievement
		return pAchievementData;
	}
	return NULL;

}

//clears out the achievement for a given slot
void s_AchievementRemoveBySlot( 
	UNIT *pPlayer, 
	int nSlotIndex )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( UnitGetGenus( pPlayer ) == GENUS_PLAYER );
	ASSERT_RETURN( nSlotIndex >= 0 && nSlotIndex < AchievementGetSlotCount()  );		
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN( pGame );

	const ACHIEVEMENT_DATA * pAchievementData = AchievementGetBySlotIndex( pPlayer, nSlotIndex );	
	if( pAchievementData &&
		pAchievementData->nRewardSkill != INVALID_ID )
	{
		SkillDeselect( pGame, pPlayer, pAchievementData->nRewardSkill, TRUE ); //deselect the skill		
	}
	UnitSetStat( pPlayer, STATS_ACHIEVEMENT_SLOTS, nSlotIndex, INVALID_ID ); //remove it from the slot		
	s_AchievementSetStats( pPlayer );
	//we are all clear
}


//clears out the achievement in any slots
void s_AchievementRemoveByID( 
	UNIT *pPlayer, 
	int nAchievementID )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( UnitGetGenus( pPlayer ) == GENUS_PLAYER );
	ASSERT_RETURN( nAchievementID != INVALID_ID );		
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN( pGame );
	int nSlots = AchievementGetSlotCount();

	for( int i = 0; i < nSlots; i++ )
	{
		int nCurrentID = UnitGetStat( pPlayer, STATS_ACHIEVEMENT_SLOTS, i ); //set the stat on the achievement	
		if( nCurrentID == nAchievementID )
		{
			const ACHIEVEMENT_DATA * pAchievementData = AchievementGetBySlotIndex( pPlayer, i );	
			if( pAchievementData &&
				pAchievementData->nRewardSkill != INVALID_ID )
			{
				SkillDeselect( pGame, pPlayer, pAchievementData->nRewardSkill, TRUE ); //deselect the skill	
				UnitSetStat( pPlayer, STATS_ACHIEVEMENT_SLOTS, i, INVALID_ID ); //remove it from the slot		
				s_AchievementSetStats( pPlayer );
			}
		}
	}

	//we are all clear
}

//adds and sends messages for selecting achievements for the player ( client and server ) - pass INVALID_ID for achievementID to remove it
BOOL x_AchievementPlayerSelect( 
	UNIT *pPlayer, 
	int nAchievementID, 
	int nSlotIndex )
{
	
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( UnitGetGenus( pPlayer ) == GENUS_PLAYER );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERT_RETFALSE( pGame );
	ASSERT_RETFALSE( nSlotIndex >= 0 && nSlotIndex < AchievementGetSlotCount()  );		
	
	if( IS_CLIENT(pGame ) )
	{
#if !ISVERSION(SERVER_VERSION)
		c_SendPlayerSelectAchievment( nAchievementID, nSlotIndex );
#endif
		return TRUE;
	}
	
	if( IS_SERVER(pGame) )
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)
		if( !AchievementSlotIsValid( pPlayer, nSlotIndex ) )
		{
			return FALSE;
		}

		s_AchievementRemoveBySlot( pPlayer, nSlotIndex ); //remove the old one
		if( nAchievementID != INVALID_ID )
		{
			s_AchievementRemoveByID( pPlayer, nAchievementID ); //remove this one from any other slots

			const ACHIEVEMENT_DATA *pAchievementData = (ACHIEVEMENT_DATA *)ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, nAchievementID);
			if( pAchievementData  &&
				s_AchievementCheckComplete(pPlayer, pAchievementData))	//set the data only if we are selecting an achievement
			{			
				UnitSetStat( pPlayer, STATS_ACHIEVEMENT_SLOTS, nSlotIndex, nAchievementID ); //set the stat on the achievement		

				if( pAchievementData->nRewardSkill != INVALID_ID)
				{
					SkillSelect( pGame, pPlayer, pAchievementData->nRewardSkill, TRUE ); //set the skill active by force			
				}
				s_AchievementSetStats( pPlayer );		
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			UnitSetStat( pPlayer, STATS_ACHIEVEMENT_SLOTS, nSlotIndex, INVALID_ID ); //set the stat on the achievement		
		}
#endif
	}
	return TRUE;
}


void s_AchievementsInitialize( 
	UNIT *pPlayer )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( IS_SERVER( UnitGetGame( pPlayer ) ) );
	int nAchievementSlotCount = AchievementGetSlotCount();	
	for( int t = 0; t < nAchievementSlotCount; t++ )
	{		
		int nAchievementID = UnitGetStat( pPlayer, STATS_ACHIEVEMENT_SLOTS, t );		
		if( nAchievementID != INVALID_ID )
		{			
			if( AchievementSlotIsValid( pPlayer, t ) )
			{
				x_AchievementPlayerSelect( pPlayer, nAchievementID, t );
			}
			else
			{
				x_AchievementPlayerSelect( pPlayer, INVALID_ID, t ); //it'll clear the achievement out
			}
		}
	}	
}

//achievements can have just simple stat mods this handles those.
void s_AchievementSetStats( 
	UNIT *pPlayer )
{	
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( IS_SERVER( UnitGetGame( pPlayer ) ) );
	StateClearAllOfType( pPlayer, STATE_ACHIEVEMENT_STATS );
	STATS *stats = s_StateSet( pPlayer, NULL, STATE_ACHIEVEMENT_STATS, 0 );	
	if( stats == NULL ) //this is a work around for an old bug
		stats = s_StateSet( pPlayer, NULL, STATE_ACHIEVEMENT_STATS, 0 );
	ASSERT_RETURN( stats );
	int nAchievementSlotCount = AchievementGetSlotCount();
	for( int t = 0; t < nAchievementSlotCount; t++ )
	{
		const ACHIEVEMENT_DATA *pAchievementData = AchievementGetBySlotIndex( pPlayer, t );
		if( pAchievementData &&
			pAchievementData->codeRewardScript != NULL_CODE )
		{
			int code_len( 0 );	
			BYTE * code_ptr = ExcelGetScriptCode( UnitGetGame( pPlayer ), DATATABLE_ACHIEVEMENTS, pAchievementData->codeRewardScript, &code_len);		
			if (code_ptr)
			{				
				int nSkill = pAchievementData->nRewardSkill;
				int nSkillLevel( (nSkill != INVALID_ID)?1:0 );
				VMExecI( UnitGetGame( pPlayer ), pPlayer, NULL, stats, nSkill, nSkillLevel, nSkill, nSkillLevel, STATE_ACHIEVEMENT_STATS, code_ptr, code_len);
			}
		}		
	}

}