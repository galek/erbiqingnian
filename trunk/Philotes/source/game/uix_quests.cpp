//----------------------------------------------------------------------------
// uix_quests.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "uix.h"
#include "uix_graphic.h"
#include "uix_priv.h"
#include "uix_components.h"
#include "uix_graphic.h"
#include "uix_scheduler.h"
#include "uix_menus.h"
#include "uix_quests.h"
#include "quests.h"
#include "c_quests.h"
#include "c_message.h"
#include "fontcolor.h"
#include "dialog.h"
#include "c_message.h"
#include "Quest.h"
#include "quest_testhope.h"
#include "quest_tutorial.h"

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdateDialogNextPrevBtns(
	UI_COMPONENT *pComponent)
{
	UI_LABELEX *pLabel = UICastToLabel(pComponent);
	UI_COMPONENT *pChild = UIComponentFindChildByName(pLabel->m_pParent, "dialog prev");
	if (pChild)
	{
		UIComponentSetActive(pChild, (pLabel->m_nPage > 0));
	}
	pChild = UIComponentFindChildByName(pLabel->m_pParent, "dialog next");
	if (pChild)
	{
		UIComponentSetActive(pChild, (pLabel->m_nPage < pLabel->m_nNumPages-1));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdateQuestSteps(
	int nQuest)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_QUEST_PANEL);
	ASSERT_RETURN(pComponent);
	UI_QUEST_PANEL *pQuestPanel = UICastToQuestPanel(pComponent);

	UI_COMPONENT *pHolderPanel = UIComponentFindChildByName(pComponent, "quest steps list");
	ASSERT_RETURN(pHolderPanel);

	// go through logs
	UNIT *pPlayer = UIComponentGetFocusUnit(pComponent);

	UI_COMPONENT *pChild = UIComponentIterateChildren(pHolderPanel, NULL, UITYPE_LABEL, FALSE);
	int iState = 0;
	pQuestPanel->m_nNumSteps = 0;
	int nNumStepComponentsVisible = 0;
	int nOffset = pQuestPanel->m_nStepsScrollPos;
	BOOL bBeyondLimits = FALSE;

	if (nQuest != INVALID_ID)
	{

		QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );	
		if (!pQuest)
			return;
		
		// do nothing if not visible in log
		if (c_QuestIsVisibleInLog( pQuest ))
		{

			WCHAR szTemp[ MAX_QUESTLOG_STRLEN ];

			// go through each of the states
			while (iState < pQuest->nNumStates)
			{
				QUEST_STATE *pState = QuestStateGetByIndex( pQuest, iState );

				// we display only active or completed entries
				QUEST_STATE_VALUE eStateValue = QuestStateGetValue( pQuest, pState );
				if (eStateValue == QUEST_STATE_ACTIVE || eStateValue == QUEST_STATE_COMPLETE)
				{
					pQuestPanel->m_nNumSteps++;

					if (nOffset > 0)
					{
						nOffset--;
					}
					else if (pChild)
					{
						int nColor;
						WCHAR szQuestStateLogBuffer[ MAX_QUESTLOG_STRLEN ];		
						const WCHAR *puszLogText = c_QuestStateGetLogText( pPlayer, pQuest, pState, &nColor );
						// skip states that don't have any log strings
						if (puszLogText != NULL)
						{
							c_QuestReplaceStringTokens(
								pPlayer,
								pQuest,
								puszLogText,
								szQuestStateLogBuffer,
								MAX_QUESTLOG_STRLEN);

							// set the text
							szTemp[0] = L'\0';
							UIColorCatString(szTemp, MAX_QUESTLOG_STRLEN, (FONTCOLOR)nColor, szQuestStateLogBuffer);

							UILabelSetText(pChild, szTemp);
							UIComponentSetActive(pChild, TRUE);
							pChild->m_dwData = (DWORD)iState;

							//move the next components down
							if (pChild->m_pNextSibling)
							{
								pChild->m_pNextSibling->m_Position.m_fY = pChild->m_Position.m_fY + pChild->m_fHeight + 
									((float)pHolderPanel->m_dwParam/* * UIGetLogToScreenRatioY()*/);
							}
							if (pChild->m_Position.m_fY + pChild->m_fHeight > pHolderPanel->m_fHeight)
							{
								bBeyondLimits = TRUE;
							}

							const QUEST_STATE_DEFINITION *ptStateDef = QuestStateGetDefinition( pState->nQuestState );
							UI_COMPONENT *pBtn = UIComponentFindChildByName(pChild, "quest text btn");
							if (pBtn)
							{
								pBtn->m_dwData = (DWORD)pState->nQuestState;
								UIComponentSetActive(pBtn, (ptStateDef->nDialogForState != INVALID_LINK || pQuest->pQuestDef->nDescriptionDialog != INVALID_LINK));
							}

							pBtn = UIComponentFindChildByName(pChild, "quest ask btn");
							if (pBtn)
							{
								pBtn->m_dwData = (DWORD)pState->nQuestState;
#if HELLGATE_ONLY				
								UIComponentSetActive(pBtn, (ptStateDef->nMurmurString != INVALID_LINK) && !QuestIsMurmurDead(pPlayer));
#endif
							}

							if (!bBeyondLimits)
								nNumStepComponentsVisible++;

							pChild = UIComponentIterateChildren(pHolderPanel, pChild, UITYPE_LABEL, FALSE);
						}
					}
				}

				iState++;
			}
		}
	}

	pQuestPanel->m_nMaxStepsScrollPos = MAX(0, pQuestPanel->m_nNumSteps - nNumStepComponentsVisible);
	pQuestPanel->m_nStepsScrollPos = MIN(pQuestPanel->m_nStepsScrollPos, pQuestPanel->m_nMaxStepsScrollPos);
	UI_COMPONENT *pBtn = UIComponentFindChildByName(pQuestPanel, "quest steps scroll up btn");
	if (pBtn)
		UIComponentSetActive(pBtn, pQuestPanel->m_nStepsScrollPos > 0);
	pBtn = UIComponentFindChildByName(pQuestPanel, "quest steps scroll down btn");
	if (pBtn)
		UIComponentSetActive(pBtn, (pQuestPanel->m_nStepsScrollPos < pQuestPanel->m_nMaxStepsScrollPos || bBeyondLimits));

	// Set the rest of the panels invisible
	while (pChild)
	{
		UIComponentSetVisible(pChild, FALSE);
		pChild->m_dwData = (DWORD)INVALID_ID;
		pChild = UIComponentIterateChildren(pHolderPanel, pChild, UITYPE_LABEL, FALSE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestOnSelect(
	int nQuest)
{
	if (c_QuestIsAllInitialQuestStateReceived() == FALSE)
		return;

	UI_COMPONENT *pQuestText = UIComponentGetByEnum(UICOMP_QUEST_PANEL_DIALOG);
	ASSERT_RETURN(pQuestText);
	UILabelSetText(pQuestText, L"");
	sUpdateDialogNextPrevBtns(pQuestText);

	UI_COMPONENT *pPortrait = UIComponentGetByEnum(UICOMP_QUEST_PANEL_PORTRAIT);
	ASSERT_RETURN(pPortrait);
	UIComponentSetFocusUnit(pPortrait, INVALID_ID);

	sUpdateQuestSteps(nQuest);
	UI_COMPONENT *pAbandon = UIComponentGetByEnum(UICOMP_QUEST_PANEL_ABANDON);
	if (pAbandon)
	{
		UIComponentSetVisible(pAbandon, TRUE);
		UIComponentSetActive(pAbandon, QuestCanAbandon(nQuest));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdateQuests(
	UI_COMPONENT* pComponent,
	BOOL bInitial = FALSE)
{
	UI_QUEST_PANEL *pQuestPanel = UICastToQuestPanel(pComponent);

	// go through logs
	UNIT *pPlayer = UIComponentGetFocusUnit(pComponent);

	WCHAR szLogBuffer[ MAX_QUESTLOG_STRLEN ];
	UI_COMPONENT *pHolderPanel = UIComponentFindChildByName(pComponent, "quests list");
	ASSERT_RETURN(pHolderPanel);

	// if the currently selected quest has been removed
	if (pQuestPanel->m_nSelectedQuest != INVALID_ID)
	{
		QUEST *pSelectedQuest = QuestGetQuest( pPlayer, pQuestPanel->m_nSelectedQuest );
		// do nothing if not visible in log
		if (!pSelectedQuest || !c_QuestIsVisibleInLog( pSelectedQuest ))
		{
			pQuestPanel->m_nSelectedQuest = INVALID_ID;
		}
	}

	UI_COMPONENT *pChild = UIComponentIterateChildren(pHolderPanel, NULL, UITYPE_LABEL, FALSE);
	int nQuest = 0;
	int nNumListComponents = 0;
	pQuestPanel->m_nNumQuests = 0;
	int nOffset = pQuestPanel->m_nQuestScrollPos;
	while (nQuest < NUM_QUESTS)
	{
		// int log buffer
		szLogBuffer[ 0 ] = 0;
		
		QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
		
		// do nothing if not visible in log
		if (pQuest && c_QuestIsVisibleInLog( pQuest ))
		{
			if (pQuestPanel->m_nSelectedQuest == INVALID_ID)
			{
				pQuestPanel->m_nSelectedQuest = nQuest;
				sQuestOnSelect(nQuest);
			}

			pQuestPanel->m_nNumQuests++;

			if (nOffset > 0)
			{
				nOffset--;
			}
			else if (pChild)
			{
				// set quest name
				const WCHAR *puszQuestName = StringTableGetStringByIndex( pQuest->nStringKeyName );

				// set the text
				UILabelSetText( pChild, puszQuestName );
				pChild->m_dwData = (DWORD)nQuest;
				if (nQuest == pQuestPanel->m_nSelectedQuest)
					UILabelSelect(pChild, 0, 0, 0);
					
				UIComponentSetActive(pChild, TRUE);			
				UI_COMPONENT *pOtherChild = UIComponentIterateChildren(pChild, NULL, UITYPE_BUTTON, FALSE);
				if (pOtherChild)
				{
					UI_BUTTONEX *pButton = UICastToButton(pOtherChild);
					int nDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT);
					UIButtonSetDown(pButton, UnitGetStat(pPlayer, STATS_QUEST_PLAYER_TRACKING, nQuest, nDifficulty));
					UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
				}

				nNumListComponents++;
				pChild = UIComponentIterateChildren(pHolderPanel, pChild, UITYPE_LABEL, FALSE);
			}
		}

		nQuest++;
	}

	if (pQuestPanel->m_nSelectedQuest == INVALID_ID)
	{
		UI_COMPONENT *pAbandon = UIComponentGetByEnum(UICOMP_QUEST_PANEL_ABANDON);
		if (pAbandon)
		{
			UIComponentSetVisible(pAbandon, TRUE);
			UIComponentSetActive(pAbandon, FALSE);
		}
	}

	pQuestPanel->m_nMaxQuestScrollPos = MAX(0, pQuestPanel->m_nNumQuests - nNumListComponents);
	pQuestPanel->m_nQuestScrollPos = MIN(pQuestPanel->m_nQuestScrollPos, pQuestPanel->m_nMaxQuestScrollPos);
	UI_COMPONENT *pBtn = UIComponentFindChildByName(pQuestPanel, "quest list scroll up btn");
	if (pBtn)
		UIComponentSetActive(pBtn, pQuestPanel->m_nQuestScrollPos > 0);
	pBtn = UIComponentFindChildByName(pQuestPanel, "quest list scroll down btn");
	if (pBtn)
		UIComponentSetActive(pBtn, pQuestPanel->m_nQuestScrollPos < pQuestPanel->m_nMaxQuestScrollPos);

	// Set the rest of the panels invisible
	while (pChild)
	{
		UIComponentSetVisible(pChild, FALSE);
		pChild = UIComponentIterateChildren(pHolderPanel, pChild, UITYPE_LABEL, FALSE);
	}

	sUpdateQuestSteps(pQuestPanel->m_nSelectedQuest);			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIUpdateQuestPanel(
	void)
{
	UI_COMPONENT *pQuestPanel = UIComponentGetByEnum(UICOMP_QUEST_PANEL);
	if (!pQuestPanel)
		return;

	if (UIComponentGetVisible(pQuestPanel))
		sUpdateQuests(pQuestPanel);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestPanelOnActivate(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	// do nothing if we havne't received all the setup from the server yet
	if (c_QuestIsAllInitialQuestStateReceived() == FALSE)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_COMPONENT *pQuestText = UIComponentGetByEnum(UICOMP_QUEST_PANEL_DIALOG);
	ASSERT_RETVAL(pQuestText, UIMSG_RET_NOT_HANDLED);
	UILabelSetText(pQuestText, L"");

	UI_COMPONENT *pPortrait = UIComponentGetByEnum(UICOMP_QUEST_PANEL_PORTRAIT);
	ASSERT_RETVAL(pPortrait, UIMSG_RET_NOT_HANDLED);
	UIComponentSetFocusUnit(pPortrait, INVALID_ID);

	sUpdateQuests(pComponent, TRUE);

	return UIComponentOnActivate(pComponent, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestPanelOnPostActivate(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIStopAllSkillRequests();
	c_PlayerClearAllMovement(AppGetCltGame());

/*	UI_COMPONENT *pCloseButton = UIComponentGetById(UIComponentGetIdByName("quest panel close btn"));
	if (pCloseButton)
	{
		UNIT *pPlayer = UIComponentGetFocusUnit(pComponent);
		UIComponentSetActive(pCloseButton, c_TutorialInputOk(pPlayer, TUTORIAL_INPUT_QUESTLOG_CLOSE));
		UIComponentHandleUIMessage(pCloseButton, UIMSG_PAINT, 0, 0);
	}*/

	UI_COMPONENT *pAutoTrackButton = UIComponentGetById(UIComponentGetIdByName("auto track quests btn"));
	if (pAutoTrackButton)
	{
		UNIT *pPlayer = UIComponentGetFocusUnit(pComponent);
		ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);

		UI_BUTTONEX *pButton = UICastToButton(pAutoTrackButton);
		UIButtonSetDown(pButton, !UnitGetStat(pPlayer, STATS_DONT_AUTO_TRACK_QUESTS));
		UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestPanelOnPostInactivate(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (AppIsHellgate() && pComponent)
	{
		UNIT *pPlayer = UIComponentGetFocusUnit(pComponent);
		if ( pPlayer )
			c_TutorialInputOk( pPlayer, TUTORIAL_INPUT_QUESTLOG_CLOSE );
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestListLabelOnClick(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	int nQuest = (int)pComponent->m_dwData;

	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_QUEST_PANEL);
	if (!pPanel)
		return UIMSG_RET_NOT_HANDLED;

	UI_QUEST_PANEL *pQuestPanel = UICastToQuestPanel(pPanel);

	pQuestPanel->m_nSelectedQuest = nQuest;
	if (nQuest != INVALID_ID)
	{
		UILabelSelect(pComponent, msg, wParam, lParam);

		sQuestOnSelect(nQuest);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
// Reset the quest list selection, so that it will be updated next time the
// quest list is updated. -cmarch
//----------------------------------------------------------------------------
void UIResetQuestPanel(void)
{
	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_QUEST_PANEL);
	if (!pPanel)
		return;

	UI_QUEST_PANEL *pQuestPanel = UICastToQuestPanel(pPanel);
	if (!pQuestPanel)
		return;

	pQuestPanel->m_nSelectedQuest = INVALID_ID;
	return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestListTrackOnClick(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pComponent->m_pParent, UIMSG_RET_NOT_HANDLED);

	int nQuest = (int)pComponent->m_pParent->m_dwData;

	if (nQuest != INVALID_ID)
	{
		UNIT *pPlayer = UIComponentGetFocusUnit(pComponent);
		QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );

		if (!pQuest)
			return UIMSG_RET_NOT_HANDLED;
		
		UI_BUTTONEX *pButton = UICastToButton(pComponent);
		if (UIButtonGetDown(pButton))
		{
			c_QuestTrack(pQuest, TRUE);
			//UIUpdateQuestLog(QUEST_LOG_UI_VISIBLE);
		}
		else
		{
			c_QuestTrack(pQuest, FALSE);
			//UIUpdateQuestLog(QUEST_LOG_UI_UPDATE);
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestPanelOnCloseBtnClick(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_QUEST_PANEL), UIMSG_INACTIVATE, 0, 0);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestStepAskOnClick(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	int nQuestState = (int)pComponent->m_dwData;
	if (nQuestState != INVALID_LINK)
	{
		const QUEST_STATE_DEFINITION *pStateDef = QuestStateGetDefinition( nQuestState );		
		ASSERT_RETVAL(pStateDef, UIMSG_RET_NOT_HANDLED);

		UI_COMPONENT *pQuestText = UIComponentGetByEnum(UICOMP_QUEST_PANEL_DIALOG);
		ASSERT_RETVAL(pQuestText, UIMSG_RET_NOT_HANDLED);

		UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_QUEST_PANEL);
		ASSERT_RETVAL(pPanel, UIMSG_RET_NOT_HANDLED);
		UI_QUEST_PANEL *pQuestPanel = UICastToQuestPanel(pPanel);

		UNIT *pPlayer = UIComponentGetFocusUnit(pComponent);
		GENDER eGender = UnitGetGender( pPlayer );
		DWORD dwAttributes = StringTableGetGenderAttribute( eGender );		
	    WCHAR uszDialog[ MAX_STRING_ENTRY_LENGTH ] = { 0 };

		ASSERTX_RETVAL(pQuestPanel->m_nSelectedQuest != INVALID_LINK, UIMSG_RET_NOT_HANDLED, "Expected selected quest");
		QUEST *pQuest = QuestGetQuest(pPlayer, pQuestPanel->m_nSelectedQuest, FALSE);
		if (pQuest)
		{
			int nStringIndex = pStateDef->nMurmurString;
			if ( pQuest->pQuestDef && ( pQuest->pQuestDef->nLogOverrideState == nQuestState ) && ( pQuest->pQuestDef->nLogOverrideString != INVALID_LINK ) )
			{
				nStringIndex = pQuest->pQuestDef->nLogOverrideString;
			}

			const WCHAR *puszQuestText = StringTableGetStringByIndexVerbose(nStringIndex, dwAttributes, 0, NULL, NULL);		
			
			c_QuestReplaceStringTokens( 
				pPlayer,
				pQuest,
				puszQuestText,
				uszDialog, 
				arrsize( uszDialog ) );
				
			UIDoNPCDialogReplacements(pPlayer, uszDialog, MAX_STRING_ENTRY_LENGTH, NULL);
			
		}

		UILabelSetText(pQuestText, uszDialog);
		sUpdateDialogNextPrevBtns(pQuestText);

		UI_COMPONENT *pPortrait = UIComponentGetByEnum(UICOMP_QUEST_PANEL_PORTRAIT);
		ASSERT_RETVAL(pPortrait, UIMSG_RET_NOT_HANDLED);

		pQuestPanel->m_bMurmurText = TRUE;
		if (pQuestPanel->m_idMurmur == INVALID_ID)
		{
			if (pQuestPanel->m_nMurmurClass == INVALID_LINK)
			{
				pQuestPanel->m_nMurmurClass = GlobalIndexGet(GI_MONSTER_MURMUR);
			}
			pQuestPanel->m_idMurmur = UICreateClientOnlyUnit(AppGetCltGame(), pQuestPanel->m_nMurmurClass);
		}
		UIComponentSetFocusUnit(pPortrait, pQuestPanel->m_idMurmur);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestStepTextOnClick(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	int nQuestState = (int)pComponent->m_dwData;
	if (nQuestState != INVALID_LINK)
	{
		const QUEST_STATE_DEFINITION *pStateDef = QuestStateGetDefinition( nQuestState );		
		ASSERT_RETVAL(pStateDef, UIMSG_RET_NOT_HANDLED);

		UI_COMPONENT *pQuestText = UIComponentGetByEnum(UICOMP_QUEST_PANEL_DIALOG);
		ASSERT_RETVAL(pQuestText, UIMSG_RET_NOT_HANDLED);

		UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_QUEST_PANEL);
		ASSERT_RETVAL(pPanel, UIMSG_RET_NOT_HANDLED);
		UI_QUEST_PANEL *pQuestPanel = UICastToQuestPanel(pPanel);

		UNIT *pPlayer = UIComponentGetFocusUnit(pComponent);
	    WCHAR uszDialog[ MAX_STRING_ENTRY_LENGTH ] = { 0 };

		ASSERTX_RETVAL(pQuestPanel->m_nSelectedQuest != INVALID_LINK, UIMSG_RET_NOT_HANDLED, "Expected selected quest");
		QUEST *pQuest = QuestGetQuest(pPlayer, pQuestPanel->m_nSelectedQuest, FALSE);
		if (pQuest)
		{
			const WCHAR *puszQuestText = 
				c_DialogTranslate(
					(pStateDef->nDialogForState != INVALID_LINK) ? 
					pStateDef->nDialogForState : 
					pQuest->pQuestDef->nDescriptionDialog);

			c_QuestReplaceStringTokens( 
				pPlayer,
				pQuest,
				puszQuestText,
				uszDialog, 
				arrsize( uszDialog ) );
		
			UIDoNPCDialogReplacements(pPlayer, uszDialog, MAX_STRING_ENTRY_LENGTH, NULL);
		}
		
			
		UILabelSetText(pQuestText, uszDialog);

		//UILabelSetText(pQuestText, L"Page 1...[pagebreak]Page 2...[pagebreak]Page 3");
		sUpdateDialogNextPrevBtns(pQuestText);

		UI_COMPONENT *pPortrait = UIComponentGetByEnum(UICOMP_QUEST_PANEL_PORTRAIT);
		ASSERT_RETVAL(pPortrait, UIMSG_RET_NOT_HANDLED);

		pQuestPanel->m_bMurmurText = FALSE;
		for (int i=0; i < MAX_CONVERSATION_SPEAKERS; i++)
		{
			int nNPCForState = pStateDef->nNPCForState[i];

			// if no NPC was specified for the main speaker, try to use the 
			// quest giver
			if (i == 0 && nNPCForState == INVALID_LINK && pQuest && pQuest->pQuestDef)
			{
				int nCastGiver = pQuest->pQuestDef->nCastGiver;
				if (nCastGiver != INVALID_LINK)
				{
					SPECIES spGiver = QuestCastGetSpecies( pQuest, nCastGiver );
					nNPCForState = GET_SPECIES_CLASS(spGiver);
				}
			}

			if (nNPCForState != pQuestPanel->m_nQuestStepPortraitUnitClass[i])
			{
				// free the previous one
				if ( pQuestPanel->m_idQuestStepPortraitUnit[i] != INVALID_ID )
				{
					UNIT *pUnit = UnitGetById(AppGetCltGame(), pQuestPanel->m_idQuestStepPortraitUnit[i]);
					if (pUnit &&
						UnitTestFlag(pUnit, UNITFLAG_CLIENT_ONLY))
					{
						UnitFree( pUnit );
						pQuestPanel->m_idQuestStepPortraitUnit[i] = INVALID_ID;
					}
				}

				if (nNPCForState != INVALID_LINK)
				{
					pQuestPanel->m_nQuestStepPortraitUnitClass[i] = nNPCForState;
					pQuestPanel->m_idQuestStepPortraitUnit[i] = UICreateClientOnlyUnit(AppGetCltGame(), nNPCForState);
				}
			}
		}

		UIComponentSetFocusUnit(pPortrait, pQuestPanel->m_idQuestStepPortraitUnit[0]);

	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestPanelDialogChangePage(
	UI_COMPONENT* component,
	int nDelta)
{
	ASSERT_RETURN(component);
	UI_LABELEX *pLabel = UICastToLabel(component);
	if (pLabel && 
		pLabel->m_nPage + nDelta >= 0 && 
		pLabel->m_nPage + nDelta <= pLabel->m_nNumPages - 1)
	{
	
		// a click on the button will try to reveal all the text
		if (UILabelDoRevealAllText( pLabel ) == TRUE)
		{
			return;
		}
		
		UILabelOnAdvancePage(pLabel, nDelta);

		BOOL bVideo = FALSE;
		int nSpeaker = UIGetSpeakerNumFromText(UILabelGetText(pLabel), pLabel->m_nPage, bVideo);
		ASSERTX_RETURN(nSpeaker >= 0 && nSpeaker < 2, "Unexpected speaker found" );

		UI_COMPONENT *pPortrait = UIComponentGetByEnum(UICOMP_QUEST_PANEL_PORTRAIT);
		ASSERT_RETURN(pPortrait);

		UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_QUEST_PANEL);
		ASSERT_RETURN(pPanel);
		UI_QUEST_PANEL *pQuestPanel = UICastToQuestPanel(pPanel);

		UNITID idFocus = ( pQuestPanel->m_bMurmurText ? pQuestPanel->m_idMurmur : pQuestPanel->m_idQuestStepPortraitUnit[nSpeaker] ); 
		
		UNIT *pFocus = UIComponentGetFocusUnit(pPortrait);
		if (!pFocus	|| 
			UnitGetId(pFocus) != idFocus)
		{
			UIComponentSetFocusUnit(pPortrait, idFocus);
		}

		sUpdateDialogNextPrevBtns(pLabel);
	}
					
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestPanelDialogOnPagePrev(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (UIComponentGetActive(component))
	{
		sQuestPanelDialogChangePage(UIComponentGetByEnum(UICOMP_QUEST_PANEL_DIALOG), -1);
						
		return UIMSG_RET_HANDLED_END_PROCESS;  
	}
	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestPanelDialogOnPageNext(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (UIComponentGetActive(component))
	{
		sQuestPanelDialogChangePage(UIComponentGetByEnum(UICOMP_QUEST_PANEL_DIALOG), 1);
						
		return UIMSG_RET_HANDLED_END_PROCESS;  
	}
	return UIMSG_RET_NOT_HANDLED;  
}

static void sQuestStuffScrollBase(
	BOOL bQuestList,
	int nDelta)
{
	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_QUEST_PANEL);
	if (!pPanel)
		return;

	UI_QUEST_PANEL *pQuestPanel = UICastToQuestPanel(pPanel);

	int &nScrollPos = (bQuestList ? pQuestPanel->m_nQuestScrollPos : pQuestPanel->m_nStepsScrollPos);
	int &nScrollMax = (bQuestList ? pQuestPanel->m_nMaxQuestScrollPos : pQuestPanel->m_nMaxStepsScrollPos);

	if (nScrollPos + nDelta <= nScrollMax &&
		nScrollPos + nDelta >= 0)
	{

		nScrollPos += nDelta;

		if (bQuestList)
			sUpdateQuests(pQuestPanel);
		else
			sUpdateQuestSteps(pQuestPanel->m_nSelectedQuest);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestListOnScrollDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	sQuestStuffScrollBase(TRUE, 1);

	return UIMSG_RET_HANDLED_END_PROCESS;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestListOnScrollUp(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	sQuestStuffScrollBase(TRUE, -1);

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestStepsOnScrollUp(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	sQuestStuffScrollBase(FALSE, -1);

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestStepsOnScrollDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	sQuestStuffScrollBase(FALSE, 1);

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestListOnMouseWheel(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentCheckBounds(component) || !UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	sQuestStuffScrollBase(TRUE, ((int)lParam > 0 ? -1 : 1));
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestStepsOnMouseWheel(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentCheckBounds(component) || !UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	sQuestStuffScrollBase(FALSE, ((int)lParam > 0 ? -1 : 1));
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sAbandonStoryQuestCallbackOk( void *, DWORD dwData)
{
	int nSelectedQuest = (int)dwData;
	AppPause(FALSE);
	AppUserPause(FALSE);

	UIHideGameMenu();
	UIGameRestart();

	c_SendQuestAbandon(nSelectedQuest);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sAbandonQuestCallbackOk( void *, DWORD dwData)
{

	int nSelectedQuest = (int)dwData;
	if ( c_QuestGetStyle( AppGetCltGame(), nSelectedQuest ) == QS_STORY )
	{
		//pop up another confirmation dialog
			
		DIALOG_CALLBACK tOkCallback;
		tOkCallback.dwCallbackData = (DWORD)nSelectedQuest;
		DialogCallbackInit( tOkCallback );
		tOkCallback.pfnCallback = sAbandonStoryQuestCallbackOk;
		UIShowGenericDialog(GlobalStringGet(GS_CONFIRM_DIALOG_HEADER), GlobalStringGet(GS_CONFIRM_QUEST_ABANDON_EXIT), TRUE, &tOkCallback );
	}
	else
	{
		// skip the dialog and go straight to the abandon.
	c_SendQuestAbandon(nSelectedQuest);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestAbandonOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_QUEST_PANEL);
	if (!pPanel)
		return UIMSG_RET_NOT_HANDLED;

	UI_QUEST_PANEL *pQuestPanel = UICastToQuestPanel(pPanel);
	DIALOG_CALLBACK tOkCallback;
	tOkCallback.dwCallbackData = (DWORD)pQuestPanel->m_nSelectedQuest;
	DialogCallbackInit( tOkCallback );
	tOkCallback.pfnCallback = sAbandonQuestCallbackOk;
	UIShowGenericDialog(GlobalStringGet(GS_CONFIRM_DIALOG_HEADER), GlobalStringGet(GS_CONFIRM_QUEST_ABANDON), TRUE, &tOkCallback );

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAutoTrackQuestsOnClick(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	UNIT *pPlayer = UIComponentGetFocusUnit(pComponent);
	ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);

	UnitSetStat(pPlayer, STATS_DONT_AUTO_TRACK_QUESTS, !UnitGetStat(pPlayer, STATS_DONT_AUTO_TRACK_QUESTS));

	c_AutoTrackQuests(!UnitGetStat(pPlayer, STATS_DONT_AUTO_TRACK_QUESTS));

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadQuestPanel(
	CMarkup& xml, 
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	ASSERT_RETFALSE(component);

	UI_QUEST_PANEL *pQuestPanel = UICastToQuestPanel(component);

	pQuestPanel->m_nSelectedQuest = INVALID_ID;
	for (int i=0; i < MAX_CONVERSATION_SPEAKERS; i++)
	{
		pQuestPanel->m_nQuestStepPortraitUnitClass[MAX_CONVERSATION_SPEAKERS] = INVALID_LINK;				
	}
	pQuestPanel->m_nMurmurClass = INVALID_LINK;		
	pQuestPanel->m_idMurmur = INVALID_ID;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentFreeQuestPanel(
	UI_COMPONENT* component)
{
	UI_QUEST_PANEL* pQuestPanel = UICastToQuestPanel(component);
	ASSERT_RETURN(pQuestPanel);

	GAME *pGame = AppGetCltGame();
	if (pGame)
	{
		for (int i=0; i < MAX_CONVERSATION_SPEAKERS; i++)
		{
			if ( pQuestPanel->m_idQuestStepPortraitUnit[i] != INVALID_ID )
			{
				UNIT *pUnit = UnitGetById(pGame, pQuestPanel->m_idQuestStepPortraitUnit[i]);
				if (pUnit &&
					UnitTestFlag(pUnit, UNITFLAG_CLIENT_ONLY))
				{
					UnitFree( pUnit );
					pQuestPanel->m_idQuestStepPortraitUnit[i] = INVALID_ID;
				}
			}
		}

		if ( pQuestPanel->m_idMurmur != INVALID_ID )
		{
			UNIT *pUnit = UnitGetById(pGame, pQuestPanel->m_idMurmur);
			if (pUnit &&
				UnitTestFlag(pUnit, UNITFLAG_CLIENT_ONLY))
			{
				UnitFree( pUnit );
				pQuestPanel->m_idMurmur = INVALID_ID;
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitComponentTypesQuest(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize)
{	
	//					   struct				type						name				load function				free function
	UI_REGISTER_COMPONENT( UI_QUEST_PANEL,		UITYPE_QUEST_PANEL,			"questpanel",		UIXmlLoadQuestPanel,		UIComponentFreeQuestPanel			);

	UIMAP(UITYPE_QUEST_PANEL,		UIMSG_ACTIVATE,					UIComponentOnActivate);
	UIMAP(UITYPE_QUEST_PANEL,		UIMSG_INACTIVATE,				UIComponentOnInactivate);

	UI_XML_ONFUNC gUIXmlFunctions[] =
	{	// function name						function pointer
		{ "UIQuestPanelOnActivate",				UIQuestPanelOnActivate },
		{ "UIQuestPanelOnPostActivate",			UIQuestPanelOnPostActivate },
		{ "UIQuestPanelOnPostInactivate",		UIQuestPanelOnPostInactivate },
		{ "UIQuestListLabelOnClick",			UIQuestListLabelOnClick },
		{ "UIQuestListTrackOnClick",			UIQuestListTrackOnClick },
		{ "UIQuestPanelOnCloseBtnClick",		UIQuestPanelOnCloseBtnClick },
		{ "UIQuestStepAskOnClick",				UIQuestStepAskOnClick },
		{ "UIQuestStepTextOnClick",				UIQuestStepTextOnClick },
		{ "UIQuestPanelDialogOnPagePrev",		UIQuestPanelDialogOnPagePrev },
		{ "UIQuestPanelDialogOnPageNext",		UIQuestPanelDialogOnPageNext },
		{ "UIQuestListOnScrollUp",				UIQuestListOnScrollUp },
		{ "UIQuestListOnScrollDown",			UIQuestListOnScrollDown },
		{ "UIQuestStepsOnScrollUp",				UIQuestStepsOnScrollUp },
		{ "UIQuestStepsOnScrollDown",			UIQuestStepsOnScrollDown },
		{ "UIQuestListOnMouseWheel",			UIQuestListOnMouseWheel },
		{ "UIQuestStepsOnMouseWheel",			UIQuestStepsOnMouseWheel },
		{ "UIQuestAbandonOnClick",				UIQuestAbandonOnClick },
		{ "UIAutoTrackQuestsOnClick",			UIAutoTrackQuestsOnClick },
	};


	// Add on the message handler functions for the local components
	int nOldSize = nXmlFunctionsSize;
	nXmlFunctionsSize += sizeof(gUIXmlFunctions);
	pUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, pUIXmlFunctions, nXmlFunctionsSize);
	memcpy((BYTE *)pUIXmlFunctions + nOldSize, gUIXmlFunctions, sizeof(gUIXmlFunctions));
}
#endif //!ISVERSION(SERVER_VERSION)


