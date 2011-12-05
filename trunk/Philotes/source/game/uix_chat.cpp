//----------------------------------------------------------------------------
// uix_chat.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "uix.h"
#include "uix_chat.h"
#include "ChatServer.h"
#include "chat.h"
#include "console.h"
#include "console_priv.h"
#include "c_input.h"
#include "uix_components.h"
#include "player.h"
#include "gameoptions.h"
#include "settings.h"
#include "settingsexchange.h"
#include "gameoptions.h"
#include "e_settings.h"
#include "c_Camera.h"
#include "items.h"
#include "consolecmd.h"
#include "uix_scheduler.h"
#include "uix_hypertext.h"


#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UI_MSG_RETVAL UIChatMinimizeOnClick( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIChatOnActivate( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIChatOnInactivate( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UISetChatActive( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UITextEntryBackgroundOnClick( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIChatTabOnClick( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIChatFadeIn( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIChatFadeOut( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUISetBlinky(
	 void)
{
	if (!UIComponentGetVisibleByEnum(UICOMP_CHAT_TEXT))
	{
		UI_COMPONENT *pMinButton = UIComponentGetByEnum(UICOMP_CHAT_MINIMIZE_BUTTON);
		if (pMinButton)
		{
			UI_COMPONENT *pBlinky = UIComponentFindChildByName(pMinButton, "incoming chat notify");
			if (pBlinky)
			{
				UIComponentSetVisible(pBlinky, TRUE);
				int nDuration = MSECS_PER_SEC * 5;
				UIComponentBlink(pBlinky, &nDuration, NULL, TRUE);
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIChatUpdateScrollBottomButtons(
	void)
{
	UI_COMPONENT *pChatPanel = UIComponentGetByEnum(UICOMP_CHAT_TEXT);
	if (pChatPanel)
	{
		UI_COMPONENT * pScrollBottomButton = NULL;
		while ((pScrollBottomButton = UIComponentIterateChildren(pChatPanel, pScrollBottomButton, UITYPE_BUTTON, TRUE)) != NULL)
		{
			if (!PStrICmp(pScrollBottomButton->m_szName, "scroll bottom btn") && pScrollBottomButton->m_pParent)
			{
				UI_COMPONENT * pScrollBar = UIComponentIterateChildren(pScrollBottomButton->m_pParent, NULL, UITYPE_SCROLLBAR, TRUE);
				BOOL bVisible = pScrollBar ? UIComponentGetVisible(pScrollBar) : FALSE;
				UIComponentSetVisible(pScrollBottomButton, bVisible);
				UIComponentSetActive(pScrollBottomButton, bVisible);
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAddChatLineArgs(
	CHAT_TYPE eChatType,
	DWORD dwColor,
	const WCHAR * szFormat,
	...)
{
	va_list args;
	va_start(args, szFormat);

	WCHAR szBuf[CONSOLE_MAX_STR];
	PStrVprintf(szBuf, CONSOLE_MAX_STR, szFormat, args);

	UIAddChatLine(eChatType, dwColor, szBuf);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAddChatLine(
	CHAT_TYPE eChatType,
	DWORD dwColor,
	const WCHAR * szBuf,
	UI_LINE * pLine /* = NULL */)
{
	ASSERT_RETURN(szBuf);
	if( AppIsHellgate() )
	{
		const BOOL bOnAllTabs =	(
			eChatType == CHAT_TYPE_SERVER		||
			eChatType == CHAT_TYPE_CSR			||
			eChatType == CHAT_TYPE_WHISPER		||
			eChatType == CHAT_TYPE_ANNOUNCEMENT	||
			eChatType == CHAT_TYPE_CLIENT_ERROR );

		UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_MAIN_CHAT);
		if (pComp)
		{
			UI_TEXTBOX *pTextBox = UICastToTextBox(pComp);
			UITextBoxAddLine(pTextBox, szBuf, dwColor, 0, pLine, (pLine == NULL));
			sUISetBlinky();
		}

		pComp = UIComponentGetByEnum(UICOMP_LOCAL_CHAT);
		if (pComp && eChatType != CHAT_TYPE_PARTY && eChatType != CHAT_TYPE_GUILD)
		{
			UI_TEXTBOX *pTextBox = UICastToTextBox(pComp);
			UITextBoxAddLine(pTextBox, szBuf, dwColor, 0, pLine, (pLine == NULL));
			sUISetBlinky();
		}

		pComp = UIComponentGetByEnum(UICOMP_PARTY_CHAT);
		if (pComp && (eChatType == CHAT_TYPE_PARTY || bOnAllTabs))
		{
			UI_TEXTBOX *pTextBox = UICastToTextBox(pComp);
			UITextBoxAddLine(pTextBox, szBuf, dwColor, 0, pLine, (pLine == NULL));
			sUISetBlinky();
		}

		pComp = UIComponentGetByEnum(UICOMP_GUILD_CHAT);
		if (pComp && (eChatType == CHAT_TYPE_GUILD || bOnAllTabs))
		{
			UI_TEXTBOX *pTextBox = UICastToTextBox(pComp);
			UITextBoxAddLine(pTextBox, szBuf, dwColor, 0, pLine, (pLine == NULL));
			sUISetBlinky();
		}

		pComp = UIComponentFindChildByName(UIComponentGetByEnum(UICOMP_CHAT_TEXT), "csr chat");
		if (pComp && (eChatType == CHAT_TYPE_CSR || bOnAllTabs))
		{
			UI_TEXTBOX *pTextBox = UICastToTextBox(pComp);
			UITextBoxAddLine(pTextBox, szBuf, dwColor, 0, pLine, (pLine == NULL));
			sUISetBlinky();
		}

		if (eChatType == CHAT_TYPE_ANNOUNCEMENT || eChatType == CHAT_TYPE_WHISPER)
		{
			UIChatOpen();
		}
	}
	else
	{
		const BOOL bOnAllTabs =	(
			eChatType == CHAT_TYPE_CSR			||
			eChatType == CHAT_TYPE_WHISPER		||
			eChatType == CHAT_TYPE_ANNOUNCEMENT	||
			eChatType == CHAT_TYPE_CLIENT_ERROR );

		UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_MAIN_CHAT);
		if (pComp)
		{
			UI_TEXTBOX *pTextBox = UICastToTextBox(pComp);
			UITextBoxAddLine(pTextBox, szBuf, dwColor, 0, pLine, (pLine == NULL));
			sUISetBlinky();
		}

		pComp = UIComponentGetByEnum(UICOMP_LOCAL_CHAT);
		if (pComp && eChatType != CHAT_TYPE_PARTY && eChatType != CHAT_TYPE_GUILD && eChatType != CHAT_TYPE_SERVER && eChatType != CHAT_TYPE_SHOUT)
		{
			UI_TEXTBOX *pTextBox = UICastToTextBox(pComp);
			UITextBoxAddLine(pTextBox, szBuf, dwColor, 0, pLine, (pLine == NULL));
			sUISetBlinky();
		}

		pComp = UIComponentGetByEnum(UICOMP_PARTY_CHAT);
		if (pComp && (eChatType == CHAT_TYPE_PARTY || bOnAllTabs))
		{
			UI_TEXTBOX *pTextBox = UICastToTextBox(pComp);
			UITextBoxAddLine(pTextBox, szBuf, dwColor, 0, pLine, (pLine == NULL));
			sUISetBlinky();
		}

		pComp = UIComponentGetByEnum(UICOMP_GUILD_CHAT);
		if (pComp && (eChatType == CHAT_TYPE_GUILD || bOnAllTabs))
		{
			UI_TEXTBOX *pTextBox = UICastToTextBox(pComp);
			UITextBoxAddLine(pTextBox, szBuf, dwColor, 0, pLine, (pLine == NULL));
			sUISetBlinky();
		}

		pComp = UIComponentFindChildByName(UIComponentGetByEnum(UICOMP_CHAT_TEXT), "csr chat");
		if (pComp && eChatType == CHAT_TYPE_CSR )
		{
			UI_TEXTBOX *pTextBox = UICastToTextBox(pComp);
			UITextBoxAddLine(pTextBox, szBuf, dwColor, 0, pLine, (pLine == NULL));
			sUISetBlinky();
		}

	}

	sUIChatUpdateScrollBottomButtons();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAddServerChatLine(
	const WCHAR * format)
{
	UIAddChatLine( 
		CHAT_TYPE_SERVER, 
		ChatGetTypeColor( CHAT_TYPE_SERVER ),
		format);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIChatInit(
	void)
{
	UIChatSetAutoFadeState(UIChatAutoFadeEnabled());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIChatSessionStart(
	void)
{
	UIComponentActivate(UICOMP_CHAT_PANEL, TRUE);

	UIChatRemoveAllTabs();

	if (AppIsMultiplayer())
	{
		//UIChatAddTab("ui chat tab global", "chat tab global", CHAT_TYPE_GAME, ~(0));
		//UIChatAddTab("ui chat tab local", "chat tab local", CHAT_TYPE_GAME, ~(CHAT_TYPE_PARTY|CHAT_TYPE_GUILD));
		//UIChatAddTab("ui chat tab party", "chat tab party", CHAT_TYPE_PARTY, CHAT_TYPE_PARTY);
		//UIChatAddTab("ui chat tab guild", "chat tab guild", CHAT_TYPE_GUILD, CHAT_TYPE_GUILD);
		UIChatOpen(FALSE);
	}
	else
	{
		//UIChatAddTab("ui chat tab global", "chat tab global", CHAT_TYPE_GAME, ~(0));
		UIChatClose(FALSE);
	}

	UIChatTabActivate(CHAT_TAB_GLOBAL);

	UICSRPanelClose();
	ConsoleSetCSRPlayerName(L"");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIChatFree()
{
	UIChatRemoveAllTabs();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatOpen(
	BOOL bUserActive /*=TRUE*/)
{
	UI_COMPONENT *pChat = UIComponentGetByEnum(UICOMP_CHAT_PANEL);
	ASSERT_RETVAL(pChat, UIMSG_RET_NOT_HANDLED);

	UIComponentActivate( pChat, TRUE );
	UIComponentActivate( UICOMP_CHAT_TABS, TRUE );
	UIComponentActivate( UICOMP_CHAT_TEXT, TRUE );
	UIComponentSetVisibleByEnum(UICOMP_CHAT_TABS, TRUE);

	if (bUserActive)
	{
		UIComponentSetVisibleByEnum( UICOMP_CHAT_TEXT, TRUE );
		pChat->m_bUserActive = TRUE;
	}
	else
	{
		UIComponentSetVisible( pChat, TRUE );
		ConsoleSetEditActive( FALSE );
	}

	UI_COMPONENT *pMinimizeButton = UIComponentGetByEnum(UICOMP_CHAT_MINIMIZE_BUTTON);
	if (pMinimizeButton)
	{
		UI_BUTTONEX *pButton = UICastToButton(pMinimizeButton);
		if (UIButtonGetDown(pButton))
		{
			UIButtonSetDown(pButton, FALSE);
			UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
		}
	}

	UI_COMPONENT *pBlinky = UIComponentGetById(UIComponentGetIdByName("incoming chat notify"));
	if (pBlinky)
	{
		UIComponentSetVisible(pBlinky, FALSE);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatClose(
	BOOL bUserActive /*=TRUE*/)
{
	UI_COMPONENT *pChat = UIComponentGetByEnum(UICOMP_CHAT_PANEL);
	ASSERT_RETVAL(pChat, UIMSG_RET_NOT_HANDLED);

	UIComponentActivate( pChat, FALSE );

	if (!bUserActive)
	{
		UIComponentSetVisible( pChat, FALSE );
	}

	ConsoleSetEditActive( FALSE );
	pChat->m_bUserActive = FALSE;

	UI_COMPONENT *pMinimizeButton = UIComponentGetByEnum(UICOMP_CHAT_MINIMIZE_BUTTON);
	if (pMinimizeButton)
	{
		UI_BUTTONEX *pButton = UICastToButton(pMinimizeButton);
		if (!UIButtonGetDown(pButton))
		{
			UIButtonSetDown(pButton, TRUE);
			UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UI_BUTTONEX * sGetChatTabByIdx(int nIndex)
{
	UI_COMPONENT *pChatPanel = UIComponentGetByEnum(UICOMP_CHAT_TABS);
	ASSERT_RETNULL(pChatPanel);

	UI_COMPONENT *pChatTab = UIComponentIterateChildren(pChatPanel, NULL, UITYPE_BUTTON, TRUE);
	while (pChatTab)
	{
		if (!(nIndex--))
		{
			return UICastToButton(pChatTab);
		}
		pChatTab = UIComponentIterateChildren(pChatPanel, pChatTab, UITYPE_BUTTON, TRUE);
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UI_TEXTBOX * sGetChatTextByIdx(int nIndex)
{
	UI_COMPONENT *pChatPanel = UIComponentGetByEnum(UICOMP_CHAT_TEXT);
	ASSERT_RETNULL(pChatPanel);

	UI_COMPONENT *pChatText = UIComponentIterateChildren(pChatPanel, NULL, UITYPE_TEXTBOX, TRUE);
	while (pChatText)
	{
		if (!(nIndex--))
		{
			return UICastToTextBox(pChatText);
		}
		pChatText = UIComponentIterateChildren(pChatPanel, pChatText, UITYPE_TEXTBOX, TRUE);
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIGetChatTabIdx(void)
{
	UI_COMPONENT *pChatPanel = UIComponentGetByEnum(UICOMP_CHAT_TEXT);
	ASSERT_RETNULL(pChatPanel);

	int nIndex = 0;

	UI_COMPONENT *pChatText = UIComponentIterateChildren(pChatPanel, NULL, UITYPE_TEXTBOX, TRUE);
	while (pChatText)
	{
		if (UIComponentGetVisible(pChatText))
		{
			return nIndex;
		}
		++nIndex;
		pChatText = UIComponentIterateChildren(pChatPanel, pChatText, UITYPE_TEXTBOX, TRUE);
	}

	return -1;
}

void UINextChatTab(void)
{
	int nIndex = UIGetChatTabIdx();
	if (nIndex == -1)
	{
		UIChatTabActivate(CHAT_TAB_GLOBAL);
	}
	else
	{
		if (++nIndex > 3)
		{
			nIndex = 0;
		}
		UIChatTabActivate(nIndex);
	}
}

void UIPrevChatTab(void)
{
	int nIndex = UIGetChatTabIdx();
	if (nIndex == -1)
	{
		UIChatTabActivate(CHAT_TAB_GLOBAL);
	}
	else
	{
		if (--nIndex < 0)
		{
			nIndex = 3;
		}
		UIChatTabActivate(nIndex);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_CHAT_TABS	16

struct CHAT_TAB
{
	UI_COMPONENT *	m_pButton;
	UI_COMPONENT *	m_pText;
	CHAR			m_szLabel[MAX_STRING_KEY_LENGTH];
	CHAT_TYPE		m_eChatContext;
	int				m_nChatTypes;
};

static CHAT_TAB * s_pChatTabs[MAX_CHAT_TABS] = {NULL};
static int s_iNumChatTabs = 0;

BOOL UIChatAddTab(
	LPCSTR szLabel,
	LPCSTR szComponentName,
	CHAT_TYPE eChatContext,
	int nChatTypes)
{
	ASSERT_RETFALSE(s_iNumChatTabs+1 < MAX_CHAT_TABS);
	ASSERT_RETFALSE(!s_pChatTabs[s_iNumChatTabs]);

	char szTempName[256];

	CHAT_TAB * pTab = s_pChatTabs[s_iNumChatTabs] = (CHAT_TAB*)MALLOCZ(NULL, sizeof(CHAT_TAB));
	ASSERT_RETFALSE(pTab);

	PStrCopy(pTab->m_szLabel, szLabel, arrsize(pTab->m_szLabel));
	pTab->m_eChatContext = eChatContext;
	pTab->m_nChatTypes = nChatTypes;

	UI_COMPONENT *pChatTabs = UIComponentGetByEnum(UICOMP_CHAT_TABS);
	ASSERT_RETFALSE(pChatTabs);

	// add tab and label

	PStrPrintf(szTempName, sizeof(szTempName), "%s button", szComponentName);
	pTab->m_pButton = UIXmlLoadBranch(L"data//uix//xml//Console.xml", "chat tab 1", szTempName, pChatTabs);
	ASSERT_RETFALSE(pTab->m_pButton);

	pTab->m_pButton->m_Position.m_fY += 30.0f;
	UIComponentHandleUIMessage(pTab->m_pButton, UIMSG_PAINT, 0, 0);

	// add text panel, textbox, and scroll bottom button

	PStrPrintf(szTempName, sizeof(szTempName), "%s text", szComponentName);
	pTab->m_pText = UIXmlLoadBranch(L"data//uix//xml//Console.xml", "main chat panel", szTempName, pChatTabs);
	ASSERT_RETFALSE(pTab->m_pText);

	++s_iNumChatTabs;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIChatRemoveTab(
	int nTab)
{
	ASSERT_RETFALSE(nTab >= 0);
	ASSERT_RETFALSE(nTab < s_iNumChatTabs);
	ASSERT_RETFALSE(s_pChatTabs[nTab]);

	CHAT_TAB * pTab = s_pChatTabs[nTab];
	UIComponentFree(pTab->m_pButton);
	UIComponentFree(pTab->m_pText);
	FREE(NULL, pTab);

	while (nTab<s_iNumChatTabs-1)
	{
		s_pChatTabs[nTab] = s_pChatTabs[++nTab];
	}

	s_pChatTabs[nTab] = NULL;
	--s_iNumChatTabs;

	return TRUE;
}

BOOL UIChatRemoveTab(
	CHAT_TAB * pTab)
{
	ASSERT_RETFALSE(pTab);

	for (int i=0; i<s_iNumChatTabs; ++i)
	{
		if (s_pChatTabs[i] == pTab)
		{
			return UIChatRemoveTab(i);
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIChatRemoveAllTabs(
	void)
{
	int iCount = 0;
	while (s_iNumChatTabs>0)
	{
		ASSERT_RETURN(iCount++ < MAX_CHAT_TABS);
		UIChatRemoveTab(s_iNumChatTabs-1);
	}	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIChatTabIsVisible(const int nIndex)
{
	UI_TEXTBOX *pChatText = sGetChatTextByIdx(nIndex);
	ASSERT_RETFALSE(pChatText);
	return UIComponentGetVisible((UI_COMPONENT*)pChatText);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIChatTabActivate(const int nIndex)
{
	UI_BUTTONEX *pChatTab = sGetChatTabByIdx(nIndex);
	ASSERT_RETURN(pChatTab);
	UIComponentActivate((UI_COMPONENT*)pChatTab, TRUE);
	if( AppIsHellgate() )
		UIComponentHandleUIMessage((UI_COMPONENT*)pChatTab, UIMSG_LBUTTONDOWN, 1, 0, FALSE);
	else
		UIComponentHandleUIMessage((UI_COMPONENT*)pChatTab, UIMSG_LBUTTONCLICK, 1, 0, FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIChatSetContextBasedOnTab(void)
{
	const int iTab = UIGetChatTabIdx();
	if (iTab > 0)
	{
		UIChatTabActivate(iTab);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UICSRPanelOpen()
{
	UIChatOpen();

	UI_COMPONENT *pButton;
	UI_COMPONENT *pChatPanel = UIComponentGetByEnum(UICOMP_CHAT_TABS);
	ASSERT_RETURN(pChatPanel);

	//pButton = UIComponentFindChildByName(pChatPanel, "chat tab 1");
	//ASSERT_RETURN(pButton);
	//UIComponentSetVisible(pButton, FALSE);

	//pButton = UIComponentFindChildByName(pChatPanel, "chat tab 2");
	//ASSERT_RETURN(pButton);
	//UIComponentSetVisible(pButton, FALSE);

	//pButton = UIComponentFindChildByName(pChatPanel, "chat tab 3");
	//ASSERT_RETURN(pButton);
	//UIComponentSetVisible(pButton, FALSE);

	//pButton = UIComponentFindChildByName(pChatPanel, "chat tab 4");
	//ASSERT_RETURN(pButton);
	//UIComponentSetVisible(pButton, FALSE);

	pButton = UIComponentFindChildByName(pChatPanel, "chat tab csr");
	ASSERT_RETURN(pButton);
	UIComponentActivate(pButton, TRUE);
	UIComponentHandleUIMessage(pButton, UIMSG_LBUTTONDOWN, 1, 0, FALSE);

	//UI_COMPONENT * pComponent = UIComponentGetByEnum(UICOMP_CHAT_MINIMIZE_BUTTON);
	//ASSERT_RETURN(pComponent);
	//pButton = UICastToButton(pComponent);
	//UIComponentSetActive(pButton, FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UICSRPanelClose()
{
	UI_COMPONENT *pButton;
	UI_COMPONENT *pChatPanel = UIComponentGetByEnum(UICOMP_CHAT_TABS);
	ASSERT_RETURN(pChatPanel);

	//pButton = UIComponentFindChildByName(pChatPanel, "chat tab 1");
	//ASSERT_RETURN(pButton);
	//UIComponentActivate(pButton, TRUE);
	//UIComponentHandleUIMessage(pButton, UIMSG_LBUTTONDOWN, 1, 0, FALSE);

	//pButton = UIComponentFindChildByName(pChatPanel, "chat tab 2");
	//ASSERT_RETURN(pButton);
	//UIComponentActivate(pButton, TRUE);

	//pButton = UIComponentFindChildByName(pChatPanel, "chat tab 3");
	//ASSERT_RETURN(pButton);
	//UIComponentActivate(pButton, TRUE);

	//pButton = UIComponentFindChildByName(pChatPanel, "chat tab 4");
	//ASSERT_RETURN(pButton);
	//UIComponentActivate(pButton, TRUE);

	pButton = UIComponentFindChildByName(pChatPanel, "chat tab csr");
	ASSERT_RETURN(pButton);
	UI_BUTTONEX *pButtonCSR = UICastToButton(pButton);
	if (UIButtonGetDown(pButtonCSR))
	{
		UI_COMPONENT *pButton1 = UIComponentFindChildByName(pChatPanel, "chat tab 1");
		ASSERT_RETURN(pButton1);
		UIComponentHandleUIMessage(pButton1, UIMSG_LBUTTONDOWN, 1, 0, FALSE);
	}
	UIComponentSetVisible(pButton, FALSE);

	//UI_COMPONENT * pComponent = UIComponentGetByEnum(UICOMP_CHAT_MINIMIZE_BUTTON);
	//ASSERT_RETURN(pComponent);
	//pButton = UICastToButton(pComponent);
	//UIComponentSetActive(pButton, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UICSRPanelIsOpen()
{
	UI_COMPONENT *pChatPanel = UIComponentGetByEnum(UICOMP_CHAT_TABS);
	ASSERT_RETFALSE(pChatPanel);
	UI_COMPONENT *pButton = UIComponentFindChildByName(pChatPanel, "chat tab csr");
	ASSERT_RETFALSE(pButton);
	return UIComponentGetVisible(pButton);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatMinimizeOnClick( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UNIT * pPlayer = UIGetControlUnit();
	ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX *pButton = UICastToButton(pComponent); 
	if (UIButtonGetDown(pButton))
	{
		if (!UIComponentGetVisibleByEnum(UICOMP_CHAT_TEXT))
		{
			UIButtonSetDown(pButton, FALSE);
			UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
			//UnitSetStat( pPlayer, STATS_HIDE_CHAT, FALSE );
			return UIChatOpen();
		}
		else
		{
			//UnitSetStat( pPlayer, STATS_HIDE_CHAT, TRUE );
			return UIChatClose();
		}
	}
	else
	{
		//UnitSetStat( pPlayer, STATS_HIDE_CHAT, FALSE );
		return UIChatOpen();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIChatClear()
{
	UI_COMPONENT *pChat = UIComponentGetByEnum(UICOMP_MAIN_CHAT);
	ASSERT_RETURN(pChat);
	UITextBoxClear(pChat);

	pChat = UIComponentGetByEnum(UICOMP_LOCAL_CHAT);
	if (pChat) UITextBoxClear(pChat);

	pChat = UIComponentGetByEnum(UICOMP_PARTY_CHAT);
	if (pChat) UITextBoxClear(pChat);

	pChat = UIComponentGetByEnum(UICOMP_GUILD_CHAT);
	if (pChat) UITextBoxClear(pChat);

	pChat = UIComponentFindChildByName(UIComponentGetByEnum(UICOMP_CHAT_TEXT), "csr chat");
	if (pChat) UITextBoxClear(pChat);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIChatIsOpen()
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_CHAT_MINIMIZE_BUTTON);
	ASSERT_RETFALSE(pComp);

	UI_BUTTONEX *pButton = UICastToButton(pComp);
	return !UIButtonGetDown(pButton);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatOnActivate(
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam )
{
	UIComponentOnActivate(pComponent, nMessage, wParam, lParam);

	UI_COMPONENT *pBackground = UIComponentFindChildByName(pComponent, "chat background");
	if (pBackground)
	{
		// show the background if any left panels are visible
		BOOL bVisible = FALSE;
		PList<UI_COMPONENT *>::USER_NODE *pNode = NULL;
		while ((pNode = g_UI.m_listAnimCategories[UI_ANIM_CATEGORY_LEFT_PANEL].GetNext(pNode)) != NULL)
		{
			if (UIComponentGetVisible(pNode->Value))
			{
				bVisible = TRUE;
				break;
			}
		}
		UIComponentSetVisible(pBackground, bVisible);
	}
	/*
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_CHAT_MINIMIZE_BUTTON);
	if (pComp)
	{
	UI_BUTTONEX *pButton = UICastToButton(pComp);

	if (UIButtonGetDown(pButton))
	{
	UIButtonSetDown(pButton, FALSE);
	UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
	}
	}
	*/
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITextEntryBackgroundOnClick(
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam )
{
	if (UIComponentCheckBounds(pComponent))
	{
		UIChatSetContextBasedOnTab();
		ConsoleActivateEdit(FALSE, TRUE);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}
	else
	{
		return UIMSG_RET_NOT_HANDLED;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatTabOnClick(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	if ((UIComponentCheckBounds(pComponent) && UIComponentGetVisible(pComponent)) || wParam)
	{
		UI_BUTTONEX * pButton = UICastToButton(pComponent);

		//if (pButton->m_dwPushstate & UI_BUTTONSTATE_DOWN)
		//{
		//	return UIMSG_RET_HANDLED;
		//}

		switch (pButton->m_nAssocTab)
		{
		default:
			ConsoleSetChatContext(CHAT_TYPE_GAME);
			break;
		case 2: //party tab
			ConsoleSetChatContext(CHAT_TYPE_PARTY);
			break;
		case 3: //guild tab
			ConsoleSetChatContext(CHAT_TYPE_GUILD);
			break;
		case 4: //csr tab
			ConsoleSetChatContext(CHAT_TYPE_CSR);
			break;
		};

		if (UIComponentGetVisibleByEnum(UICOMP_CONSOLE_EDIT))
		{
			ConsoleActivateEdit(FALSE, TRUE);
		}

		UI_COMPONENT * pChatText = UIComponentGetByEnum(UICOMP_CHAT_TEXT_PANELS);
		if (pChatText)
		{
			UIPanelSetTab(UICastToPanel(pChatText), pButton->m_nAssocTab);
			UI_MSG_RETVAL ret = UIButtonOnButtonDown(pComponent, nMessage, wParam, lParam );
			sUIChatUpdateScrollBottomButtons();
			return ret;
		}
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatHelpButtonOnClick(
   UI_COMPONENT *pComponent,
   int nMessage,
   DWORD wParam,
   DWORD lParam )
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentCheckBounds(pComponent) && !wParam)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_COMPONENT * pChatPanel = UIComponentGetByEnum(UICOMP_CHAT_PANEL);
	ASSERT_RETVAL(pChatPanel, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT * pButtonPanel = UIComponentFindChildByName(pChatPanel, "chat help buttonpanel");
	if (pButtonPanel && !wParam)
	{
		UI_COMPONENT * pThisButton = NULL;
		while ((pThisButton = UIComponentIterateChildren(pButtonPanel, pThisButton, UITYPE_BUTTON, TRUE)) != NULL)
		{
			if (pThisButton != pComponent)
			{
				UIButtonSetDown(UICastToButton(pThisButton), FALSE);
				UIChatHelpButtonOnClick(pThisButton, UIMSG_LBUTTONCLICK, TRUE, lParam);
				UIComponentHandleUIMessage(pThisButton, UIMSG_PAINT, 0, 0);
			}
		}
	}

	UI_COMPONENT *pListBox = NULL;
	if (!PStrICmp(pComponent->m_szName, "chat help button"))
	{
		pListBox = UIComponentFindChildByName(pChatPanel, "chat help panel");
	}
	else if (!PStrICmp(pComponent->m_szName, "chat emote button"))
	{
		pListBox = UIComponentFindChildByName(pChatPanel, "chat emote panel");
	}

	if (pListBox)
	{
		BOOL bActive = UIButtonGetDown(UICastToButton(pComponent));
		UIComponentSetVisible(pListBox, bActive);
		UIComponentSetActive(pListBox, bActive);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * StringTableGetStringByGlobalKey(
	CHAR * szKey)
{
	ASSERT_RETNULL(szKey);
	ASSERT_RETNULL(szKey[0]);

	int nNumStrings = ExcelGetNumRows(EXCEL_CONTEXT(), DATATABLE_GLOBAL_STRING);
	for (int i = 0; i < nNumStrings; ++i)
	{
		GLOBAL_INDEX_ENTRY* pGlobalIndexEntry = (GLOBAL_INDEX_ENTRY*)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_GLOBAL_STRING, i);
		if (!pGlobalIndexEntry) continue;

		if (!PStrICmp(szKey, pGlobalIndexEntry->pszName))
		{
			const WCHAR * wszString = StringTableGetStringByKey(pGlobalIndexEntry->pszString);
			if (wszString)
			{
				return wszString;
			}
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatHelpPanelOnClick(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentCheckBounds(pComponent) || !UIComponentGetVisible(pComponent))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_LISTBOX * pListBox = UICastToListBox(pComponent);

	UI_MSG_RETVAL UIListBoxOnMouseDown(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);
	UIListBoxOnMouseDown(pComponent, nMessage, wParam, lParam);

	CMDMENU_DATA * pCmdMenuData = (CMDMENU_DATA*)UIListBoxGetSelectedData(pListBox);

	pListBox->m_nSelection = INVALID_LINK;
	UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);

//	trace("command = '%s' immediate = %d\n", pCmdMenuData->szChatCommand, (int)pCmdMenuData->bImmediate);

	ASSERT_RETVAL(pCmdMenuData, UIMSG_RET_NOT_HANDLED);
	const WCHAR * wszCmd = StringTableGetStringByGlobalKey(pCmdMenuData->szChatCommand);
	WCHAR wszSlashCmd[CONSOLE_MAX_STR] = L"/";
	PStrCat(wszSlashCmd, wszCmd, arrsize(wszSlashCmd));
	PStrCat(wszSlashCmd, L" ", arrsize(wszSlashCmd));

	ConsoleActivateEdit(FALSE, TRUE);

	UI_COMPONENT *pEdit = UIComponentGetByEnum(UICOMP_CONSOLE_EDIT);
	if (pEdit)
	{
		UILabelSetText(UICastToEditCtrl(pEdit), wszSlashCmd);
	}

	if (pCmdMenuData->bImmediate)
	{
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatOnInactivate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam )
{
	UIComponentOnInactivate(pComponent, nMessage, wParam, lParam);
	/*
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_CHAT_MINIMIZE_BUTTON);
	if (pComp)
	{
	UI_BUTTONEX *pButton = UICastToButton(pComp);

	if (!UIButtonGetDown(pButton))
	{
	UIButtonSetDown(pButton, TRUE);
	UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
	}
	}
	*/
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISetChatActive( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam )
{
	UI_COMPONENT* pChat = UIComponentGetByEnum(UICOMP_CHAT_PANEL);
	if (!pChat)
		return UIMSG_RET_NOT_HANDLED;

	UI_COMPONENT *pButtonComponent = UIComponentGetById(UIComponentGetIdByName("chat minimize btn"));
	if (pButtonComponent)
	{
		UI_BUTTONEX *pButton = UICastToButton(pButtonComponent);
		if (UIButtonGetDown(pButton))
		{
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	if (nMessage == UIMSG_POSTACTIVATE || nMessage == UIMSG_POSTVISIBLE)
		UIComponentHandleUIMessage(pChat, UIMSG_INACTIVATE, 0, 0, FALSE);

	if (nMessage == UIMSG_POSTINACTIVATE || nMessage == UIMSG_POSTINVISIBLE)
		UIComponentHandleUIMessage(pChat, UIMSG_ACTIVATE, 0, 0, FALSE);


	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIChatSetAutoFadeState(
	const BOOL bAutoFade)
{
	if (bAutoFade)
	{
		if (!UIChatIsOpen())
		{
			UIComponentSetActiveByEnum(UICOMP_CHAT_PANEL, TRUE);
			UIChatFadeOut(TRUE, TRUE);
			UIComponentSetActiveByEnum(UICOMP_CHAT_PANEL, FALSE);
		}
		else
		{
			UIChatFadeOut(TRUE, TRUE);
		}
	}
	else
	{
		if (!UIChatIsOpen())
		{
			UIComponentSetActiveByEnum(UICOMP_CHAT_PANEL, TRUE);
			UIChatFadeIn(TRUE);
			UIComponentSetActiveByEnum(UICOMP_CHAT_PANEL, FALSE);
		}
		else
		{
			UIChatFadeIn(TRUE);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIChatAutoFadeEnabled(
	void)
{
	GAMEOPTIONS tOptions;
	structclear(tOptions);
	GameOptions_Get(tOptions);
	return tOptions.bChatAutoFade;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIChatFadeIn(
	const BOOL bImmediate)
{
	UI_COMPONENT *pChat = UIComponentGetByEnum(UICOMP_CHAT_PANEL);
	if (pChat)
	{
		UIChatFadeIn(pChat, NULL, (DWORD)bImmediate, 0);
	}
}

UI_MSG_RETVAL UIChatFadeIn( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	if (!UIComponentCheckBounds(pComponent) && nMessage)
	{
		return UIMSG_RET_HANDLED;
	}

	const BOOL bImmediate = (BOOL)(wParam & 0x1);

	UI_COMPONENT *pBackground = NULL;

	if (AppIsHellgate())
	{
		if (nMessage == UIMSG_MOUSEHOVERLONG && !UIChatIsOpen())
		{
			return UIMSG_RET_HANDLED;
		}

		UIComponentSetVisibleByEnum(UICOMP_CHAT_TABS, TRUE);

		for (int i=0; i<s_iNumChatTabs; ++i)
		{
			ASSERT_CONTINUE(s_pChatTabs[i]);
			pBackground = s_pChatTabs[i]->m_pButton;
			if (pBackground)
			{
				UIComponentActivate(pBackground, TRUE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}
		}

		pBackground = UIComponentFindChildByName(pComponent, "chat tab 1");
		if (pBackground)
		{
			UIComponentActivate(pBackground, TRUE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
		}

		if (!AppIsSinglePlayer())
		{
			pBackground = UIComponentFindChildByName(pComponent, "chat tab 2");
			if (pBackground)
			{
				UIComponentActivate(pBackground, TRUE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent, "chat tab 3");
			if (pBackground)
			{
				UIComponentActivate(pBackground, TRUE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent, "chat tab 4");
			if (pBackground)
			{
				UIComponentActivate(pBackground, TRUE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent, "chat tab csr");
			if (pBackground && PlayerInCSRChat())
			{
				UIComponentActivate(pBackground, TRUE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}
		}

		pBackground = UIComponentFindChildByName(pComponent->m_pParent, "chat text flexborder");
		if (pBackground)
		{
			UIComponentActivate(pBackground, TRUE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
		}

		pBackground = UIComponentFindChildByName(pComponent->m_pParent, "chat text resize");
		if (pBackground)
		{
			UIComponentActivate(pBackground, TRUE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
		}

		pBackground = UIComponentFindChildByName(pComponent->m_pParent, "chat help button");
		if (pBackground)
		{
			UIComponentActivate(pBackground, TRUE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
		}

		pBackground = UIComponentFindChildByName(pComponent->m_pParent, "chat emote button");
		if (pBackground)
		{
			UIComponentActivate(pBackground, TRUE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
		}
	}
	else
	{
		if( InputGetMouseButtonDown( 0 ) || InputGetMouseButtonDown( 1 ) )
		{
			return UIMSG_RET_HANDLED;
		}

		UI_COMPONENT *pBackground = UIComponentFindChildByName(pComponent, "chat background");
		if (pBackground)
		{
			UIComponentActivate(pBackground, TRUE);
		}

		pBackground = UIComponentFindChildByName(pComponent, "global tab");
		if (pBackground)
		{
			UIComponentActivate(pBackground, TRUE);
		}

		pBackground = UIComponentFindChildByName(pComponent, "local tab");
		if (pBackground)
		{
			UIComponentActivate(pBackground, TRUE);
		}

		pBackground = UIComponentFindChildByName(pComponent, "party tab");
		if (pBackground)
		{
			UIComponentActivate(pBackground, TRUE);
		}

		pBackground = UIComponentFindChildByName(pComponent, "guild tab");
		if (pBackground)
		{
			UIComponentActivate(pBackground, TRUE);
		}

		pBackground = UIComponentFindChildByName(pComponent, "chat tab csr");
		if (pBackground && PlayerInCSRChat() )
		{
			UIComponentActivate(pBackground, TRUE);
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIChatFadeOut(
	const BOOL bImmediate,
	const BOOL bForce /*=FALSE*/)
{
	UI_COMPONENT *pChat = UIComponentGetByEnum(UICOMP_CHAT_PANEL);
	if (pChat)
	{
		UIChatFadeOut(pChat, NULL, (DWORD)bImmediate | ((DWORD)bForce << 1), 0);
	}
}

UI_MSG_RETVAL UIChatFadeOut( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT* pMainScreen = UIComponentGetByEnum(UICOMP_MAINSCREEN);
	ASSERT_RETVAL(pMainScreen, UIMSG_RET_NOT_HANDLED);

	const BOOL bActive = UIComponentGetActive(pComponent);
	const BOOL bInBounds = UIComponentCheckBounds(pComponent) || UIComponentIsResizing();

	const BOOL bImmediate = (BOOL)(wParam & 0x1);
	const BOOL bForce = (BOOL)((wParam & 0x2) >> 1);

	if (bActive && (!bInBounds || !nMessage))
	{
		UI_COMPONENT *pBackground = NULL;

		if (AppIsHellgate())
		{
			if (!bForce)
			{					
				if (!UIChatAutoFadeEnabled())
				{
					return UIMSG_RET_HANDLED;
				}

				if (nMessage == UIMSG_MOUSEMOVE)
				{
					if (UIComponentGetVisibleByEnum(UICOMP_CHAT_TEXTENTRY_ACTIVE))
					{
						return UIMSG_RET_HANDLED;
					}

					UI_COMPONENT * pHelpPanel = UIComponentFindChildByName(pComponent, "chat help panel");
					if (pHelpPanel && UIComponentGetVisible(pHelpPanel))
					{
						return UIMSG_RET_HANDLED;
					}

					UI_COMPONENT * pEmotePanel = UIComponentFindChildByName(pComponent, "chat emote panel");
					if (pEmotePanel && UIComponentGetVisible(pEmotePanel))
					{
						return UIMSG_RET_HANDLED;
					}
				}
			}

			for (int i=0; i<s_iNumChatTabs; ++i)
			{
				ASSERT_CONTINUE(s_pChatTabs[i]);
				pBackground = s_pChatTabs[i]->m_pButton;
				if (pBackground)
				{
					UIComponentActivate(pBackground, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
				}
			}

			pBackground = UIComponentFindChildByName(pComponent, "chat tab 1");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent, "chat tab 2");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent, "chat tab 3");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent, "chat tab 4");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent, "chat tab csr");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent->m_pParent, "chat text flexborder");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent->m_pParent, "chat text resize");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent->m_pParent, "chat help button");
			if (pBackground)
			{
				UI_BUTTONEX * pButton = UICastToButton(pBackground);
				UIButtonSetDown(pButton, FALSE);
				UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);

				UIComponentActivate(pBackground, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent->m_pParent, "chat emote button");
			if (pBackground)
			{
				UI_BUTTONEX * pButton = UICastToButton(pBackground);
				UIButtonSetDown(pButton, FALSE);
				UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);

				UIComponentActivate(pBackground, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent->m_pParent, "chat help panel");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}

			pBackground = UIComponentFindChildByName(pComponent->m_pParent, "chat emote panel");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
			}
		}
		else
		{
			pBackground = UIComponentFindChildByName(pComponent, "chat background");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE);
			}

			pBackground = UIComponentFindChildByName(pComponent, "global tab");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE);
			}

			pBackground = UIComponentFindChildByName(pComponent, "local tab");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE);
			}

			pBackground = UIComponentFindChildByName(pComponent, "party tab");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE);
			}

			pBackground = UIComponentFindChildByName(pComponent, "guild tab");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE);
			}

			pBackground = UIComponentFindChildByName(pComponent, "chat tab csr");
			if (pBackground)
			{
				UIComponentActivate(pBackground, FALSE);
			}
		}
	}
	return UIMSG_RET_HANDLED;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static
void sChatSizeExchangeFunc(SettingsExchange & se, void * pContext)
{
	SettingsExchange_Do(se, ((UI_POSITION*)pContext)->m_fX, "ChatWidth");
	SettingsExchange_Do(se, ((UI_POSITION*)pContext)->m_fY, "ChatHeight");
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void AppLoadChatSize(UI_POSITION* pSize)
{
	Settings_Load("ChatSize", &sChatSizeExchangeFunc, pSize);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void AppSaveChatSize(UI_POSITION* pSize)
{
	Settings_Save("ChatSize", &sChatSizeExchangeFunc, pSize);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIChatFillCmdMenu(
	UI_LISTBOX * pListBox,
	const char * szCmdMenu)
{
	ASSERT_RETURN(pListBox);
	ASSERT_RETURN(szCmdMenu);

	float fWidth = pListBox->m_fWidth;
	float fHeight = 0.0f;
	UIX_TEXTURE_FONT * pFont = UIComponentGetFont(pListBox);
	int nFontSize = UIComponentGetFontSize(pListBox);

	float fTextHeight = 0.0f;
	float fLineHeight = 0.0f;
	UIElementGetTextLogSize(pFont, nFontSize, 1.0f, pListBox->m_bNoScaleFont, L"|", NULL, &fTextHeight);
	UIElementGetTextLogSize(pFont, nFontSize, 1.0f, pListBox->m_bNoScaleFont, L"|\n|", NULL, &fLineHeight);
	fLineHeight -= fTextHeight;

	int nNumCmds = ExcelGetNumRows(EXCEL_CONTEXT(), DATATABLE_CMD_MENUS);
	for(int i = 0; i < nNumCmds; ++i)
	{
		CMDMENU_DATA* pCmdMenuData = (CMDMENU_DATA*)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_CMD_MENUS, i);
		if (!pCmdMenuData) continue;
		if (PStrICmp(pCmdMenuData->szCmdMenu, szCmdMenu)) continue;

		const WCHAR * wszCmd = StringTableGetStringByGlobalKey(pCmdMenuData->szChatCommand);

		if (wszCmd)
		{
			WCHAR wszSlashCmd[CONSOLE_MAX_STR] = L"/";
			PStrCat(wszSlashCmd, wszCmd, arrsize(wszSlashCmd));
			UIListBoxAddString(pListBox, wszSlashCmd, (QWORD)pCmdMenuData);

			float fTextWidth = 0.0f;
			UIElementGetTextLogSize(pFont, nFontSize, 1.0f, pListBox->m_bNoScaleFont, wszSlashCmd, &fTextWidth, NULL);

			fWidth = MAX(fWidth, fTextWidth);
			fHeight += (i > 0 ? fLineHeight : fTextHeight);
		}
	}

	pListBox->m_fWidth = fWidth;
	pListBox->m_fHeight = fHeight;
	if (pListBox->m_pParent)
	{
		pListBox->m_pParent->m_fWidth = pListBox->m_fWidth + 10.0f;
		pListBox->m_pParent->m_fHeight = pListBox->m_fHeight + 10.0f;
		pListBox->m_pParent->m_fAnchorXoffs = pListBox->m_pParent->m_fWidth;
	}

	pListBox->m_nSelection = INVALID_LINK;
	UIComponentHandleUIMessage(pListBox, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatPanelOnPostCreate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	GAMEOPTIONS tOptions;
	structclear(tOptions);
	GameOptions_Get(tOptions);
	UIChatSetAlpha((BYTE)tOptions.nChatAlpha);

	UI_POSITION ChatSize;
	AppLoadChatSize(&ChatSize);

	if (ChatSize.m_fX || ChatSize.m_fY)
	{
		ChatSize.m_fX -= component->m_fWidth;
		ChatSize.m_fY -= component->m_fHeight;
	}

	UIResizeComponentByDelta(component, ChatSize);

	// populate the command menus
	//
	UI_COMPONENT * pListBox;

	if ((pListBox = UIComponentFindChildByName(component, "chat help panel listbox")) != NULL)
	{
		sUIChatFillCmdMenu(UICastToListBox(pListBox), AppIsHellgate() ? "hellgate_commands" : "mythos_commands");
	}

	if ((pListBox = UIComponentFindChildByName(component, "chat emote panel listbox")) != NULL)
	{
		sUIChatFillCmdMenu(UICastToListBox(pListBox), AppIsHellgate() ? "hellgate_emotes" : "mythos_emotes");
	}

	return UIComponentResize(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatTextResizeStart(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_MSG_RETVAL UIResizeComponentStart(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
	UI_MSG_RETVAL ret = UIResizeComponentStart(component, msg, wParam, lParam);

	sUIChatUpdateScrollBottomButtons();

	return ret;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatTextResizeEnd(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_MSG_RETVAL UIResizeComponentEnd(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
	UI_MSG_RETVAL ret = UIResizeComponentEnd(component, msg, wParam, lParam);

	sUIChatUpdateScrollBottomButtons();

	return ret;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatPanelOnResize(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_MSG_RETVAL ret = UIComponentResize(component, msg, wParam, lParam);

	UI_POSITION ChatSize;
	ChatSize.m_fX = component->m_fWidth;
	ChatSize.m_fY = component->m_fHeight;
	AppSaveChatSize(&ChatSize);

	UI_COMPONENT * pTextEntry = UIComponentGetByEnum(UICOMP_CHAT_TEXTENTRY);
	UI_COMPONENT * pButtonPanel = UIComponentFindChildByName(component, "chat help buttonpanel");
	if (pTextEntry && pTextEntry->m_pParent && pButtonPanel)
	{
		pTextEntry->m_fWidth = pTextEntry->m_pParent->m_fWidth - pButtonPanel->m_fWidth;
	}

	return ret;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIChatSetAlpha(
	const BYTE bAlpha)
{
	PList<UI_COMPONENT *>::USER_NODE *pNode = NULL;
	while ((pNode = g_UI.m_listAnimCategories[UI_ANIM_CATEGORY_CHAT_FADE].GetNext(pNode)) != NULL)
	{
		UI_COMPONENT *pChatPanel = (UI_COMPONENT*)pNode->Value;
		UIComponentSetAlpha(pChatPanel, bAlpha);
		UIComponentHandleUIMessage(pChatPanel, UIMSG_PAINT, 0, 0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIMapSetAlpha(
	const BYTE bAlpha)
{
	PList<UI_COMPONENT *>::USER_NODE *pNode = NULL;
	while ((pNode = g_UI.m_listAnimCategories[UI_ANIM_CATEGORY_MAP_FADE].GetNext(pNode)) != NULL)
	{
		UI_COMPONENT *pMapPanel = (UI_COMPONENT*)pNode->Value;
		UIComponentSetAlpha(pMapPanel, bAlpha);
		UIComponentHandleUIMessage(pMapPanel, UIMSG_PAINT, 0, 0);
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatTransparencySliderOnScroll(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIScrollBarOnScroll(component, msg, wParam, lParam);
	const BYTE bAlpha = (BYTE)MAP_VALUE_TO_RANGE(UIScrollBarGetValue(UICastToScrollBar(component), TRUE), 0.0f, 1.0f, CHAT_TRANSPARENCY_MIN, CHAT_TRANSPARENCY_MAX);
	UIChatSetAlpha(bAlpha);

	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMapTransparencySliderOnScroll(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIScrollBarOnScroll(component, msg, wParam, lParam);
	const BYTE bAlpha = (BYTE)MAP_VALUE_TO_RANGE(UIScrollBarGetValue(UICastToScrollBar(component), TRUE), 0.0f, 1.0f, CHAT_TRANSPARENCY_MIN, CHAT_TRANSPARENCY_MAX);
	UIMapSetAlpha(bAlpha);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIChatBubblesEnabled(
	void)
{
	GAMEOPTIONS tOptions;
	structclear(tOptions);
	GameOptions_Get(tOptions);
	return tOptions.bShowChatBubbles;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAddChatBubble( 
	const WCHAR *uszText, 
	DWORD dwColor, 
	UNIT *pSpeakerUnit )
{
	ASSERT_RETURN (pSpeakerUnit);
	ASSERT_RETURN (uszText);

	UI_CHAT_BUBBLE tBubble;

	tBubble.idUnit = UnitGetId(pSpeakerUnit);
	PStrCopy(tBubble.szText, uszText, arrsize(tBubble.szText));
	tBubble.dwColor = dwColor;
	tBubble.tiStart = AppCommonGetAbsTime();
	
	// if this unit already has a chat bubble, replace it with this one
	PList<UI_CHAT_BUBBLE>::USER_NODE *pNode = g_UI.m_listChatBubbles.GetNext(NULL);
	while (pNode != NULL)
	{
		if (pNode->Value.idUnit == tBubble.idUnit)
		{
			pNode = g_UI.m_listChatBubbles.RemoveCurrent(pNode);
			break;
		}
		else
		{
			pNode = g_UI.m_listChatBubbles.GetNext(pNode);
		}
	}

	g_UI.m_listChatBubbles.PListPushTail(tBubble);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIChatAbbreviateNames(
	void)
{
	GAMEOPTIONS tOptions;
	structclear(tOptions);
	GameOptions_Get(tOptions);
	return tOptions.bAbbreviateChatName;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct CHAT_BUBBLE_TEMP
{
	PList<UI_CHAT_BUBBLE>::USER_NODE *pNode;
	float	fLabelScale;
	float	fCameraDistSq;
	VECTOR	vScreenPos;
};


static int sCompareChatBubbles( const void * e1, const void * e2 )
{
	CHAT_BUBBLE_TEMP * p1 = (CHAT_BUBBLE_TEMP *)e1;
	CHAT_BUBBLE_TEMP * p2 = (CHAT_BUBBLE_TEMP *)e2;
	if ( p1->fCameraDistSq > p2->fCameraDistSq )
		return -1; // sort farther distance earlier
	if ( p1->fCameraDistSq < p2->fCameraDistSq )
		return 1; // sort closer distance later
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIProcessChatBubbles(
	GAME *pGame)
{
	if (!pGame)
		return;

	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_CHAT_BUBBLES_PANEL);
	if (!pComponent)
		return;

	UIComponentRemoveAllElements(pComponent);

	UNIT *pControlUnit = GameGetControlUnit( pGame );
	if(!pControlUnit)
	{
		return;
	}

	const CAMERA_INFO * pCameraInfo = c_CameraGetInfo( FALSE );
	if (!pCameraInfo)
	{
		return;
	}

	UI_RECT rectClip = UIComponentGetRectBase(pComponent);
	
	static const DWORD dwFadeoutStart = 4000;
	static const DWORD dwFadeoutEnd = 6000;
	TIME tiCurrent = AppCommonGetAbsTime();
	VECTOR vEye = CameraInfoGetPosition( pCameraInfo );
	VECTOR vLocation = AppIsHellgate() ? vEye : pControlUnit->vPosition;

	// we need an array of these, so they can be depth-sorted
	CHAT_BUBBLE_TEMP arChatBubbles[256];
	int nNumChatBubbles = 0;

	PList<UI_CHAT_BUBBLE>::USER_NODE *pNode = g_UI.m_listChatBubbles.GetNext(NULL);
	while (pNode != NULL)
	{
		UNIT *pUnit = UnitGetById(pGame, pNode->Value.idUnit);
		if (!pUnit ||												// the unit is no longer around
			tiCurrent - pNode->Value.tiStart > dwFadeoutEnd)		// the chat bubble is too old
		{
			pNode = g_UI.m_listChatBubbles.RemoveCurrent(pNode);
		}
		else
		{
			static const float fLabelHeight = 3.00f;  // in meters
			float	fCameraDistSq;
			float	fPlayerDistSq;
			float	fLabelScale;
			VECTOR	vScreenPos;

			UnitDisplayGetOverHeadPositions( pUnit, pControlUnit, vEye, fLabelHeight, vScreenPos, fLabelScale, fPlayerDistSq, fCameraDistSq);

			if (fPlayerDistSq <= NAMELABEL_VIEW_DISTANCE_SQ	&&	// if it's not too far away
				vScreenPos.fZ < 1.0f)							// and if it's not behind you
			{
				// Add it to the temp list
				arChatBubbles[nNumChatBubbles].fLabelScale = fLabelScale;
				arChatBubbles[nNumChatBubbles].vScreenPos = vScreenPos;
				arChatBubbles[nNumChatBubbles].fCameraDistSq = fCameraDistSq;
				arChatBubbles[nNumChatBubbles].pNode = pNode;
				nNumChatBubbles++;
			}

			pNode = g_UI.m_listChatBubbles.GetNext(pNode);
		}
	}

	if (nNumChatBubbles == 0)
		return;

	UIX_TEXTURE_FONT *pFont = UIComponentGetFont(pComponent);
	ASSERT_RETURN(pFont);
	int nFontSize = UIComponentGetFontSize(pComponent, pFont);
	float fMaxWidth = 600.0f;
	float fTextWidth = 0.0f;
	float fTextHeight = 0.0f;
	UIX_TEXTURE *pTexture = UIComponentGetTexture(pComponent);

	// sort the bubbles depth first
	qsort( arChatBubbles, nNumChatBubbles, sizeof(CHAT_BUBBLE_TEMP), sCompareChatBubbles );

	for (int i=0; i < nNumChatBubbles; i++)
	{
		if( AppIsHellgate() )
		{
			arChatBubbles[i].fLabelScale = PIN(arChatBubbles[i].fLabelScale * 3.0f, 0.2f, 1.5f);
		}

		float fWidthRatio = AppIsHellgate() ? arChatBubbles[i].fLabelScale : 1.0f;

		WCHAR *uszText = arChatBubbles[i].pNode->Value.szText;

		//// scale to the nearest integral
		//int nFontSize = (int)(((float)nBaseFontSize * fWidthRatio) - 0.5);

		UIElementGetTextLogSize(pFont,
			nFontSize,
			fWidthRatio,
			pComponent->m_bNoScaleFont,
			uszText,
			&fTextWidth,
			&fTextHeight);

		BOOL bUseExistingPointer = FALSE;

		if (fTextWidth > fMaxWidth * fWidthRatio)
		{
			int nLen = PStrLen(arChatBubbles[i].pNode->Value.szText) + 1;
			uszText = (WCHAR*)MALLOC(NULL, (nLen + 1) * sizeof(WCHAR));
			PStrCopy(uszText, arChatBubbles[i].pNode->Value.szText, nLen);
			bUseExistingPointer = TRUE;

			UI_SIZE size = DoWordWrapEx(uszText, NULL, fMaxWidth, pFont, pComponent->m_bNoScaleFont, nFontSize, 0, fWidthRatio, NULL, TRUE);

			fTextWidth = size.m_fWidth;
			fTextHeight = size.m_fHeight;
		}

		//float fZDelta = e_UIConvertAbsoluteZToDeltaZ( arChatBubbles[ i ].vScreenPos.fZ );
		UI_SIZE tSize (fTextWidth, fTextHeight);
		UI_POSITION posBase;
		if( AppIsHellgate() )
		{
			posBase.m_fX = (arChatBubbles[i].vScreenPos.fX + 1.0f) * 0.5f * UIDefaultWidth();
			posBase.m_fY = (-arChatBubbles[i].vScreenPos.fY + 1.0f) * 0.5f * UIDefaultHeight();
		}
		else
		{
			UNIT *pUnit = UnitGetById(pGame, arChatBubbles[i].pNode->Value.idUnit);

			int x, y;
			VECTOR vLoc;
			vLoc = UnitGetPosition( pUnit );
			vLoc.fZ += UnitGetCollisionHeight( pUnit ) * 1.8f;
			TransformWorldSpaceToScreenSpace( &vLoc, &x, &y );

			posBase.m_fX = (float)x ;
			posBase.m_fY = (float)y;

		}

		UI_POSITION pos(posBase.m_fX - tSize.m_fWidth / 2.0f, posBase.m_fY - tSize.m_fHeight);

		BYTE byAlpha = 255;
		DWORD dwElapsed = (DWORD)(tiCurrent - arChatBubbles[i].pNode->Value.tiStart);
		if (dwElapsed > dwFadeoutStart)		
		{
			float fPct = (float)(dwElapsed - dwFadeoutStart) / (dwFadeoutEnd - dwFadeoutStart);
			byAlpha = (BYTE)((1.0f - fPct) * 255.0f);
		}


		if (pComponent->m_pFrame)
		{
			float fBorderSize = (float)pComponent->m_dwParam  ;/** (tSize.m_fWidth / pComponent->m_pFrame->m_fWidth);*/

			UI_SIZE tFrameSize = tSize;
			tFrameSize.m_fWidth += fBorderSize * 2.0f;
			tFrameSize.m_fHeight += fBorderSize * 2.0f;

			pos.m_fY -= fBorderSize;
			UI_POSITION posFrame = pos;
			posFrame.m_fX -= fBorderSize;
			posFrame.m_fY -= fBorderSize;

			UIComponentAddElement(pComponent, 
				pTexture, 
				pComponent->m_pFrame, 
				posFrame, 
				UI_MAKE_COLOR(byAlpha, GFXCOLOR_WHITE),
				&rectClip,
				FALSE,
				1.0f,
				1.0f,
				&tFrameSize);

			UI_PANELEX *pPanel = UICastToPanel(pComponent);
			if (pPanel->m_pHighlightFrame)
			{
				float fTailScale = (float)pComponent->m_dwParam2 / 100.0f;

				posFrame.m_fX += (tFrameSize.m_fWidth / 2.0f) - (pPanel->m_pHighlightFrame->m_fWidth * fTailScale / 2.0f);
				posFrame.m_fY += tFrameSize.m_fHeight;

				UIComponentAddElement(pComponent, 
					pTexture, 
					pPanel->m_pHighlightFrame, 
					posFrame, 
					UI_MAKE_COLOR(byAlpha, GFXCOLOR_WHITE),
					&rectClip,
					FALSE,
					fTailScale,
					fTailScale);
			}

		}

		UI_ELEMENT_TEXT *pElement = NULL;
		if (bUseExistingPointer)
		{
			pElement = UIComponentAddTextElementNoCopy(
				pComponent,
				NULL,
				pFont,
				nFontSize,
				uszText,
				pos,
				UI_MAKE_COLOR(byAlpha, arChatBubbles[i].pNode->Value.dwColor),
				&rectClip,
				UIALIGN_TOP,
				&tSize,
				FALSE,
				0,
				fWidthRatio);
		}
		else
		{
			pElement = UIComponentAddTextElement(
				pComponent,
				NULL,
				pFont,
				nFontSize,
				uszText,
				pos,
				UI_MAKE_COLOR(byAlpha, arChatBubbles[i].pNode->Value.dwColor),
				&rectClip,
				UIALIGN_TOP,
				&tSize,
				FALSE,
				0,
				fWidthRatio);
		}

		ASSERT_RETURN(pElement);

		//if ((pElement->m_dwColor & 0xffffff) == 0xffffff)	// color is white
		//{
		//	DWORD dwTemp = pElement->m_dwColor;
		//	pElement->m_dwColor = UI_MAKE_COLOR(byAlpha, pElement->m_dwDropshadowColor);
		//	pElement->m_dwDropshadowColor = UI_MAKE_COLOR(byAlpha, dwTemp);
		//}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define CHAT_MENU_TOOLTIP_DELAY 800

static DWORD s_dwTooltipEvent = INVALID_ID;
static BOOL s_bChatTooltipUp = FALSE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIChatHelpPanelShowTooltip(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT * component = (UI_COMPONENT *)data.m_Data1;
	ASSERT_RETURN(component);

	if (!UIComponentGetVisible(component) || !UIComponentGetActive(component))
	{
		return;
	}

	UI_LISTBOX * pListBox = UICastToListBox(component);

	UI_MSG_RETVAL UIListBoxOnMouseDown(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);
	UIListBoxOnMouseDown(component, UIMSG_LBUTTONDOWN, 0, 0);

	CMDMENU_DATA * pCmdMenuData = (CMDMENU_DATA*)UIListBoxGetSelectedData(pListBox);
	if (!pCmdMenuData)
	{
		return;
	}

	pListBox->m_nSelection = INVALID_LINK;
	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);

	WCHAR wszTooltip[CONSOLE_MAX_STR] = L"";
	WCHAR wszCmdDesc[CONSOLE_MAX_STR] = L"";

	const WCHAR * wszCmd = StringTableGetStringByGlobalKey(pCmdMenuData->szChatCommand);

	c_ConsoleGetDescString(wszCmdDesc, arrsize(wszCmdDesc), wszCmd, FONTCOLOR_WHITE);

	if (wszCmdDesc)
	{
		PStrPrintf(wszTooltip, arrsize(wszTooltip), L"%s\n%s: %s ", wszCmdDesc, GlobalStringGet(GS_CCMD_HELP_HEADER_USAGE), wszCmd);
	}
	else
	{
		PStrPrintf(wszTooltip, arrsize(wszTooltip), L"%s: %s ", GlobalStringGet(GS_CCMD_HELP_HEADER_USAGE), wszCmd);
	}

	c_ConsoleGetUsageString(wszTooltip, arrsize(wszTooltip), wszCmd, FONTCOLOR_WHITE);

	// display tooltip
	UISetSimpleHoverText(component, wszTooltip);	

	s_bChatTooltipUp = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatHelpPanelOnMouseMove(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetVisible(component) || !UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (s_dwTooltipEvent != INVALID_ID)
	{
		CSchedulerCancelEvent(s_dwTooltipEvent);
	}

	if (s_bChatTooltipUp)
	{
		UIComponentActivate(UICOMP_SIMPLETOOLTIP, FALSE);
		s_bChatTooltipUp = FALSE;
	}

	if (!UIComponentCheckBounds(component))
	{
		UI_LISTBOX * pListBox = UICastToListBox(component);
		if (pListBox->m_nHoverItem != INVALID_LINK)
		{
			pListBox->m_nHoverItem = INVALID_LINK;
			UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatHelpPanelOnMouseHoverLong(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	//UITooltipRestartTown

	if (UIComponentCheckBounds(component))
	{
		s_dwTooltipEvent = CSchedulerRegisterEvent(AppCommonGetCurTime() + CHAT_MENU_TOOLTIP_DELAY, sUIChatHelpPanelShowTooltip, CEVENT_DATA((DWORD_PTR)component));
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitComponentTypesChat(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize)
{	

	UI_XML_ONFUNC gUIXmlFunctions[] =
	{	// function name						function pointer
		{ "UIChatMinimizeOnClick",				UIChatMinimizeOnClick },
		{ "UIChatOnInactivate",					UIChatOnInactivate },
		{ "UIChatOnActivate",					UIChatOnActivate },
		{ "UISetChatActive",					UISetChatActive },
		{ "UITextEntryBackgroundOnClick",		UITextEntryBackgroundOnClick },
		{ "UIChatTabOnClick",					UIChatTabOnClick },
		{ "UIChatFadeIn",						UIChatFadeIn },
		{ "UIChatFadeOut",						UIChatFadeOut },
		{ "UIChatPanelOnPostCreate",			UIChatPanelOnPostCreate },
		{ "UIChatPanelOnResize",				UIChatPanelOnResize },
		{ "UIChatTransparencySliderOnScroll",	UIChatTransparencySliderOnScroll },
		{ "UIChatTextResizeStart",				UIChatTextResizeStart },
		{ "UIChatTextResizeEnd",				UIChatTextResizeEnd },
		{ "UIChatHelpButtonOnClick",			UIChatHelpButtonOnClick },
		{ "UIChatHelpPanelOnClick",				UIChatHelpPanelOnClick },
		{ "UIChatHelpPanelOnMouseMove",			UIChatHelpPanelOnMouseMove },
		{ "UIChatHelpPanelOnMouseHoverLong",	UIChatHelpPanelOnMouseHoverLong },
		{ "UIHypertextOnLMouseDown",			UIHypertextOnLMouseDown },
		{ "UIHypertextOnMouseHover",			UIHypertextOnMouseHover },
		{ "UIHypertextOnMouseMove",				UIHypertextOnMouseMove },
		{ "UIMapTransparencySliderOnScroll",	UIMapTransparencySliderOnScroll },
	};

	// Add on the message handler functions for the local components
	int nOldSize = nXmlFunctionsSize;
	nXmlFunctionsSize += sizeof(gUIXmlFunctions);
	pUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, pUIXmlFunctions, nXmlFunctionsSize);
	memcpy((BYTE *)pUIXmlFunctions + nOldSize, gUIXmlFunctions, sizeof(gUIXmlFunctions));
}

#endif
