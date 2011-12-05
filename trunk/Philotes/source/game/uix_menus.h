//----------------------------------------------------------------------------
// uix_menus.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_MENUS_H_
#define _UIX_MENUS_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "uix_priv.h"
#include "npc.h"
#ifndef _UIX_QUESTS_H_
#include "uix_quests.h"
#endif

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	UITYPE_PLAYER_REQUEST_DIALOG = NUM_UITYPES_QUEST,
	NUM_UITYPES_MENUS,
};

//----------------------------------------------------------------------------
// Structures
//----------------------------------------------------------------------------
typedef void (* PFN_PLAYER_REQUEST_CALLBACK)( PGUID guidOtherPlayer, void *pCallbackData );

struct UI_PLAYER_REQUEST_DATA
{
	PGUID guidOtherPlayer;
	WCHAR szMessage[2048];
	void *pData;
	PFN_PLAYER_REQUEST_CALLBACK pfnOnAccept;
	PFN_PLAYER_REQUEST_CALLBACK pfnOnDecline;
	PFN_PLAYER_REQUEST_CALLBACK pfnOnIgnore;

	UI_PLAYER_REQUEST_DATA::UI_PLAYER_REQUEST_DATA() : guidOtherPlayer(INVALID_GUID), pData(NULL), pfnOnAccept(NULL), pfnOnDecline(NULL), pfnOnIgnore(NULL)	{}
};

struct UI_PLAYER_REQUEST_DIALOG : UI_COMPONENT
{
	PList<struct UI_PLAYER_REQUEST_DATA>	m_listPlayerRequests;
	DWORD									m_dwRetryEventTicket;
};

//----------------------------------------------------------------------------
// CASTING FUNCTIONS
//----------------------------------------------------------------------------

CAST_FUNC(UITYPE_PLAYER_REQUEST_DIALOG, UI_PLAYER_REQUEST_DIALOG, PlayerRequestDialog);

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------
void InitComponentTypesMenus(struct UI_XML_COMPONENT *pComponentTypes, struct UI_XML_ONFUNC*& pUIXmlFunctions, int& nXmlFunctionsSize);
BOOL UICharacterSelectionScreenIsReadyToDraw();
void UICharacterSelectionScreenUpdate();

void UI_MultiplayerCharCountHandler(
	short nCount);

BOOL UI_MultiplayerCharListHandler(
	WCHAR *szCharName,
	int nPlayerSlot,
    BYTE *data,
    DWORD dataLen,
	WCHAR *szCharGuildName);

BOOL UICharacterSelectLoadExistingCharacter();

void UIMultiplayerStartCharacterSelect();

void UIShowStackSplitDialog(
	UNIT * pUnit);

void UIShowPlayerRequestDialog(
	PGUID guidOtherPlayer,
	const WCHAR *szMessage,
	PFN_PLAYER_REQUEST_CALLBACK pfnOnAccept,
	PFN_PLAYER_REQUEST_CALLBACK pfnOnDecline,
	PFN_PLAYER_REQUEST_CALLBACK pfnOnIgnore);

void UICancelPlayerRequestDialog(
	PGUID guidOtherPlayer,
	PFN_PLAYER_REQUEST_CALLBACK pfnOnAccept,
	PFN_PLAYER_REQUEST_CALLBACK pfnOnDecline,
	PFN_PLAYER_REQUEST_CALLBACK pfnOnIgnore);

void UIShowDuelInviteDialog(
	UNIT *pInviter);

void UIShowExitInstanceForPartyJoinDialog(
	PGUID guidInviter);

void UIShowExitInstanceForPartyJoinDialog();

void UIShowPartyInviteDialog(
	PGUID guidInviter,
	const WCHAR *szInviter);

UI_COMPONENT * UIGetCurrentMenu(void);

void UIDoConversationComplete( 
	UI_COMPONENT *pComponent,
	CONVERSATION_COMPLETE_TYPE eType, 
	BOOL bSendTalkDone = TRUE,
	BOOL bCloseConversationUI = TRUE);
	
void UIMenuSetSelect(
	int nSelected);

UI_MSG_RETVAL UIConversationOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

UI_MSG_RETVAL UIConversationOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

UI_MSG_RETVAL UIConversationOnCancel(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

void UIShowBugReportDlg(
	char * szShortText,
	const OS_PATH_CHAR * szScreenshot);

void UIUpdateAccountStatus();
void UIUpdateAccountStatus(UI_LABELEX * pLabel);

void UIExitApplication(void);	// CHB 2007.05.10

void UIShowGuildInviteDialog(
	PGUID guidInviter,
	const WCHAR *szInviter, 
	const WCHAR *szGuildName);

void UIShowBuddyInviteDialog(
	UNIT *pInviter);

void UIShowBuddyInviteDialog(
	const WCHAR *szInviter);

void UIDelayedShowBuddyInviteDialog(void);

void UIHideCurrentMenu(void);

void UIRestoreLastMenu(void);

void CharCreateResetUnitLists(void);

int SinglePlayerGetNumSaves();

// -- Load functions ----------------------------------------------------------
BOOL UIXmlLoadPlayerRequestDialog		(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);

// -- Free functions ----------------------------------------------------------
void UIComponentFreePlayerRequestDialog	(UI_COMPONENT* component);

#endif