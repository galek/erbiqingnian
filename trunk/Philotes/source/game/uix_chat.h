//----------------------------------------------------------------------------
// uix_chat.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_CHAT_H_
#define _UIX_CHAT_H_

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum
{
	CHAT_TAB_GLOBAL,
	CHAT_TAB_LOCAL,
	CHAT_TAB_PARTY,
	CHAT_TAB_GUILD,
	CHAT_TAB_CSR,
};

//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------
enum CHAT_TYPE;
struct CHAT_TAB;
struct UI_XML_COMPONENT;
struct UI_XML_ONFUNC;
class UI_LINE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAddChatLineArgs(
	CHAT_TYPE eChatType,
	DWORD dwColor,
	const WCHAR * szFormat,
	...);

void UIAddChatLine(
	CHAT_TYPE eChatType,
	DWORD dwColor,
	const WCHAR * szBuf,
	UI_LINE * pLine = NULL );

void UIAddServerChatLine(
	const WCHAR * format);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void			UIChatInit();
void			UIChatSessionStart();
void			UIChatFree();

UI_MSG_RETVAL	UIChatOpen(BOOL bUserActive = TRUE);
UI_MSG_RETVAL	UIChatClose(BOOL bUserActive = TRUE);
BOOL			UIChatIsOpen();
void			UIChatClear();

void			UIChatSetAutoFadeState(const BOOL bAutoFade);
BOOL			UIChatAutoFadeEnabled();
void			UIChatFadeIn(const BOOL bImmediate);
void			UIChatFadeOut(const BOOL bImmediate, const BOOL bForce = FALSE);

BOOL			UIChatAddTab(LPCSTR szLabel, LPCSTR szComponentName, CHAT_TYPE eChatContext, int nChatTypes);
BOOL			UIChatRemoveTab(int nTab);
BOOL			UIChatRemoveTab(CHAT_TAB * pTab);
void			UIChatRemoveAllTabs();
BOOL			UIChatTabIsVisible(const int nIndex);
void			UIChatTabActivate(const int nIndex);
int				UIGetChatTabIdx(void);
void			UINextChatTab(void);
void			UIPrevChatTab(void);
void			UIChatSetContextBasedOnTab(void);

void			UICSRPanelOpen();
void			UICSRPanelClose();
BOOL			UICSRPanelIsOpen();

void			UIChatSetAlpha(const BYTE bAlpha);
void			UIMapSetAlpha(const BYTE bAlpha);

BOOL UIChatAbbreviateNames(
	void);

BOOL UIChatBubblesEnabled(
	void);

void UIAddChatBubble( 
	const WCHAR *uszText, 
	DWORD dwColor, 
	UNIT *pSpeakerUnit );

void UIProcessChatBubbles( 
	GAME * pGame);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitComponentTypesChat(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize);

#endif
