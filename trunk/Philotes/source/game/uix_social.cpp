//----------------------------------------------------------------------------
// uix_social.cpp
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
#include "uix_social.h"
#include "console.h"
#include "servers.h"
#include "ServerRunnerAPI.h"
#include "unit_priv.h"
#include "player.h"
#include "chat.h"
#include "console_priv.h"
#include "language.h"
#include "stringreplacement.h"

#if !ISVERSION(SERVER_VERSION)
#include "c_chatNetwork.h"
#include "c_chatBuddy.h"
#include "c_chatIgnore.h"

static WCHAR curPendingCharName[MAX_CHARACTER_NAME];
static CLIENT_BUDDY_ID curBuddyLinkId = INVALID_BUDDY_ID;
static BOOL curBuddyIsOnline = FALSE;

PGUID curGuildMemberLinkId = INVALID_GUID;
static BOOL curGuildMemberIsOnline = FALSE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISocialEditOnChar( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_EDITCTRL *pEditCtrl = UICastToEditCtrl(component);
	if (!pEditCtrl->m_bHasFocus)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (wParam == VK_RETURN)
	{
	
		const WCHAR *szText = UILabelGetText(pEditCtrl);

		int idComponent = UIComponentGetId(component);
		ConsoleSubmitInput(szText);

		// Tricky, but get pEdit again on the off chance that the command reloaded the UI and invalidated the pointer
		component = UIComponentGetById(idComponent);
		if (component)
		{
			pEditCtrl = UICastToEditCtrl(component);
			UILabelSetText(pEditCtrl, L"");
		}

		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIEditCtrlOnKeyChar(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIHideBuddyListChildPanel()
{
	UI_COMPONENT *pComp = NULL;
	pComp = UIComponentGetByEnum(UICOMP_ADDBUDDY);
	if (pComp) {
		UIComponentActivate(pComp, FALSE);
	}

	pComp = UIComponentGetByEnum(UICOMP_REMOVEBUDDY);
	if (pComp) {
		UIComponentActivate(pComp, FALSE);
	}

	pComp = UIComponentGetByEnum(UICOMP_SETNICKBUDDY);
	if (pComp) {
		UIComponentActivate(pComp, FALSE);
	}

	pComp = UIComponentGetByEnum(UICOMP_PENDINGBUDDY);
	if (pComp) {
		UIComponentActivate(pComp, FALSE);
	}

	pComp = UIComponentGetByEnum(UICOMP_GUILD_INVITE_PANEL);
	if (pComp) {
		UIComponentActivate(pComp, FALSE);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIHideBuddylist(
	void)		
{
	UIHideBuddyListChildPanel();

	UI_COMPONENT* pComp = UIComponentGetByEnum(UICOMP_BUDDYLIST);
	if (pComp) {
		UIStopSkills();
		UIComponentActivate(pComp, FALSE);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIToggleBuddylist(
	void)		
{
#if !ISVERSION(DEVELOPMENT)
	if (AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{
		return;
	}
#endif

	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_BUDDYLIST);
	if (pComponent)
	{
		if (UIComponentGetVisible(pComponent))
		{
			UIHideBuddylist();
		}
		else
		{
			UIComponentActivate(pComponent, TRUE);
			if( AppIsTugboat() )
			{
				UIHideGuildPanel();
				UIHidePartyPanel();
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIToggleBuddyList( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIToggleBuddylist();
	return UIMSG_RET_HANDLED_END_PROCESS;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIBuddyListUpdate(
	void)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_BUDDYLIST);

	if (pComponent)
	{
		UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSortBuddies(
	void * pContext,
	const void * pLeft,
	const void * pRight)
{

	int nSortColumn = *(int *)pContext;

	BUDDY_UI_ENTRY *pBuddy1 = (BUDDY_UI_ENTRY *)pLeft;
	BUDDY_UI_ENTRY *pBuddy2 = (BUDDY_UI_ENTRY *)pRight;

	switch (nSortColumn)
	{
	case 0: 
		return PStrCmp(pBuddy1->szCharName, pBuddy2->szCharName, arrsize(pBuddy1->szCharName)); 
	case 1:
		return (((pBuddy1->nCharacterLevel * 1000) + pBuddy1->nCharacterRank) <
		        ((pBuddy2->nCharacterLevel * 1000) + pBuddy2->nCharacterRank) ? -1 : 1);
	case 2: 
		{
			const UNIT_DATA * pPlayerData = PlayerGetData(NULL, pBuddy1->nCharacterClass);
			if (!pPlayerData)
				return -1;
			const WCHAR *str1 = StringTableGetStringByIndex(pPlayerData->nString);
							  pPlayerData = PlayerGetData(NULL, pBuddy2->nCharacterClass);
			if (!pPlayerData)
				return 1;
			const WCHAR *str2 = StringTableGetStringByIndex(pPlayerData->nString);
			return PStrCmp(str1, str2);
		}
		break;
	case 3:
		WCHAR szLevelName1[100] = L"", szLevelName2[100] = L"";
		if (pBuddy1->nLevelDefinitionId != INVALID_LINK)
		{
			const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet(pBuddy1->nLevelDefinitionId);
			if (pLevelDef)
			{
				PStrCopy(szLevelName1, LevelDefinitionGetDisplayName(pLevelDef), arrsize(szLevelName1));
			}
		}
		if (pBuddy2->nLevelDefinitionId != INVALID_LINK)
		{
			const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet(pBuddy2->nLevelDefinitionId);
			if (pLevelDef)
			{
				PStrCopy(szLevelName2, LevelDefinitionGetDisplayName(pLevelDef), arrsize(szLevelName2));
			}
		}
		return PStrCmp(szLevelName1, szLevelName2);
		break;
	}

	return PStrCmp(pBuddy1->szAcctNick, pBuddy2->szAcctNick, arrsize(pBuddy1->szCharName)); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBuddyListOnActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (ResultIsHandled(UIComponentOnActivate(component, msg, wParam, lParam)) )
	{
		curPendingCharName[0] = L'0';
		curBuddyLinkId = INVALID_BUDDY_ID;
		curBuddyIsOnline = FALSE;
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBuddyListPanelOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
//	UISetMouseOverrideComponent(component);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBuddyListPanelOnPostInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
//	UIRemoveMouseOverrideComponent(component);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBuddyListOnPaint( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	UIComponentRemoveAllElements(component);

	GAME *pGame = AppGetCltGame();
	if (!pGame)
		return UIMSG_RET_NOT_HANDLED;

	// get the column widths from the column headers
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pChild = NULL;
	static const int NUM_COLUMNS = 4;
	float fColumnStart[NUM_COLUMNS];
	float fColumnWidth[NUM_COLUMNS];
	int nSortColumn = -1;
	memclear(fColumnWidth, sizeof(fColumnWidth));
	for (int i=0; i < NUM_COLUMNS; i++)
	{
		while ((pChild = UIComponentIterateChildren(component->m_pParent, pChild, UITYPE_BUTTON, FALSE)) != NULL)
		{
			if ((int)pChild->m_dwParam == i+1)
			{
				fColumnStart[i] = pChild->m_Position.m_fX - component->m_Position.m_fX;
				fColumnWidth[i] = pChild->m_fWidth;
				if (UIComponentIsButton(pChild))
				{
					UI_BUTTONEX * pBtn = UICastToButton(pChild);
					if (UIButtonGetDown(pBtn))
					{
						nSortColumn = i;
					}
				}
			}
		}
	}

	BUDDY_UI_ENTRY pBuddies[MAX_CHAT_BUDDIES];
	int nBuddyCount = c_BuddyGetList(pBuddies, MAX_CHAT_BUDDIES);

	//for (int i=0; i < 40/*MAX_CHAT_BUDDIES - nBuddyCount*/; i++)
	//{
	//	PStrPrintf(pBuddies[nBuddyCount].szAcctNick, arrsize(pBuddies[nBuddyCount].szAcctNick), L"%d Buddy Acct", i);
	//	PStrPrintf(pBuddies[nBuddyCount].szCharName, arrsize(pBuddies[nBuddyCount].szCharName), L"Buddy Char %d", i);
	//	pBuddies[nBuddyCount].bOnline = i%2;// (BOOL)RandGetNum(pGame, 0, 1);
	//	pBuddies[nBuddyCount].nLevel= (i%50) + 1;//(BOOL)RandGetNum(pGame, 1, 99);
	//	pBuddies[nBuddyCount].nClass = i%2;// (BOOL)RandGetNum(pGame, 0, 16);
	//	pBuddies[nBuddyCount].idLink = i+1;//INVALID_BUDDY_ID;//(CLIENT_BUDDY_ID)-1;//nBuddyCount;
	//	nBuddyCount++;
	//}

	qsort_s(pBuddies, nBuddyCount, sizeof(BUDDY_UI_ENTRY), sSortBuddies, (void *)&nSortColumn);

	UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
	int nFontsize = UIComponentGetFontSize(component, pFont);


	UI_RECT rectComp = UIComponentGetRectBase(component);
	DWORD dwColor = GFXCOLOR_WHITE;
	UI_RECT rect(0.0f, 0.0f, 0.0f/*component->m_ScrollPos.m_fY*/, 0.0f);
	float SPACING = 4.0f * UIGetScreenToLogRatioY(component->m_bNoScale);

	UI_COMPONENT *pHilightBar = UIComponentFindChildByName(AppIsHellgate() ? component : component->m_pParent, "buddylist highlight");
	if (pHilightBar)
	{
		UIComponentSetVisible(pHilightBar, FALSE);
	}

	BOOL bHideOffline = FALSE;
	UI_COMPONENT *pHideOfflineBtn = UIComponentFindChildByName(component->m_pParent, "buddy list hide offline btn");
	if (pHideOfflineBtn)
	{
		UI_BUTTONEX * pButton = UICastToButton( pHideOfflineBtn );
		bHideOffline = UIButtonGetDown( pButton );
	}

	if (AppIsTugboat())
	{
		UIComponentSetActive(UIComponentFindChildByName(component->m_pParent, "setnick buddy button"), FALSE);
	}
	//	UIComponentSetActive(UIComponentFindChildByName(component->m_pParent, "whisper buddy button"), FALSE);
	//	UIComponentSetActive(UIComponentFindChildByName(component->m_pParent, "remove buddy button"), FALSE);

	for (int i=0; i < nBuddyCount; i++)
	{
		if (bHideOffline && !pBuddies[i].bOnline)
		{
			continue;
		}

		dwColor = (pBuddies[i].bOnline ? GFXCOLOR_WHITE : GFXCOLOR_GRAY);
		if (curBuddyLinkId == pBuddies[i].idLink)
		{
			curBuddyIsOnline = pBuddies[i].bOnline;
			if (AppIsTugboat())
			{
				UIComponentSetActive(UIComponentFindChildByName(component->m_pParent, "setnick buddy button"), TRUE);
			}
//			UIComponentSetActive(UIComponentFindChildByName(component->m_pParent, "whisper buddy button"), curBuddyIsOnline);
//			UIComponentSetActive(UIComponentFindChildByName(component->m_pParent, "remove buddy button"), TRUE);

			if (pHilightBar)
			{
/*				const float yPos = rect.m_fY1 - component->m_ScrollPos.m_fY;
				if (yPos >= 0)
				{
					pHilightBar->m_Position.m_fY = yPos;
					UIComponentSetVisible(pHilightBar, TRUE);
				}
*/			}
			else
			{
				dwColor = GFXCOLOR_YELLOW;
			}
		}
		if( AppIsHellgate() )
		{
			rect.m_fY2 = rect.m_fY1 + (float)nFontsize * UIGetScreenToLogRatioY(component->m_bNoScale); 
		}
		else
		{
			rect.m_fY2 = rect.m_fY1 + (float)(nFontsize - 4) * UIGetScreenToLogRatioY(component->m_bNoScale); 
		}

//		UI_RECT rectClip = rect;
//		rectClip.m_fY1 -= component->m_ScrollPos.m_fY;
//		rectClip.m_fY2 -= component->m_ScrollPos.m_fY;

//		if (UIUnionRects(rectComp, rectClip, rectClip))
		{
//			trace("\t drawing buddy #%d at y=%f\n", i, rect.m_fY1);

			for (int j=0; j < NUM_COLUMNS; j++)
			{
				WCHAR puszText[256] = L"";
				if( AppIsHellgate() )
				{
					switch (j)
					{
					
					case 0: 
						PStrCopy(puszText, pBuddies[i].szCharName, arrsize(puszText)); 
						break;
					case 1:
						if (pBuddies[i].nCharacterLevel > 0)
						{
							if (pBuddies[i].nCharacterRank > 0)
							{
								//xxx show rank
								WCHAR szLevel[256], szRank[256];
								LanguageFormatIntString(szLevel, arrsize(szRank), pBuddies[i].nCharacterLevel);
								LanguageFormatIntString(szRank, arrsize(szRank), pBuddies[i].nCharacterRank);
								PStrCopy(puszText, GlobalStringGet(GS_LEVEL_WITH_RANK_SHORT), arrsize(puszText));
								PStrReplaceToken( puszText, arrsize(puszText), StringReplacementTokensGet( SR_LEVEL ), szLevel );
								PStrReplaceToken( puszText, arrsize(puszText), StringReplacementTokensGet( SR_RANK ), szRank );
							}
							else
							{
								PStrPrintf(puszText, arrsize(puszText), L"%d", pBuddies[i].nCharacterLevel);
							}
						}
						break;
					case 2: 
						{
							const UNIT_DATA * pPlayerData = PlayerGetData(pGame, pBuddies[i].nCharacterClass);
							if (pPlayerData)
							{
								PStrCopy(puszText, StringTableGetStringByIndex(pPlayerData->nString), arrsize(puszText)); 
							}
						}
						break;
					case 3:
						if (pBuddies[i].nLevelDefinitionId != INVALID_LINK)
						{
							const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet(pBuddies[i].nLevelDefinitionId);
							if (pLevelDef)
							{
								PStrCopy(puszText, LevelDefinitionGetDisplayName(pLevelDef), arrsize(puszText));
							}
						}
						break;
					};
				}
				else
				{
					switch (j)
					{
					case 0:		
						PStrCopy(puszText, pBuddies[i].szAcctNick, arrsize(puszText)); 
						break;
					case 1: 
						PStrCopy(puszText, pBuddies[i].szCharName, arrsize(puszText)); 
						break;
					case 2:
						if (pBuddies[i].nCharacterLevel > 0)
						{
							PStrPrintf(puszText, arrsize(puszText), L"%d", pBuddies[i].nCharacterLevel);
						}
						break;
					case 3: 
						{
							const UNIT_DATA * pPlayerData = PlayerGetData(pGame, pBuddies[i].nCharacterClass);
							if (pPlayerData)
							{
								PStrCopy(puszText, StringTableGetStringByIndex(pPlayerData->nString), arrsize(puszText)); 
							}
						}
						break;
					};
				}
				

				rect.m_fX1 = fColumnStart[j] + SPACING;
				rect.m_fX2 = rect.m_fX1 + fColumnWidth[j] - SPACING;

				UI_RECT rectClip = rect;
				rectClip.m_fY1 -= component->m_ScrollPos.m_fY;
				rectClip.m_fY2 -= component->m_ScrollPos.m_fY;

				if (UIUnionRects(rectComp, rectClip, rectClip))
				{
					UI_ELEMENT_TEXT *pElement =  UIComponentAddTextElement(
						component, NULL, pFont, nFontsize, puszText, UI_POSITION(rect.m_fX1, rect.m_fY1), dwColor, 
						&rectClip, UIALIGN_LEFT, &UI_SIZE(rect.m_fX2 - rect.m_fX1, rect.m_fY2 - rect.m_fY1) );

					ASSERT_RETVAL(pElement, UIMSG_RET_NOT_HANDLED);
					pElement->m_qwData = (QWORD)pBuddies[i].idLink;

					if (curBuddyLinkId == pBuddies[i].idLink && pHilightBar)
					{
						pHilightBar->m_Position.m_fY = rect.m_fY1;
						UIComponentSetVisible(pHilightBar, TRUE);
					}
				}
			}
		}

		rect.m_fY1 = rect.m_fY2 + 1.0f * UIGetScreenToLogRatioY(component->m_bNoScale); 
	}

	rect.m_fX1 = 0;
	rect.m_fX2 = component->m_fWidth;

	pChild = UIComponentIterateChildren(AppIsHellgate() ? component : component->m_pParent, NULL, UITYPE_SCROLLBAR, FALSE);
	if (pChild)
	{	
		UI_SCROLLBAR *pScroll = UICastToScrollBar(pChild);
		pScroll->m_fMin = 0;
		pScroll->m_fMax = rect.m_fY1 - component->m_fHeight;
		if (pScroll->m_fMax < 0.0f)
		{
			pScroll->m_fMax = 0.0f;
			UIComponentSetVisible(pScroll, FALSE);
		}
		else
		{
			UIComponentSetActive(pScroll, TRUE);
		}
		component->m_fScrollVertMax = pScroll->m_fMax;
		component->m_ScrollPos.m_fY = MIN(component->m_ScrollPos.m_fY, component->m_fScrollVertMax);

	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBuddyListOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	c_ChatNetRefreshBuddyListInfo();
	UIBuddyListOnPaint(component, UIMSG_PAINT, 0, 0);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIBuddyListSetupWhisperContext()
{
	LPCWSTR strCharName = NULL;
	if (curBuddyLinkId != INVALID_BUDDY_ID)
	{
		strCharName = c_BuddyGetCharNameByID(curBuddyLinkId);
	}
	else if (curPendingCharName[0])
	{
		strCharName = curPendingCharName;
	}

	if (strCharName)
	{
		c_PlayerSetupWhisperTo(strCharName);
		//ConsoleSetChatContext(CHAT_TYPE_WHISPER);
		//ConsoleSetLastTalkedToPlayerName(strCharName);
		//ConsoleActivateEdit(FALSE, FALSE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBuddyListOnLClick( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	x -= pos.m_fX;
	y -= pos.m_fY;

	x /= UIGetScreenToLogRatioX(component->m_bNoScale);
	y /= UIGetScreenToLogRatioY(component->m_bNoScale);
	y += component->m_ScrollPos.m_fY;
	curBuddyLinkId = INVALID_BUDDY_ID;
	curBuddyIsOnline = FALSE;
	curPendingCharName[0] = L'\0';
	UIHideBuddyListChildPanel();

	UI_GFXELEMENT *pElement = component->m_pGfxElementFirst;
	while (pElement)
	{
		if ( UIElementCheckBounds(pElement, x, y) )
		{
			curBuddyLinkId = (CLIENT_BUDDY_ID)pElement->m_qwData;
			if (pElement->m_qwData == (QWORD)INVALID_BUDDY_ID && pElement->m_eGfxElement == GFXELEMENT_TEXT)
			{
				UI_ELEMENT_TEXT *pTxtElement = (UI_ELEMENT_TEXT*)pElement;
				PStrCopy(curPendingCharName, pTxtElement->m_pText, MAX_CHARACTER_NAME);

				UI_COMPONENT* pComp = UIComponentGetByEnum(UICOMP_PENDINGBUDDY);
				if (pComp)
				{
					UIComponentActivate(pComp, TRUE);
				}
			}
			break;
		}
		pElement = pElement->m_pNextElement;
	}

	UIBuddyListOnPaint(component, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBuddyListOnRButtonDown( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	x -= pos.m_fX;
	y -= pos.m_fY;

	x /= UIGetScreenToLogRatioX(component->m_bNoScale);
	y /= UIGetScreenToLogRatioY(component->m_bNoScale);
	y += component->m_ScrollPos.m_fY;
	//curBuddyLinkId = INVALID_BUDDY_ID;
	//curBuddyIsOnline = FALSE;
	//curPendingCharName[0] = L'\0';

	UI_GFXELEMENT *pElement = component->m_pGfxElementFirst;
	while (pElement)
	{
		if ( UIElementCheckBounds(pElement, x, y) )
		{
			curBuddyLinkId = (CLIENT_BUDDY_ID)pElement->m_qwData;
			if (curBuddyLinkId == INVALID_BUDDY_ID)
			{
				UIBuddyListOnLClick(component, msg, wParam, lParam);
			}
			else
			{
				UNIT * pPlayer = NULL;
				PGUID UnitGUID = c_BuddyGetGuidByID(curBuddyLinkId);
				if (UnitGUID != INVALID_GUID)
				{
					pPlayer = UnitGetByGuid(AppGetCltGame(), UnitGUID);
				}
				c_InteractRequestChoices(pPlayer, component, UnitGUID);
				UIHideBuddyListChildPanel();
			}
			break;
		}
		pElement = pElement->m_pNextElement;
	}

	UIBuddyListOnPaint(component, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBuddyListOnLDoubleClick( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (UIBuddyListOnLClick(component, msg, wParam, lParam) == UIMSG_RET_NOT_HANDLED)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	else
	{
		if (!curBuddyIsOnline)
		{
			return UIMSG_RET_NOT_HANDLED;
		}

		if( AppIsHellgate() )
		{
			UIHideBuddylist();
		}

		UIBuddyListSetupWhisperContext();
		return UIMSG_RET_HANDLED_END_PROCESS;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOpenAddBuddyPanel( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIHideBuddyListChildPanel();

	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_ADDBUDDY);
	if (pComp) {
		UIComponentActivate(pComp, TRUE);
		pComp = UIComponentIterateChildren(pComp, NULL, UITYPE_EDITCTRL, FALSE);
		if (pComp) {
			UILabelSetText(pComp, L"");
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAddBuddyOnClick( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || 
		msg == UIMSG_KEYUP || 
		msg == UIMSG_KEYCHAR) &&
		wParam != VK_RETURN)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_COMPONENT *pComp = UIComponentFindChildByName(component->m_pParent, "add buddy edit");
	if (pComp) {
		const WCHAR *szName = UILabelGetText(pComp);
		if (szName && szName[0] && c_BuddyAdd(szName)) {
			UIHideBuddyListChildPanel();
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBuddyListCancelOnClick( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || 
		msg == UIMSG_KEYUP || 
		msg == UIMSG_KEYCHAR) &&
		wParam != VK_ESCAPE)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIHideBuddyListChildPanel();

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
UI_MSG_RETVAL UICloseBuddyPanel(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIHideBuddylist();
	return UIMSG_RET_HANDLED_END_PROCESS;
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOpenRemoveBuddyPanel( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component)) {
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || 
		msg == UIMSG_KEYUP || 
		msg == UIMSG_KEYCHAR) &&
		wParam != VK_RETURN) {
		return UIMSG_RET_NOT_HANDLED;
	}

	UIHideBuddyListChildPanel();
	if (curBuddyLinkId != INVALID_BUDDY_ID && curPendingCharName[0] == L'\0')
	{
		UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_REMOVEBUDDY);
		if (pComp)
		{
			UIComponentActivate(pComp, TRUE);
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRemoveBuddyOnClick( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent->m_pParent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || 
		msg == UIMSG_KEYUP || 
		msg == UIMSG_KEYCHAR) &&
		wParam != VK_RETURN)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (curBuddyLinkId != INVALID_BUDDY_ID)
	{
		if (UIRemoveCurBuddy())
		{
			UIHideBuddyListChildPanel();
		}
	}
//	else if (c_BuddyDecline(curPendingCharName))
//	{
//		UIHideBuddyListChildPanel();
//	}


	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIRemoveCurBuddy(void)
{
	return c_BuddyRemove(curBuddyLinkId);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOpenSetNickBuddyPanel( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component)) {
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || 
		msg == UIMSG_KEYUP || 
		msg == UIMSG_KEYCHAR) &&
		wParam != VK_RETURN) {
		return UIMSG_RET_NOT_HANDLED;
	}

	UIHideBuddyListChildPanel();
	if (curBuddyLinkId != INVALID_BUDDY_ID)
	{
		UI_COMPONENT *pComp = NULL;
		pComp = UIComponentGetByEnum(UICOMP_SETNICKBUDDY);
		if (pComp)
		{
			UIComponentActivate(pComp, TRUE);
			pComp = UIComponentIterateChildren(pComp, NULL, UITYPE_EDITCTRL, FALSE);
			if (pComp)
			{
				UILabelSetText(pComp, L"");
			}
		}
	}
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISetNickBuddyOnClick( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent->m_pParent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || 
		msg == UIMSG_KEYUP || 
		msg == UIMSG_KEYCHAR) &&
		wParam != VK_RETURN)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	LPCWSTR strNickNameOld = NULL, strNickNameNew = NULL;

	if (curBuddyLinkId != INVALID_BUDDY_ID)
	{
		strNickNameOld = c_BuddyGetNickNameByID(curBuddyLinkId);
	}
	
	UI_COMPONENT *pComp = UIComponentFindChildByName(component->m_pParent, "setnick buddy edit");
	if (pComp) 
	{
		strNickNameNew = UILabelGetText(pComp);
	}

	//	TODO: The buddy system has changed to character rather than account based, set nick is gone.

	if (strNickNameOld && strNickNameNew && c_BuddySetNick(strNickNameOld, strNickNameNew))
	{
		UIHideBuddyListChildPanel();
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPendingAcceptBuddyOnClick( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent->m_pParent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || 
		msg == UIMSG_KEYUP || 
		msg == UIMSG_KEYCHAR) &&
		wParam != VK_RETURN)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	//	the buddy system is no longer consentual, players will never have to
	//		"accept" a buddy request
	//
	//if (c_BuddyAccept(curPendingCharName)) {
	//	UIHideBuddyListChildPanel();
	//}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPendingDeclineBuddyOnClick( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent->m_pParent, UIMSG_RET_NOT_HANDLED);

	/* buddy list is no longer consentual so this is not allowed
	 *
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || 
		msg == UIMSG_KEYUP || 
		msg == UIMSG_KEYCHAR) &&
		wParam != VK_RETURN)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (c_BuddyDecline(curPendingCharName)) {
		UIHideBuddyListChildPanel();
	}
*/
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWhisperBuddyOnClick(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component)) {
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || 
		msg == UIMSG_KEYUP || 
		msg == UIMSG_KEYCHAR) &&
		wParam != VK_RETURN) {
			return UIMSG_RET_NOT_HANDLED;
	}

	UIBuddyListSetupWhisperContext();
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIHideGuildPanel(
	void)		
{
	UIHideBuddyListChildPanel();

	UI_COMPONENT* pComp = UIComponentGetByEnum(UICOMP_GUILDPANEL);
	if (pComp) {
		UIStopSkills();
		UIComponentActivate(pComp, FALSE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIToggleGuildPanel(
	void)		
{
#if !ISVERSION(DEVELOPMENT)
	if ( AppIsHellgate() && c_PlayerGetGuildChannelId() == INVALID_CHANNELID)
	{
		return;
	}
#endif

	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_GUILDPANEL);
	if (pComponent)
	{
		if (UIComponentGetVisible(pComponent))
		{
			UIHideGuildPanel();
		}
		else
		{
			if( AppIsTugboat() )
			{
				UIHideBuddylist();
				UIHidePartyPanel();
			}
			UIComponentActivate(pComponent, TRUE);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIToggleGuildPanel( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIToggleGuildPanel();
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildListPanelOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
//	UISetMouseOverrideComponent(component);

	UI_COMPONENT *pHeader = UIComponentGetById(UIComponentGetIdByName("guildlist panel header"));
	if (pHeader)
	{
		UI_LABELEX *pLabel = UICastToLabel(pHeader);
		UILabelSetText( pLabel, c_PlayerGetGuildName() );
	}

	UI_COMPONENT *pInviteButton = UIComponentGetById(UIComponentGetIdByName("guild invite button"));
	if (pInviteButton)
	{
		UIComponentSetActive(pInviteButton, c_PlayerGetIsGuildLeader());
	}

	//UI_COMPONENT *pLeaveButton = UIComponentGetById(UIComponentGetIdByName("guild leave button"));
	//if (pLeaveButton)
	//{
	//	pLeaveButton->m_Position.m_fX = c_PlayerGetIsGuildLeader() ? 242.0f : 160.0f;
	//}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSortGuildMembers(
	void * pContext,
	const void * pLeft,
	const void * pRight)
{
	int nSortColumn = *(int *)pContext;

	PLAYER_GUILD_MEMBER_DATA *pMember1 = (PLAYER_GUILD_MEMBER_DATA *)pLeft;
	PLAYER_GUILD_MEMBER_DATA *pMember2 = (PLAYER_GUILD_MEMBER_DATA *)pRight;

	switch (nSortColumn)
	{
		default:
		case 0:
			return PStrCmp(pMember1->wszMemberName, pMember2->wszMemberName, arrsize(pMember1->wszMemberName)); 
		case 1:
			return (((pMember1->tMemberClientData.nCharacterExpLevel * 1000) + pMember1->tMemberClientData.nCharacterExpRank) <
			        ((pMember2->tMemberClientData.nCharacterExpLevel * 1000) + pMember2->tMemberClientData.nCharacterExpRank) ? -1 : 1);
		case 2: 
		{
			const UNIT_DATA * pPlayerData = PlayerGetData(NULL, pMember1->tMemberClientData.nPlayerUnitClass);
			if (!pPlayerData)
				return -1;
			const WCHAR *str1 = StringTableGetStringByIndex(pPlayerData->nString);
			pPlayerData = PlayerGetData(NULL, pMember2->tMemberClientData.nPlayerUnitClass);
			if (!pPlayerData)
				return 1;
			const WCHAR *str2 = StringTableGetStringByIndex(pPlayerData->nString);
			return PStrCmp(str1, str2);
		}
		case 3:
			WCHAR szLevelName1[100] = L"", szLevelName2[100] = L"";
			if (pMember1->tMemberClientData.nLevelDefId != INVALID_LINK)
			{
				const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet(pMember1->tMemberClientData.nLevelDefId);
				if (pLevelDef)
				{
					PStrCopy(szLevelName1, LevelDefinitionGetDisplayName(pLevelDef), arrsize(szLevelName1));
				}
			}
			if (pMember2->tMemberClientData.nLevelDefId != INVALID_LINK)
			{
				const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet(pMember2->tMemberClientData.nLevelDefId);
				if (pLevelDef)
				{
					PStrCopy(szLevelName2, LevelDefinitionGetDisplayName(pLevelDef), arrsize(szLevelName2));
				}
			}
			return PStrCmp(szLevelName1, szLevelName2);
			break;
	};
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildListOnPaint( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	UIComponentRemoveAllElements(component);

	GAME *pGame = AppGetCltGame();
	if (!pGame)
		return UIMSG_RET_NOT_HANDLED;

	// get the column widths from the column headers
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pChild = NULL;
	static const int NUM_COLUMNS = 4;
	float fColumnStart[NUM_COLUMNS];
	float fColumnWidth[NUM_COLUMNS];
	int nSortColumn = -1;
	memclear(fColumnWidth, sizeof(fColumnWidth));
	for (int i=0; i < NUM_COLUMNS; i++)
	{
		while ((pChild = UIComponentIterateChildren(component->m_pParent, pChild, UITYPE_BUTTON, FALSE)) != NULL)
		{
			if ((int)pChild->m_dwParam == i+1)
			{
				fColumnStart[i] = pChild->m_Position.m_fX - component->m_Position.m_fX;
				fColumnWidth[i] = pChild->m_fWidth;
				if (UIComponentIsButton(pChild))
				{
					UI_BUTTONEX * pBtn = UICastToButton(pChild);
					if (UIButtonGetDown(pBtn))
					{
						nSortColumn = i;
					}
				}
			}
		}
	}

	BOOL bHideOffline = FALSE;
	UI_COMPONENT *pHideOfflineBtn = UIComponentFindChildByName(component->m_pParent, "guild hide offline btn");
	if (pHideOfflineBtn)
	{
		UI_BUTTONEX * pButton = UICastToButton( pHideOfflineBtn );
		bHideOffline = UIButtonGetDown( pButton );
	}

#define MAX_DISPLAY_GUILD_MEMBERS (MAX_CHAT_BUDDIES*4)	//	MAX_GUILD_SIZE is huge and blows the Stack

	PLAYER_GUILD_MEMBER_DATA pGuildMembers[MAX_DISPLAY_GUILD_MEMBERS];
	const PLAYER_GUILD_MEMBER_DATA *pGuildMember = NULL;
	int nGuildMemberCount = 0;
//	for (int xx=0; xx<7; ++xx)
	{
		pGuildMember = c_PlayerGuildMemberListGetNextMember(NULL, bHideOffline);
		while (pGuildMember)
		{
			pGuildMembers[nGuildMemberCount++] = *pGuildMember;
			ASSERT_BREAK(nGuildMemberCount < MAX_DISPLAY_GUILD_MEMBERS);
			pGuildMember = c_PlayerGuildMemberListGetNextMember(pGuildMember, bHideOffline);
		}
	}

	const PGUID nMyGuid = c_PlayerGetMyGuid();

//	BUDDY_UI_ENTRY pBuddies[MAX_CHAT_BUDDIES + BUDDY_PENDING_MAX];
//	int nBuddyCount = c_BuddyGetList(pBuddies, MAX_CHAT_BUDDIES + BUDDY_PENDING_MAX);
//	for (int i=0; i < 40/*MAX_CHAT_BUDDIES - nBuddyCount*/; i++)
//	{
//		pGuildMembers[nGuildMemberCount].bIsOnline = i%2;// (BOOL)RandGetNum(pGame, 0, 1);
//		pGuildMembers[nGuildMemberCount].idMemberCharacterId = i+1;//INVALID_BUDDY_ID;//(CLIENT_BUDDY_ID)-1;//nBuddyCount;
		//pBuddies[nBuddyCount].bPending = FALSE;
//		nGuildMemberCount++;
//	}

	qsort_s(pGuildMembers, nGuildMemberCount, sizeof(PLAYER_GUILD_MEMBER_DATA), sSortGuildMembers, (void *)&nSortColumn);

	UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
	int nFontsize = UIComponentGetFontSize(component, pFont);


	UI_RECT rectComp = UIComponentGetRectBase(component);
	DWORD dwColor = GFXCOLOR_WHITE;
	UI_RECT rect(0.0f, 0.0f, 0.0f/*component->m_ScrollPos.m_fY*/, 0.0f);
	static const float SPACING = 4.0f;

	UI_COMPONENT *pHilightBar = UIComponentFindChildByName(AppIsHellgate() ? component : component->m_pParent, "buddylist highlight");
	if (pHilightBar)
	{
		UIComponentSetVisible(pHilightBar, FALSE);
	}

	UI_COMPONENT *pGuildLeaderInd = UIComponentFindChildByName(component, "guild leader ind");
	UI_COMPONENT *pGuildOfficerInd = UIComponentFindChildByName(component, "guild officer ind");
	UI_COMPONENT *pGuildMemberInd = UIComponentFindChildByName(component, "guild member ind");

	for (int i=0; i < nGuildMemberCount; i++)
	{
		if (bHideOffline && !pGuildMembers[i].bIsOnline)
		{
			continue;
		}

		dwColor = (pGuildMembers[i].bIsOnline ? GFXCOLOR_WHITE : GFXCOLOR_GRAY);
		if (curGuildMemberLinkId == pGuildMembers[i].idMemberCharacterId)
		{
			curGuildMemberIsOnline = pGuildMembers[i].bIsOnline;
		}

		if (pGuildMembers[i].idMemberCharacterId == nMyGuid)
		{
			dwColor = GetFontColor(FONTCOLOR_FSORANGE);// GFXCOLOR_YELLOW;
		}

		if( AppIsHellgate() )
		{
			rect.m_fY2 = rect.m_fY1 + (float)nFontsize * UIGetScreenToLogRatioY(component->m_bNoScale); 
		}
		else
		{
			rect.m_fY2 = rect.m_fY1 + (float)(nFontsize - 4) * UIGetScreenToLogRatioY(component->m_bNoScale); 
		}

		//		UI_RECT rectClip = rect;
		//		rectClip.m_fY1 -= component->m_ScrollPos.m_fY;
		//		rectClip.m_fY2 -= component->m_ScrollPos.m_fY;

		//		if (UIUnionRects(rectComp, rectClip, rectClip))
		{
			//			trace("\t drawing buddy #%d at y=%f\n", i, rect.m_fY1);

			for (int j=0; j < NUM_COLUMNS; j++)
			{
				WCHAR puszText[256] = L"";
				switch (j)
				{
				case 0:
					PStrCopy(puszText, pGuildMembers[i].wszMemberName, arrsize(puszText)); 
					break;
				case 1:
					if (pGuildMembers[i].tMemberClientData.nCharacterExpLevel > 0)
					{
						if (pGuildMembers[i].tMemberClientData.nCharacterExpRank > 0)
						{
							//xxx show rank
							WCHAR szLevel[256], szRank[256];
							LanguageFormatIntString(szLevel, arrsize(szRank), pGuildMembers[i].tMemberClientData.nCharacterExpLevel);
							LanguageFormatIntString(szRank, arrsize(szRank), pGuildMembers[i].tMemberClientData.nCharacterExpRank);
							PStrCopy(puszText, GlobalStringGet(GS_LEVEL_WITH_RANK_SHORT), arrsize(puszText));
							PStrReplaceToken( puszText, arrsize(puszText), StringReplacementTokensGet( SR_LEVEL ), szLevel );
							PStrReplaceToken( puszText, arrsize(puszText), StringReplacementTokensGet( SR_RANK ), szRank );
						}
						else
						{
							PStrPrintf(puszText, arrsize(puszText), L"%d", pGuildMembers[i].tMemberClientData.nCharacterExpLevel);
						}
					}
					break;
				case 2:
					{
						const UNIT_DATA * pPlayerData = PlayerGetData(pGame, pGuildMembers[i].tMemberClientData.nPlayerUnitClass);
						if (pPlayerData)
						{
							PStrCopy(puszText, StringTableGetStringByIndex(pPlayerData->nString), arrsize(puszText)); 
						}
					}
					break;
				case 3:
					if (pGuildMembers[i].tMemberClientData.nLevelDefId != INVALID_LINK)
					{
						const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet(pGuildMembers[i].tMemberClientData.nLevelDefId);
						if (pLevelDef)
						{
							PStrCopy(puszText, LevelDefinitionGetDisplayName(pLevelDef), arrsize(puszText));
						}
					}
					break;
				};

				rect.m_fX1 = fColumnStart[j] + SPACING;
				rect.m_fX2 = rect.m_fX1 + fColumnWidth[j] - SPACING;

				if (j==0)
				{
					rect.m_fX1 += 25.0f; // leave room for the "crown" icon (for guild leader)
				}

				UI_RECT rectClip = rect;
				rectClip.m_fY1 -= component->m_ScrollPos.m_fY;
				rectClip.m_fY2 -= component->m_ScrollPos.m_fY;

				if (UIUnionRects(rectComp, rectClip, rectClip))
				{
					UI_ELEMENT_TEXT *pElement =  UIComponentAddTextElement(
						component, NULL, pFont, nFontsize, puszText, UI_POSITION(rect.m_fX1, rect.m_fY1), dwColor, 
						&rectClip, UIALIGN_LEFT, &UI_SIZE(rect.m_fX2 - rect.m_fX1, rect.m_fY2 - rect.m_fY1) );

					ASSERT_RETVAL(pElement, UIMSG_RET_NOT_HANDLED);
					pElement->m_qwData = (QWORD)pGuildMembers[i].idMemberCharacterId;

					if (j == 0)
					{
						if (pGuildMembers[i].idMemberCharacterId == curGuildMemberLinkId && pHilightBar)
						{
							pHilightBar->m_Position.m_fY = rect.m_fY1;
							UIComponentSetVisible(pHilightBar, TRUE);
						}

						UI_COMPONENT *pIconComponent = NULL;

						switch (pGuildMembers[i].eGuildRank)
						{
							case GUILD_RANK_LEADER:		pIconComponent = pGuildLeaderInd;	break;
							case GUILD_RANK_OFFICER:	pIconComponent = pGuildOfficerInd;	break;
							case GUILD_RANK_MEMBER:		pIconComponent = pGuildMemberInd;	break;
						};

						if (pIconComponent)
						{
							UI_POSITION posIcon;
							posIcon.m_fX = pGuildLeaderInd->m_Position.m_fX;
							posIcon.m_fY = pGuildLeaderInd->m_Position.m_fY + rect.m_fY1;

							UI_GFXELEMENT *pGfxElement = UIComponentAddElement(component, pIconComponent->m_pTexture, pIconComponent->m_pFrame,
								posIcon, pIconComponent->m_dwColor, NULL, FALSE, 1.0f, 1.0f );
							ASSERT_RETVAL(pGfxElement, UIMSG_RET_NOT_HANDLED);
							ASSERT_RETVAL(pGfxElement->m_pComponent, UIMSG_RET_NOT_HANDLED);
							pGfxElement->m_pComponent->m_nRenderSection = pIconComponent->m_nRenderSection;
						}
					}
				}
			}
		}

		rect.m_fY1 = rect.m_fY2 + 1.0f; 
	}

	pChild = UIComponentIterateChildren(AppIsHellgate() ? component : component->m_pParent, NULL, UITYPE_SCROLLBAR, FALSE);
	if (pChild)
	{	
		UI_SCROLLBAR *pScroll = UICastToScrollBar(pChild);
		pScroll->m_fMin = 0;
		pScroll->m_fMax = rect.m_fY1 - component->m_fHeight;
		if (pScroll->m_fMax < 0.0f)
		{
			pScroll->m_fMax = 0.0f;
			UIComponentSetVisible(pScroll, FALSE);
		}
		else
		{
			UIComponentSetActive(pScroll, TRUE);
		}
		component->m_fScrollVertMax = pScroll->m_fMax;
		component->m_ScrollPos.m_fY = MIN(component->m_ScrollPos.m_fY, component->m_fScrollVertMax);

	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildListOnLClick(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	x -= pos.m_fX;
	y -= pos.m_fY;
	x /= UIGetScreenToLogRatioX(component->m_bNoScale);
	y /= UIGetScreenToLogRatioY(component->m_bNoScale);
	y += component->m_ScrollPos.m_fY;
	curGuildMemberLinkId = INVALID_GUID;
	curGuildMemberIsOnline = FALSE;
	UIHideBuddyListChildPanel();

	UI_GFXELEMENT *pElement = component->m_pGfxElementFirst;
	while (pElement)
	{
		if ( UIElementCheckBounds(pElement, x, y) )
		{
			curGuildMemberLinkId = (CLIENT_BUDDY_ID)pElement->m_qwData;
			break;
		}
		pElement = pElement->m_pNextElement;
	}

	UIGuildListOnPaint(component, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildListOnActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (ResultIsHandled(UIComponentOnActivate(component, msg, wParam, lParam)) )
	{
		//		curPendingCharName[0] = L'0';
		curGuildMemberLinkId = INVALID_GUID;
		curGuildMemberIsOnline = FALSE;
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildListOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	c_ChatNetRefreshGuildListInfo();
	UIGuildListOnPaint(component, UIMSG_PAINT, 0, 0);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOpenGuildInvitePanel(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIHideBuddyListChildPanel();

	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_GUILD_INVITE_PANEL);
	if (pComp) {
		UIComponentActivate(pComp, TRUE);
		pComp = UIComponentIterateChildren(pComp, NULL, UITYPE_EDITCTRL, FALSE);
		if (pComp) {
			UILabelSetText(pComp, L"");
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildInviteOnClick(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || 
		msg == UIMSG_KEYUP || 
		msg == UIMSG_KEYCHAR) &&
		wParam != VK_RETURN)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_COMPONENT *pComp = UIComponentFindChildByName(component->m_pParent, "add guild edit");
	if (pComp)
	{
		const WCHAR *szName = UILabelGetText(pComp);
		if (szName && szName[0] && c_GuildInvite(szName))
		{
			UIHideBuddyListChildPanel();
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGuildLeaveCallback(
	void *pUserData, 
	DWORD dwCallbackData)
{
	if (c_GuildLeave())
	{
		UIHideGuildPanel();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildLeaveOnClick(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIHideBuddyListChildPanel();

	// setup callback
	DIALOG_CALLBACK tGuildLeaveCallback;
	DialogCallbackInit( tGuildLeaveCallback );
	tGuildLeaveCallback.pfnCallback = sGuildLeaveCallback;

	UIShowGenericDialog(StringTableGetStringByKey("ui guildlist leave button label"), StringTableGetStringByKey("ui buddylist remove instructions"), TRUE, &tGuildLeaveCallback, 0, 0 );

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildListOnRButtonDown( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	x -= pos.m_fX;
	y -= pos.m_fY;
	x /= UIGetScreenToLogRatioX(component->m_bNoScale);
	y /= UIGetScreenToLogRatioY(component->m_bNoScale);
	y += component->m_ScrollPos.m_fY;
	//curBuddyLinkId = INVALID_BUDDY_ID;
	//curBuddyIsOnline = FALSE;
	//curPendingCharName[0] = L'\0';
	UIHideBuddyListChildPanel();

	UI_GFXELEMENT *pElement = component->m_pGfxElementFirst;
	while (pElement)
	{
		if ( UIElementCheckBounds(pElement, x, y) )
		{
			curGuildMemberLinkId = (PGUID)pElement->m_qwData;
			if (curGuildMemberLinkId != c_PlayerGetMyGuid())
			{
				UNIT * pPlayer = NULL;
				if (curGuildMemberLinkId != INVALID_GUID)
				{
					pPlayer = UnitGetByGuid(AppGetCltGame(), curGuildMemberLinkId);
				}

				if (c_PlayerIsGuildMemberOnline(curGuildMemberLinkId))
				{
					c_InteractRequestChoices(pPlayer, component, curGuildMemberLinkId);
				}
				else if (c_PlayerGetCurrentGuildRank() > GUILD_RANK_RECRUIT)
				{
					c_InteractRequestChoices(pPlayer, component, INVALID_GUID);
				}
			}
			break;
		}
		pElement = pElement->m_pNextElement;
	}

	UIGuildListOnPaint(component, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIGuildListSetupWhisperContext()
{
	LPCWSTR strCharName = NULL;
	if (curGuildMemberLinkId != INVALID_GUID)
	{
		strCharName = (WCHAR*)c_PlayerGetGuildMemberNameByID(curGuildMemberLinkId);
	}

	if (strCharName)
	{
		c_PlayerSetupWhisperTo(strCharName);
		//ConsoleSetChatContext(CHAT_TYPE_WHISPER);
		//ConsoleSetLastTalkedToPlayerName(strCharName);
		//ConsoleActivateEdit(FALSE, FALSE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildListOnLDoubleClick(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (UIGuildListOnLClick(component, msg, wParam, lParam) == UIMSG_RET_NOT_HANDLED)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	else
	{
		if (curGuildMemberLinkId == c_PlayerGetMyGuid())
		{
			return UIMSG_RET_NOT_HANDLED;
		}

		if (!curGuildMemberIsOnline)
		{
			return UIMSG_RET_NOT_HANDLED;
		}

		if( AppIsHellgate() )
		{
			UIHideGuildPanel();
		}

		UIGuildListSetupWhisperContext();
		return UIMSG_RET_HANDLED_END_PROCESS;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIGuildPanelUpdate(
	void)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_GUILDPANEL);

	if (pComponent)
	{
		UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIHidePartyPanel(
	void)
{
//	UIHideBuddyListChildPanel();

	UI_COMPONENT* pComp = NULL;
	if (AppIsTugboat())
		pComp = UIComponentGetByEnum(UICOMP_PARTYPANEL);
	else
		pComp = UIComponentGetByEnum(UICOMP_PLAYERMATCH_PANEL);
	if (pComp) {
		UIStopSkills();
		UIComponentActivate(pComp, FALSE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITogglePartyPanel(
	void)
{
#if !ISVERSION(DEVELOPMENT)
	if (AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{
		return;
	}
#endif

	UI_COMPONENT *pComponent = NULL;

	// KCK: Not sure if this is a good idea or not:
	// In PvP levels, we don't really need the party panel, so bring up the PvP scoreboard instead
	if (GameIsPVP(AppGetCltGame()))
	{
		pComponent = UIComponentGetByEnum(UICOMP_SCOREBOARD);
		if (pComponent)
		{
			if (UIComponentGetVisible(pComponent))
				UICloseScoreboard();
			else
				UIOpenScoreboard();
		}
	}
	else
	{
		if (AppIsTugboat())
			pComponent = UIComponentGetByEnum(UICOMP_PARTYPANEL);
		else
			pComponent = UIComponentGetByEnum(UICOMP_PLAYERMATCH_PANEL);
		if (pComponent)
		{
			if (UIComponentGetVisible(pComponent))
			{
				UIHidePartyPanel();
			}
			else
			{
				UIComponentActivate(pComponent, TRUE);
				if( AppIsTugboat() )
				{
					UIHideGuildPanel();
					UIHideBuddylist();
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITogglePartyPanel( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UITogglePartyPanel();
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIOpenScoreboard(void)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_SCOREBOARD);
	if (pComponent)
	{
		UIComponentActivate(pComponent, TRUE);
		pComponent = UIComponentGetByEnum(UICOMP_SCOREBOARD_SHORT);
		if (pComponent)
		{
			UIComponentSetActive(pComponent, FALSE);
			UIComponentSetVisible(pComponent, FALSE);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UICloseScoreboard(void)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_SCOREBOARD);
	if (pComponent)
	{
		UIComponentActivate(pComponent, FALSE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildToggleManagementBtnOnClick( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pGuildPanel = UIComponentGetByEnum(UICOMP_GUILDPANEL);
	ASSERT_RETVAL(pGuildPanel, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pPanel = UIComponentFindChildByName(pGuildPanel, "guild management panel");
	ASSERT_RETVAL(pPanel, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX *pButton = UICastToButton(component);
	if (UIButtonGetDown(pButton))
	{
		UIComponentActivate(pPanel, TRUE);
	}
	else
	{
		UIComponentSetVisible(pPanel, FALSE);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildManagementCancelOnClick( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	UI_COMPONENT *pGuildPanel = UIComponentGetByEnum(UICOMP_GUILDPANEL);
	ASSERT_RETVAL(pGuildPanel, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pButton = UIComponentFindChildByName(pGuildPanel, "guild toggle management btn");
	ASSERT_RETVAL(pButton, UIMSG_RET_NOT_HANDLED);

	UIButtonSetDown(UICastToButton(pButton), FALSE);
	UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);

	return UIGuildToggleManagementBtnOnClick(pButton, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildManagementSaveOnClick( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	UI_COMPONENT *pGuildPanel = UIComponentGetByEnum(UICOMP_GUILDPANEL);
	ASSERT_RETVAL(pGuildPanel, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pPanel = UIComponentFindChildByName(pGuildPanel, "guild management panel");
	ASSERT_RETVAL(pPanel, UIMSG_RET_NOT_HANDLED);

	ASSERT_RETVAL(c_PlayerGetIsGuildLeader(), UIMSG_RET_NOT_HANDLED);

	// save guild management settings here

	// save rank names

	BOOL bError = FALSE;
	UI_COMPONENT * pLabel = NULL;
	const WCHAR * szOldRankString = NULL;
	const WCHAR * szLeaderRankString = NULL, *szOfficerRankString = NULL, *szMemberRankString = NULL, *szRecruitRankString = NULL;
	const WCHAR * szNewLeaderRankString = NULL, *szNewOfficerRankString = NULL, *szNewMemberRankString = NULL, *szNewRecruitRankString = NULL;

	if ((szOldRankString = c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_LEADER)) != NULL)
	{
		if ((pLabel = UIComponentFindChildByName(pPanel, "rank name editctrl 1")) != NULL)
		{
			szLeaderRankString = UILabelGetText(pLabel);
			if (!szLeaderRankString || !szLeaderRankString[0])
			{
				bError = TRUE;
			}
			else if (PStrCmp(szLeaderRankString, szOldRankString))
			{
				szNewLeaderRankString = szLeaderRankString;
			}
		}
	}

	if ((szOldRankString = c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_OFFICER)) != NULL)
	{
		if ((pLabel = UIComponentFindChildByName(pPanel, "rank name editctrl 2")) != NULL)
		{
			szOfficerRankString = UILabelGetText(pLabel);
			if (!szOfficerRankString || !szOfficerRankString[0])
			{
				bError = TRUE;
			}
			else if (PStrCmp(szOfficerRankString, szOldRankString))
			{
				szNewOfficerRankString = szOfficerRankString;
			}
		}
	}

	if ((szOldRankString = c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_MEMBER)) != NULL)
	{
		if ((pLabel = UIComponentFindChildByName(pPanel, "rank name editctrl 3")) != NULL)
		{
			szMemberRankString = UILabelGetText(pLabel);
			if (!szMemberRankString || !szMemberRankString[0])
			{
				bError = TRUE;
			}
			else if (PStrCmp(szMemberRankString, szOldRankString))
			{
				szNewMemberRankString = szMemberRankString;
			}
		}
	}

	if ((szOldRankString = c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_RECRUIT)) != NULL)
	{
		if ((pLabel = UIComponentFindChildByName(pPanel, "rank name editctrl 4")) != NULL)
		{
			szRecruitRankString = UILabelGetText(pLabel);
			if (!szRecruitRankString || !szRecruitRankString[0])
			{
				bError = TRUE;
			}
			else if (PStrCmp(szRecruitRankString, szOldRankString))
			{
				szNewRecruitRankString = szRecruitRankString;
			}
		}
	}

	if (bError)
	{
		UIShowGenericDialog(StringTableGetStringByKey("guildmanagement_title"), StringTableGetStringByKey("guildmanagement_ranks_cannot_be_blank"));
		return UIMSG_RET_NOT_HANDLED;
	}
	else if (!PStrICmp(szLeaderRankString, szOfficerRankString) || !PStrICmp(szLeaderRankString, szMemberRankString) ||
	         !PStrICmp(szLeaderRankString, szRecruitRankString) || !PStrICmp(szOfficerRankString, szMemberRankString) ||
	         !PStrICmp(szOfficerRankString, szRecruitRankString) || !PStrICmp(szMemberRankString, szRecruitRankString))
	{
		UIShowGenericDialog(StringTableGetStringByKey("guildmanagement_title"), StringTableGetStringByKey("guildmanagement_ranks_cannot_be_duplicated"));
		return UIMSG_RET_NOT_HANDLED;
	}

	// save rank permissions based on checkboxes

	GUILD_RANK promoteMinRank, demoteMinRank, emailMinRank, inviteMinRank, bootMinRank;
	if (!c_PlayerGetGuildPermissionsInfo(promoteMinRank, demoteMinRank, emailMinRank, inviteMinRank, bootMinRank))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	GUILD_RANK eCurRank;
	UI_COMPONENT *pButtonPanel = NULL;
	UI_COMPONENT *pButton = NULL;

	if ((pButtonPanel = UIComponentFindChildByName(pPanel, "gm promote buttons")) != NULL)
	{
		eCurRank = (GUILD_RANK)1;
		pButton = NULL;
		while ((pButton = UIComponentIterateChildren(pButtonPanel, pButton, UITYPE_BUTTON, FALSE)) != NULL)
		{
			if (UIButtonGetDown(UICastToButton(pButton)))
			{
				promoteMinRank = (promoteMinRank == eCurRank) ? GUILD_RANK_INVALID : eCurRank;
				break;
			}
			eCurRank = (GUILD_RANK)(((int)eCurRank)+1);
		}
	}

	if ((pButtonPanel = UIComponentFindChildByName(pPanel, "gm demote buttons")) != NULL)
	{
		eCurRank = (GUILD_RANK)1;
		pButton = NULL;
		while ((pButton = UIComponentIterateChildren(pButtonPanel, pButton, UITYPE_BUTTON, FALSE)) != NULL)
		{
			if (UIButtonGetDown(UICastToButton(pButton)))
			{
				demoteMinRank = (demoteMinRank == eCurRank) ? GUILD_RANK_INVALID : eCurRank;
				break;
			}
			eCurRank = (GUILD_RANK)(((int)eCurRank)+1);
		}
	}

	if ((pButtonPanel = UIComponentFindChildByName(pPanel, "gm ban buttons")) != NULL)
	{
		eCurRank = (GUILD_RANK)1;
		pButton = NULL;
		while ((pButton = UIComponentIterateChildren(pButtonPanel, pButton, UITYPE_BUTTON, FALSE)) != NULL)
		{
			if (UIButtonGetDown(UICastToButton(pButton)))
			{
				bootMinRank = (bootMinRank == eCurRank) ? GUILD_RANK_INVALID : eCurRank;
				break;
			}
			eCurRank = (GUILD_RANK)(((int)eCurRank)+1);
		}
	}

	if ((pButtonPanel = UIComponentFindChildByName(pPanel, "gm invite buttons")) != NULL)
	{
		eCurRank = (GUILD_RANK)1;
		pButton = NULL;
		while ((pButton = UIComponentIterateChildren(pButtonPanel, pButton, UITYPE_BUTTON, FALSE)) != NULL)
		{
			if (UIButtonGetDown(UICastToButton(pButton)))
			{
				inviteMinRank = (inviteMinRank == eCurRank) ? GUILD_RANK_INVALID : eCurRank;
				break;
			}
			eCurRank = (GUILD_RANK)(((int)eCurRank)+1);
		}
	}

	if ((pButtonPanel = UIComponentFindChildByName(pPanel, "gm email buttons")) != NULL)
	{
		eCurRank = (GUILD_RANK)1;
		pButton = NULL;
		while ((pButton = UIComponentIterateChildren(pButtonPanel, pButton, UITYPE_BUTTON, FALSE)) != NULL)
		{
			if (UIButtonGetDown(UICastToButton(pButton)))
			{
				emailMinRank = (emailMinRank == eCurRank) ? GUILD_RANK_INVALID : eCurRank;
				break;
			}
			eCurRank = (GUILD_RANK)(((int)eCurRank)+1);
		}
	}

	// everything looks okay, send the new values to the server

	if (szNewLeaderRankString)
	{
		c_GuildChangeRankName(GUILD_RANK_LEADER, szNewLeaderRankString);
	}
	if (szNewOfficerRankString)
	{
		c_GuildChangeRankName(GUILD_RANK_OFFICER, szNewOfficerRankString);
	}
	if (szNewMemberRankString)
	{
		c_GuildChangeRankName(GUILD_RANK_MEMBER, szNewMemberRankString);
	}
	if (szNewRecruitRankString)
	{
		c_GuildChangeRankName(GUILD_RANK_RECRUIT, szNewRecruitRankString);
	}
	if (promoteMinRank != GUILD_RANK_INVALID)
	{
		c_GuildChangeActionPermissions(GUILD_ACTION_PROMOTE, promoteMinRank);
	}
	if (demoteMinRank != GUILD_RANK_INVALID)
	{
		c_GuildChangeActionPermissions(GUILD_ACTION_DEMOTE, demoteMinRank);
	}
	if (emailMinRank != GUILD_RANK_INVALID)
	{
		c_GuildChangeActionPermissions(GUILD_ACTION_EMAIL_GUILD, emailMinRank);
	}
	if (inviteMinRank != GUILD_RANK_INVALID)
	{
		c_GuildChangeActionPermissions(GUILD_ACTION_INVITE, inviteMinRank);
	}
	if (bootMinRank != GUILD_RANK_INVALID)
	{
		c_GuildChangeActionPermissions(GUILD_ACTION_BOOT, bootMinRank);
	}

	return UIGuildManagementCancelOnClick(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildManagementOnPostVisible( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam )
{
	UI_COMPONENT *pGuildPanel = UIComponentGetByEnum(UICOMP_GUILDPANEL);
	ASSERT_RETVAL(pGuildPanel, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pPanel = UIComponentFindChildByName(pGuildPanel, "guild management panel");
	ASSERT_RETVAL(pPanel, UIMSG_RET_NOT_HANDLED);

	BOOL bIsGuildLeader = c_PlayerGetIsGuildLeader();

	// initialize rank name labels and edit controls

	UI_COMPONENT * pLabel = NULL;
	const WCHAR * szRankString = NULL;

	if ((szRankString = c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_LEADER)) != NULL)
	{
		if ((pLabel = UIComponentFindChildByName(pPanel, "rank name label 1")) != NULL)
		{
			UILabelSetText(pLabel, szRankString);
		}
		if ((pLabel = UIComponentFindChildByName(pPanel, "rank name editctrl 1")) != NULL)
		{
			UILabelSetText(pLabel, szRankString);
			UIComponentSetActive(pLabel, bIsGuildLeader);
			UICastToLabel(pLabel)->m_bDisabled = !bIsGuildLeader;
			UICastToEditCtrl(pLabel)->m_nMaxLen = MAX_CHARACTER_NAME;
			UIComponentHandleUIMessage(pLabel, UIMSG_PAINT, 0, 0);
		}
	}

	if ((szRankString = c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_OFFICER)) != NULL)
	{
		if ((pLabel = UIComponentFindChildByName(pPanel, "rank name label 2")) != NULL)
		{
			UILabelSetText(pLabel, szRankString);
		}
		if ((pLabel = UIComponentFindChildByName(pPanel, "rank name editctrl 2")) != NULL)
		{
			UILabelSetText(pLabel, szRankString);
			UIComponentSetActive(pLabel, bIsGuildLeader);
			UICastToLabel(pLabel)->m_bDisabled = !bIsGuildLeader;
			UICastToEditCtrl(pLabel)->m_nMaxLen = MAX_CHARACTER_NAME;
			UIComponentHandleUIMessage(pLabel, UIMSG_PAINT, 0, 0);
		}
	}

	if ((szRankString = c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_MEMBER)) != NULL)
	{
		if ((pLabel = UIComponentFindChildByName(pPanel, "rank name label 3")) != NULL)
		{
			UILabelSetText(pLabel, szRankString);
		}
		if ((pLabel = UIComponentFindChildByName(pPanel, "rank name editctrl 3")) != NULL)
		{
			UILabelSetText(pLabel, szRankString);
			UIComponentSetActive(pLabel, bIsGuildLeader);
			UICastToLabel(pLabel)->m_bDisabled = !bIsGuildLeader;
			UICastToEditCtrl(pLabel)->m_nMaxLen = MAX_CHARACTER_NAME;
			UIComponentHandleUIMessage(pLabel, UIMSG_PAINT, 0, 0);
		}
	}

	if ((szRankString = c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_RECRUIT)) != NULL)
	{
		if ((pLabel = UIComponentFindChildByName(pPanel, "rank name label 4")) != NULL)
		{
			UILabelSetText(pLabel, szRankString);
		}
		if ((pLabel = UIComponentFindChildByName(pPanel, "rank name editctrl 4")) != NULL)
		{
			UILabelSetText(pLabel, szRankString);
			UIComponentSetActive(pLabel, bIsGuildLeader);
			UICastToLabel(pLabel)->m_bDisabled = !bIsGuildLeader;
			UICastToEditCtrl(pLabel)->m_nMaxLen = MAX_CHARACTER_NAME;
			UIComponentHandleUIMessage(pLabel, UIMSG_PAINT, 0, 0);
		}
	}

	// initialize rank permission checkboxes

	GUILD_RANK promoteMinRank, demoteMinRank, emailMinRank, inviteMinRank, bootMinRank;
	if (!c_PlayerGetGuildPermissionsInfo(promoteMinRank, demoteMinRank, emailMinRank, inviteMinRank, bootMinRank))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	GUILD_RANK eMinRank, eCurRank;
	UI_COMPONENT *pButtonPanel = NULL;
	UI_COMPONENT *pButton = NULL;

	if ((pButtonPanel = UIComponentFindChildByName(pPanel, "gm promote buttons")) != NULL)
	{
		eMinRank = promoteMinRank;
		eCurRank = (GUILD_RANK)1;
		pButton = NULL;
		while ((pButton = UIComponentIterateChildren(pButtonPanel, pButton, UITYPE_BUTTON, FALSE)) != NULL)
		{
			UIButtonSetDown(UICastToButton(pButton), (eCurRank >= eMinRank));
			UIComponentSetActive(pButton, bIsGuildLeader && eCurRank != GUILD_RANK_LEADER && eCurRank != GUILD_RANK_RECRUIT);
			UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
			eCurRank = (GUILD_RANK)(((int)eCurRank)+1);
		}
	}

	if ((pButtonPanel = UIComponentFindChildByName(pPanel, "gm demote buttons")) != NULL)
	{
		eMinRank = demoteMinRank;
		eCurRank = (GUILD_RANK)1;
		pButton = NULL;
		while ((pButton = UIComponentIterateChildren(pButtonPanel, pButton, UITYPE_BUTTON, FALSE)) != NULL)
		{
			UIButtonSetDown(UICastToButton(pButton), (eCurRank >= eMinRank));
			UIComponentSetActive(pButton, bIsGuildLeader && eCurRank != GUILD_RANK_LEADER && eCurRank != GUILD_RANK_RECRUIT);
			UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
			eCurRank = (GUILD_RANK)(((int)eCurRank)+1);
		}
	}

	if ((pButtonPanel = UIComponentFindChildByName(pPanel, "gm ban buttons")) != NULL)
	{
		eMinRank = bootMinRank;
		eCurRank = (GUILD_RANK)1;
		pButton = NULL;
		while ((pButton = UIComponentIterateChildren(pButtonPanel, pButton, UITYPE_BUTTON, FALSE)) != NULL)
		{
			UIButtonSetDown(UICastToButton(pButton), (eCurRank >= eMinRank));
			UIComponentSetActive(pButton, bIsGuildLeader && eCurRank != GUILD_RANK_LEADER && eCurRank != GUILD_RANK_RECRUIT);
			UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
			eCurRank = (GUILD_RANK)(((int)eCurRank)+1);
		}
	}

	if ((pButtonPanel = UIComponentFindChildByName(pPanel, "gm invite buttons")) != NULL)
	{
		eMinRank = inviteMinRank;
		eCurRank = (GUILD_RANK)1;
		pButton = NULL;
		while ((pButton = UIComponentIterateChildren(pButtonPanel, pButton, UITYPE_BUTTON, FALSE)) != NULL)
		{
			UIButtonSetDown(UICastToButton(pButton), (eCurRank >= eMinRank));
			UIComponentSetActive(pButton, bIsGuildLeader && eCurRank != GUILD_RANK_LEADER && eCurRank != GUILD_RANK_RECRUIT);
			UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
			eCurRank = (GUILD_RANK)(((int)eCurRank)+1);
		}
	}

	if ((pButtonPanel = UIComponentFindChildByName(pPanel, "gm email buttons")) != NULL)
	{
		eMinRank = emailMinRank;
		eCurRank = (GUILD_RANK)1;
		pButton = NULL;
		while ((pButton = UIComponentIterateChildren(pButtonPanel, pButton, UITYPE_BUTTON, FALSE)) != NULL)
		{
			UIButtonSetDown(UICastToButton(pButton), (eCurRank >= eMinRank));
			UIComponentSetActive(pButton, bIsGuildLeader && eCurRank != GUILD_RANK_LEADER && eCurRank != GUILD_RANK_RECRUIT);
			UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
			eCurRank = (GUILD_RANK)(((int)eCurRank)+1);
		}
	}

	// initialize ok/cancel/save buttons

	if ((pButton = UIComponentFindChildByName(pPanel, "button gm ok")) != NULL)
	{
		UIComponentSetActive(pButton, !bIsGuildLeader);
		UIComponentSetVisible(pButton, !bIsGuildLeader);
	}

	if ((pButton = UIComponentFindChildByName(pPanel, "button gm cancel")) != NULL)
	{
		UIComponentSetActive(pButton, bIsGuildLeader);
		UIComponentSetVisible(pButton, bIsGuildLeader);
	}

	if ((pButton = UIComponentFindChildByName(pPanel, "button gm save settings")) != NULL)
	{
		UIComponentSetActive(pButton, bIsGuildLeader);
		UIComponentSetVisible(pButton, bIsGuildLeader);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildPermissionButtonOnClick( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_COMPONENT * pButtonPanel = component->m_pParent;
	ASSERT_RETVAL(pButtonPanel, UIMSG_RET_NOT_HANDLED);

	const BOOL bDown = UIButtonGetDown(UICastToButton(component));
	BOOL bSetDown = FALSE;

	UI_COMPONENT * pButton = NULL;
	while ((pButton = UIComponentIterateChildren(pButtonPanel, pButton, UITYPE_BUTTON, FALSE)) != NULL)
	{
		if (pButton == component)
		{
			UIButtonSetDown(UICastToButton(pButton), !bDown);
			bSetDown = TRUE;
		}
		else
		{
			UIButtonSetDown(UICastToButton(pButton), bSetDown);
		}

		UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitComponentTypesSocial(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize)
{	

UI_XML_ONFUNC gUIXmlFunctions[] =
	{	// function name							function pointer
		{ "UISocialEditOnChar",						UISocialEditOnChar },
		{ "UIBuddyListPanelOnPostActivate",			UIBuddyListPanelOnPostActivate },
		{ "UIBuddyListPanelOnPostInactivate",		UIBuddyListPanelOnPostInactivate },
		{ "UIBuddyListOnActivate",					UIBuddyListOnActivate },
		{ "UIBuddyListOnPostActivate",				UIBuddyListOnPostActivate },
		{ "UIBuddyListOnPaint",						UIBuddyListOnPaint },
		{ "UIOpenAddBuddyPanel",					UIOpenAddBuddyPanel },
		{ "UIAddBuddyOnClick",						UIAddBuddyOnClick },
		{ "UIBuddyListCancelOnClick",				UIBuddyListCancelOnClick },
		{ "UIBuddyListOnLClick",					UIBuddyListOnLClick },
		{ "UIBuddyListOnRButtonDown",				UIBuddyListOnRButtonDown },
		{ "UIBuddyListOnLDoubleClick",				UIBuddyListOnLDoubleClick },
		{ "UIOpenRemoveBuddyPanel",					UIOpenRemoveBuddyPanel },
		{ "UIRemoveBuddyOnClick",					UIRemoveBuddyOnClick },
		{ "UIOpenSetNickBuddyPanel",				UIOpenSetNickBuddyPanel },
		{ "UISetNickBuddyOnClick",					UISetNickBuddyOnClick },
		{ "UIPendingAcceptBuddyOnClick",			UIPendingAcceptBuddyOnClick },
		{ "UIPendingDeclineBuddyOnClick",			UIPendingDeclineBuddyOnClick },
		{ "UIWhisperBuddyOnClick",					UIWhisperBuddyOnClick },
//		{ "UICloseBuddyPanel",						UICloseBuddyPanel },
		{ "UIToggleBuddyList",						UIToggleBuddyList },
		{ "UIGuildListPanelOnPostActivate",			UIGuildListPanelOnPostActivate },
		{ "UIGuildListOnPostActivate",				UIGuildListOnPostActivate },
		{ "UIGuildListOnPaint",						UIGuildListOnPaint },
		{ "UIOpenGuildInvitePanel",					UIOpenGuildInvitePanel },
		{ "UIGuildListOnLClick",					UIGuildListOnLClick },
		{ "UIGuildListOnActivate",					UIGuildListOnActivate },
		{ "UIGuildInviteOnClick",					UIGuildInviteOnClick },
		{ "UIGuildLeaveOnClick",					UIGuildLeaveOnClick },
		{ "UIGuildListOnRButtonDown",				UIGuildListOnRButtonDown },
		{ "UIGuildListOnLDoubleClick",				UIGuildListOnLDoubleClick },
		{ "UIToggleGuildPanel",						UIToggleGuildPanel },
		{ "UITogglePartyPanel",						UITogglePartyPanel },
		{ "UIGuildToggleManagementBtnOnClick",		UIGuildToggleManagementBtnOnClick },
		{ "UIGuildManagementCancelOnClick",			UIGuildManagementCancelOnClick },
		{ "UIGuildManagementSaveOnClick",			UIGuildManagementSaveOnClick },
		{ "UIGuildManagementOnPostVisible",			UIGuildManagementOnPostVisible },
		{ "UIGuildPermissionButtonOnClick",			UIGuildPermissionButtonOnClick },
	};

	// Add on the message handler functions for the local components
	int nOldSize = nXmlFunctionsSize;
	nXmlFunctionsSize += sizeof(gUIXmlFunctions);
	pUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, pUIXmlFunctions, nXmlFunctionsSize);
	memcpy((BYTE *)pUIXmlFunctions + nOldSize, gUIXmlFunctions, sizeof(gUIXmlFunctions));
}

#endif


