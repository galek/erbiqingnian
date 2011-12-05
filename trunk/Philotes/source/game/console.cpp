//----------------------------------------------------------------------------
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "chat.h"
#include "console.h"
#include "console_priv.h"
#include "consolecmd.h"
#include "c_message.h"
#include "globalindex.h"
#include "s_gdi.h"
#include "uix.h"
#include "uix_priv.h"
#include "uix_components.h"
#include "uix_chat.h"
#include "units.h"
#include "e_settings.h"
#include "performance.h"
#include "chat.h"
#include "version.h"

#if !ISVERSION(SERVER_VERSION)
#include "svrstd.h"
#include "player.h"
#include "c_chatNetwork.h"
#include "UserChatCommunication.h"
#include "c_imm.h"
#include "stringreplacement.h"
#include "c_channelmap.h"
#else
#include "svrstd.h"
#include "UserChatCommunication.h"
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
//#define MARK_BUILD_STRING() \
//	static WCHAR szMarkBuildString[] = L"<name goes here>"; \
//	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Recipient: %s", szMarkBuildString );
#define MARK_BUILD_STRING()

#define CONSOLE_PROMPT					L">"
#define CONSOLE_PROMPT_MAX				4

#define CONSOLE_LINEBUFFER				100
#define CONSOLE_LINES					35
#define CONSOLE_LINELEN					62

#define CONSOLE_HISTORY					32

#define CONSOLE_BOTTOM_MARGIN			80
#define CONSOLE_PIXEL_WIDTH				500

#define MAX_TICKS_ON_SCREEN				20000
#define TICK_START_FADE					15000
#define MAX_TICKS_BETWEEN_DRAWS			10000


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct CONSOLE_LINE 
{
	WCHAR			m_pszString[CONSOLE_MAX_STR];
	DWORD			m_dwColor;
	TIME			m_tiTimeAdded;
};

struct CONSOLE
{
	BOOL			m_bActive;

	CONSOLE_LINE*	m_History;
	int				m_nHistoryCount;
	int				m_nHistoryFirst;
	int				m_nHistoryBuffer;
	int				m_nHistoryScroll;

	int				m_nComponentId;

	DWORD			m_dwDefaultColor;
	DWORD			m_dwInputColor;
	DWORD			m_dwErrorColor;

	CHAT_TYPE		m_eChatTypeContext;
	WCHAR			m_wszNextConversationPlayerName[MAX_CHARACTER_NAME];
	WCHAR			m_wszLastConversationPlayerName[MAX_CHARACTER_NAME];
	WCHAR			m_wszCSRPlayerName[MAX_CHARACTER_NAME];
	WCHAR			m_wszLastInstancingChannelName[MAX_INSTANCING_CHANNEL_NAME];
	WCHAR			m_wszChatPrefixCommand[CONSOLE_MAX_STR];
	BOOL			m_bKeepChatContext;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline UI_TEXTBOX * sGetConsole(
	void)
{
	// tug wants to keep using the chat as the console for now.
	UI_COMPONENT *pComp = AppIsHellgate() ?UIComponentGetByEnum(UICOMP_CONSOLE) : UIComponentGetByEnum(UICOMP_MAIN_CHAT);;
	//ASSERT(pComp);
	if ( ! pComp )
		return NULL;
	return UICastToTextBox(pComp);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline UI_EDITCTRL * sGetConsoleEdit(
	void)
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_CONSOLE_EDIT);
	//ASSERT(pComp);
	if ( ! pComp )
		return NULL;
	return UICastToEditCtrl(pComp);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConsoleIsEditActive(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	UI_EDITCTRL *pEdit = sGetConsoleEdit();
	return UIComponentGetActive(pEdit);
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleActivate(
	BOOL bActivate)
{
	// TRAVIS : Tugboat console goes to chat.
	if( AppIsTugboat() )
	{
		return;
	}
#if !ISVERSION(SERVER_VERSION)
	UI_COMPONENT *pConsolePanel = UIComponentGetByEnum(UICOMP_CONSOLE_PANEL);
	if (!pConsolePanel)
		return;

	UIComponentHandleUIMessage(pConsolePanel, bActivate ? UIMSG_ACTIVATE : UIMSG_INACTIVATE, 0, 0);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleToggle(
	void)
{
	// TRAVIS : Tugboat console goes to chat.
	if( AppIsTugboat() )
	{
		return;
	}
#if !ISVERSION(SERVER_VERSION)
	UI_COMPONENT *pConsolePanel = UIComponentGetByEnum(UICOMP_CONSOLE_PANEL);
	if (!pConsolePanel)
		return;

	BOOL bActivate = !UIComponentGetActive(pConsolePanel);
	UIComponentHandleUIMessage(pConsolePanel, bActivate ? UIMSG_ACTIVATE : UIMSG_INACTIVATE, 0, 0);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConsoleIsActive(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	UI_TEXTBOX *pConsole = sGetConsole();
	if (!pConsole)
		return FALSE;

	return UIComponentGetActive(pConsole);
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ConsoleErrorColor(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	CONSOLE * console = gApp.m_pConsole;
	ASSERT_RETVAL(console, GFXCOLOR_RED);

	return console->m_dwErrorColor;
#else
	return 0;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ConsoleInputColor(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	CONSOLE * console = gApp.m_pConsole;
	ASSERT_RETVAL(console, GFXCOLOR_WHITE);

	return console->m_dwInputColor;
#else
	return 0;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ConsoleDefaultColor(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	CONSOLE * console = gApp.m_pConsole;
	ASSERT_RETVAL(console, GFXCOLOR_RED);

	return console->m_dwDefaultColor;
#else
	return GFXCOLOR_RED;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * ConsoleGetInputLine(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	UI_EDITCTRL *pEdit = sGetConsoleEdit();
	if (!UIComponentGetActive(pEdit))
	{
		return NULL;
	}
	else
	{
		return UILabelGetText(pEdit);
	}
#else
	return NULL;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleSetInputLine(
	const WCHAR * pszNewLine,
	DWORD /*dwColor*/,
	int nOffset /*= 0*/)		
{
#if !ISVERSION(SERVER_VERSION)
	UI_EDITCTRL *pEdit = sGetConsoleEdit();
	if (UIComponentGetActive(pEdit))
	{
		if (nOffset > 0)
		{
			WCHAR szBuf[1024];
			PStrCopy(szBuf, UILabelGetText(pEdit), nOffset+1);
			PStrCat(szBuf, pszNewLine, 1024);
			UILabelSetText(pEdit, szBuf);
		}
		else
		{
			UILabelSetText(pEdit, pszNewLine);
		}

		UIComponentHandleUIMessage(pEdit, UIMSG_PAINT, 0, 0);
	}

	//CONSOLE * console = gApp.m_pConsole;
	//ASSERT_RETURN(console);

	//CONSOLE_LINE * line = &(console->m_InputLine);
	//offset += console->m_nPromptSize;
	//line->m_pszString[offset] = 0;
	//line->m_dwColor = dwColor;
	//console->m_nCursorX = offset;

	//if (pszNewLine)
	//{
	//	PStrCopy(line->m_pszString + offset, pszNewLine, CONSOLE_MAX_STR - offset);
	//	console->m_nCursorX = PStrLen(pszNewLine, CONSOLE_MAX_STR - offset) + offset;
	//}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleSetEditActive(
	BOOL bActive)
{
#if !ISVERSION(SERVER_VERSION)
	UI_EDITCTRL *pEdit = sGetConsoleEdit();
	if(pEdit)
	{
		if (bActive)
		{
			if (AppIsHellgate())
			{
				if (!UIChatIsOpen() || !UIComponentGetActiveByEnum(UICOMP_CHAT_PANEL))
				{
					UIComponentActivate(UICOMP_CHAT_PANEL, TRUE, 0, NULL, FALSE, FALSE, TRUE, TRUE);
					UIComponentSetVisibleByEnum(UICOMP_CHAT_TEXT, FALSE);
					UIComponentSetVisibleByEnum(UICOMP_CHAT_TABS, FALSE);
				}

				UIComponentActivate(UICOMP_CHAT_TEXTENTRY_ACTIVE, TRUE, 0, NULL, FALSE, FALSE, TRUE, TRUE);
				UIComponentSetVisibleByEnum(UICOMP_CHAT_TEXTENTRY_ACTIVE, TRUE);
			}
			UIComponentActivate(pEdit, TRUE);
			UIEditCtrlSetFocus(pEdit);
		}
		else
		{
			UIComponentActivate(pEdit, FALSE);
			if( AppIsTugboat() )
			{
				UI_COMPONENT *pEditLabel = UIComponentGetByEnum(UICOMP_CONSOLE_EDIT_LABEL);
				UIComponentSetVisible(pEditLabel, FALSE);
			}

			if (AppIsHellgate())
			{
				if (!UIComponentGetVisibleByEnum(UICOMP_CHAT_TEXT))
				{
					UIComponentActivate(UICOMP_CHAT_PANEL, FALSE, 0, NULL, FALSE, FALSE, TRUE, TRUE);
				}

				UIComponentActivate(UICOMP_CHAT_TEXTENTRY_ACTIVE, FALSE, 0, NULL, FALSE, FALSE, TRUE, TRUE);
				UIComponentSetVisibleByEnum(UICOMP_CHAT_TEXTENTRY_ACTIVE, FALSE);
			}
			//UIComponentSetVisible(pEdit, FALSE);
		}

		IMM_Enable(bActive);
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleVersionString(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	WCHAR szDate[CONSOLE_MAX_STR];
	PStrCvt(szDate, __TIMESTAMP__, CONSOLE_MAX_STR);

	WCHAR szBranch[ 12 ] = L"ML";
	if ( PRODUCTION_BRANCH_VERSION )
		PStrCopy( szBranch, L"Production", 12 );
	else if ( RC_BRANCH_VERSION )
		PStrCopy( szBranch, L"RC", 12 );

	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Version Date: %s\nVersion Number: %d.%d.%d.%d\n%s Build", szDate, 
        TITLE_VERSION,PRODUCTION_BRANCH_VERSION,RC_BRANCH_VERSION,MAIN_LINE_VERSION, szBranch);

	if ( AppIsBeta() )
		ConsoleString( "Running as Beta" );
	if ( AppIsDemo() )
		ConsoleString( "Running as Demo" );
	if ( AppCommonSingleplayerOnly() )
		ConsoleString( "Running as Single-Player Only" );
	else if ( AppIsSinglePlayer() )
		ConsoleString( "Running as Single-Player" );
	if ( AppIsMultiplayer() )
		ConsoleString( "Running as Multiplayer" );

	MARK_BUILD_STRING();

	LOCALIZED_CMD_OUTPUT(L"%s", ISVERSION(OPTIMIZED_VERSION) ? L"Optimizations enabled" : L"Optimizations disabled");

	if (AppGetCltGame())
	{
		LOCALIZED_CMD_OUTPUT( L"Cheats %s", GameGetDebugFlag(AppGetCltGame(), DEBUGFLAG_ALLOW_CHEATS) ? L"Enabled" : L"Disabled");
	}
	else
	{
		const GLOBAL_DEFINITION* global_definition = DefinitionGetGlobal();
		ASSERT_RETURN(global_definition);
		LOCALIZED_CMD_OUTPUT(L"Cheats %s", (global_definition->dwGameFlags & GLOBAL_GAMEFLAG_ALLOW_CHEATS) ? L"Enabled" : L"Disabled");
	}

	LOCALIZED_CMD_OUTPUT(L"Game Id: %i", (int)AppGetSrvGameId());
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConsoleInitBuffer(
	void)
{
	ConsoleFree();

#if !ISVERSION(SERVER_VERSION)
	CONSOLE * console = (CONSOLE *)MALLOCZ(NULL, sizeof(CONSOLE));
	gApp.m_pConsole = console;

	console->m_nHistoryBuffer = CONSOLE_HISTORY;
	
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	console->m_History = (CONSOLE_LINE *)MALLOCZ(g_StaticAllocator, sizeof(CONSOLE_LINE) * console->m_nHistoryBuffer);
#else
	console->m_History = (CONSOLE_LINE *)MALLOCZ(NULL, sizeof(CONSOLE_LINE) * console->m_nHistoryBuffer);
#endif		
	
	console->m_nHistoryFirst = -1;
	console->m_nHistoryScroll = -1;

	console->m_dwDefaultColor = GFXCOLOR_WHITE;
	console->m_dwInputColor = GFXCOLOR_WHITE;
	console->m_dwErrorColor = GFXCOLOR_RED;

	console->m_eChatTypeContext = CHAT_TYPE_GAME;
	console->m_wszLastConversationPlayerName[0] = 0;
	console->m_wszCSRPlayerName[0] = 0;
	console->m_wszLastInstancingChannelName[0] = 0;
	ConsoleSetLastInstancingChannelName(L"chat");
	console->m_bKeepChatContext = FALSE;
#endif

	ConsoleCommandTableInit();

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConsoleInitInterface(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	TIMER_STARTEX("PrimeInit() - ConsoleInitInterface()", 100);

	ASSERT_RETFALSE(gApp.m_pConsole);
	gApp.m_pConsole->m_nComponentId = UIComponentGetIdByName("console");
#endif
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleFree(
	void)
{
	ConsoleCommandTableFree();

#if !ISVERSION(SERVER_VERSION)
	CONSOLE * console = gApp.m_pConsole;
	if (!console)
	{
		return;
	}

	if (console->m_History)
	{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		FREE(g_StaticAllocator, console->m_History);
#else
		FREE(NULL, console->m_History);
#endif		
	
	}
	FREE(NULL, console);
	gApp.m_pConsole = NULL;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleClear(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	UI_TEXTBOX *pConsole = sGetConsole();

	if ( pConsole )
	{
		UITextBoxClear(pConsole);
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sConsoleAddHistory(
	CONSOLE * console, 
	const WCHAR * str)
{
	ASSERT_RETURN(console);

	if (!str || str[0] == 0)
	{
		return;
	}

	console->m_nHistoryFirst++;
	if (console->m_nHistoryFirst >= console->m_nHistoryBuffer)
	{
		console->m_nHistoryFirst = 0;
	}
	if (console->m_nHistoryCount < console->m_nHistoryBuffer)
	{
		console->m_nHistoryCount++;
	}

	CONSOLE_LINE* line = console->m_History + console->m_nHistoryFirst;
	PStrCopy(line->m_pszString, str, CONSOLE_MAX_STR);

	console->m_nHistoryScroll = -1;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sConsoleHistoryScrollUp(
	CONSOLE * console)
{
	ASSERT_RETURN(console);

	if (console->m_nHistoryCount <= 0)
	{
		ConsoleSetInputLine(NULL, console->m_dwInputColor);
		return;
	}

	if (console->m_nHistoryScroll < 0)
	{
		console->m_nHistoryScroll = console->m_nHistoryFirst;
	}
	else
	{
		console->m_nHistoryScroll--;
		if (console->m_nHistoryScroll < 0)
		{
			if (console->m_nHistoryCount < console->m_nHistoryBuffer)
			{
				console->m_nHistoryScroll = -1;
				ConsoleSetInputLine(NULL, console->m_dwInputColor);
				return;
			}
			console->m_nHistoryScroll = console->m_nHistoryBuffer - 1;
		}
	}
	ConsoleSetInputLine(console->m_History[console->m_nHistoryScroll].m_pszString, console->m_dwInputColor); 
	console->m_bKeepChatContext = TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sConsoleHistoryScrollDown(
	CONSOLE * console)
{
	ASSERT_RETURN(console);

	if (console->m_nHistoryCount <= 0)
	{
		ConsoleSetInputLine(NULL, console->m_dwInputColor);
		return;
	}

	console->m_nHistoryScroll++;
	if (console->m_nHistoryScroll > console->m_nHistoryBuffer)
	{
		console->m_nHistoryScroll = 0;
	}
	if (console->m_nHistoryScroll > console->m_nHistoryFirst)
	{
		console->m_nHistoryScroll = -1;
		ConsoleSetInputLine(NULL, console->m_dwInputColor);
		return;
	}
	ConsoleSetInputLine(console->m_History[console->m_nHistoryScroll].m_pszString, console->m_dwInputColor); 
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sConsoleHistoryReset(
	CONSOLE * console)
{
	ASSERT_RETURN(console);
	console->m_nHistoryScroll = 0;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sConsoleAddInputChar(
	WCHAR wc,
	DWORD color)
{
	UI_EDITCTRL *pEdit = sGetConsoleEdit();
	UIEditCtrlOnKeyChar(pEdit, UIMSG_KEYCHAR, (DWORD)wc, 0);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleAddInput(
	const WCHAR wc)
{
#if !ISVERSION(SERVER_VERSION)
	CONSOLE * console = gApp.m_pConsole;
	ASSERT_RETURN(console);

	if (PStrIsPrintable(wc) == FALSE)
	{
		return;
	}
	sConsoleAddInputChar(wc, console->m_dwInputColor);
	console->m_bKeepChatContext = TRUE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleSetInputColor(
	DWORD color )
{
#if !ISVERSION(SERVER_VERSION)
	gApp.m_pConsole->m_dwInputColor = color;

	UI_COMPONENT *pChatContextLabel = UIComponentGetByEnum(UICOMP_CONSOLE_EDIT_LABEL);
	if (pChatContextLabel)
	{
		pChatContextLabel->m_dwColor = color;
	}

	UI_COMPONENT *pChatGlow = UIComponentGetByEnum(UICOMP_CHAT_TEXTENTRY_ACTIVE);
	if (pChatGlow)
	{
		pChatGlow->m_dwColor = color;
		UIComponentHandleUIMessage(pChatGlow, UIMSG_PAINT, 0, 0);
//		pChatGlow->m_bNeedsRepaintOnVisible = TRUE;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleAddInput(
	const WCHAR * str)
{
	if(!str)
		return;

	while(str[0])
	{
		ConsoleAddInput(str[0]);
		++str;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sConsoleHandleDefault(
	const WCHAR * str,
	UI_LINE * pLine)
{
	str = PStrSkipWhitespace(str);
	if (!str[0])
		return;

	if(c_PlayerGetPartyId() != INVALID_CHANNELID)
	{
		//	send to their party
		c_SendPartyChannelChat(str, pLine);
	}
	else if(gApp.m_idLevelChatChannel != INVALID_CHANNELID)
	{
		//	send to local town chat
		c_SendTownChannelChat(str, pLine);
	}
}
#endif

//----------------------------------------------------------------------------
struct CHAT_CONTEXT_COMMAND_LOOKUP
{
	CHAT_TYPE eChatType;
	GLOBAL_STRING eGlobalString;
};
static CHAT_CONTEXT_COMMAND_LOOKUP sgtChatContextLookup[] = 
{
	{ CHAT_TYPE_REPLY,		GS_CCMD_CHAT_REPLY },
	{ CHAT_TYPE_WHISPER,	GS_CCMD_CHAT_WHISPER },
	{ CHAT_TYPE_PARTY,		GS_CCMD_CHAT_PARTY },
	{ CHAT_TYPE_GUILD,		GS_CCMD_GUILD_CHAT },
	{ CHAT_TYPE_GUILD,		GS_CCMD_GUILD_GCHAT },
	{ CHAT_TYPE_GAME,		GS_CCMD_CHAT_LOCAL },
	{ CHAT_TYPE_SHOUT,		GS_CCMD_CHAT_SHOUT },
	{ CHAT_TYPE_CSR,		GS_CCMD_CSR_CHAT },
	{ CHAT_TYPE_CSR,		GS_CCMD_CSR_CSRCHAT },
};
static const int sgnNumChatContextLookup = arrsize( sgtChatContextLookup );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sConsoleSetChatContext(
	void )
{
	CONSOLE * pConsole = gApp.m_pConsole;

#if !ISVERSION(DEVELOPMENT)
	if(AppIsHellgate() && AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{
		ConsoleSetInputColor(GFXCOLOR_WHITE);
		return;
	}
#endif

	// find the context lookup
	const WCHAR *puszCommand = NULL;
	for (int i = 0; i < sgnNumChatContextLookup; ++i)
	{
		const CHAT_CONTEXT_COMMAND_LOOKUP *pLookup = &sgtChatContextLookup[ i ];
		if (pLookup->eChatType == ConsoleGetChatContext())
		{
			puszCommand = GlobalStringGet( pLookup->eGlobalString );
		}
	}

	WCHAR wszContextLabel[CONSOLE_MAX_STR] = {0};
	pConsole->m_wszChatPrefixCommand[0] = 0L;

	if (puszCommand)
	{
		switch(ConsoleGetChatContext())
		{
			case CHAT_TYPE_WHISPER:
				PStrPrintf(pConsole->m_wszChatPrefixCommand, arrsize(pConsole->m_wszChatPrefixCommand), L"/%s %s", puszCommand, pConsole->m_wszNextConversationPlayerName);
				PStrCopy( wszContextLabel, GlobalStringGet(GS_CHAT_CHANNEL_FORMAT_INPUT), CONSOLE_MAX_STR );
				PStrReplaceToken( wszContextLabel, CONSOLE_MAX_STR, StringReplacementTokensGet(SR_CHAT_CHANNEL), pConsole->m_wszNextConversationPlayerName );
				break;
			case CHAT_TYPE_PARTY:
				PStrPrintf(pConsole->m_wszChatPrefixCommand, arrsize(pConsole->m_wszChatPrefixCommand), L"/%s", puszCommand);
				PStrCopy( wszContextLabel, GlobalStringGet(GS_CHAT_CHANNEL_FORMAT_INPUT), CONSOLE_MAX_STR );
				PStrReplaceToken( wszContextLabel, CONSOLE_MAX_STR, StringReplacementTokensGet(SR_CHAT_CHANNEL), StringTableGetStringByKey("ui chat tab party") );
				break;
			case CHAT_TYPE_GUILD:
				PStrPrintf(pConsole->m_wszChatPrefixCommand, arrsize(pConsole->m_wszChatPrefixCommand), L"/%s", puszCommand);
				PStrCopy( wszContextLabel, GlobalStringGet(GS_CHAT_CHANNEL_FORMAT_INPUT), CONSOLE_MAX_STR );
				PStrReplaceToken( wszContextLabel, CONSOLE_MAX_STR, StringReplacementTokensGet(SR_CHAT_CHANNEL), StringTableGetStringByKey("ui chat tab guild") );
				break;
			case CHAT_TYPE_GAME:
				PStrPrintf(pConsole->m_wszChatPrefixCommand, arrsize(pConsole->m_wszChatPrefixCommand), L"/%s", puszCommand);
				PStrCopy( wszContextLabel, GlobalStringGet(GS_CHAT_CHANNEL_FORMAT_INPUT), CONSOLE_MAX_STR );
				PStrReplaceToken( wszContextLabel, CONSOLE_MAX_STR, StringReplacementTokensGet(SR_CHAT_CHANNEL), StringTableGetStringByKey("chat_context_say") );
				break;
			case CHAT_TYPE_CSR:
				PStrPrintf(pConsole->m_wszChatPrefixCommand, arrsize(pConsole->m_wszChatPrefixCommand), L"/%s", puszCommand);
				PStrCopy( wszContextLabel, GlobalStringGet(GS_CHAT_CHANNEL_FORMAT_INPUT), CONSOLE_MAX_STR );
				PStrReplaceToken( wszContextLabel, CONSOLE_MAX_STR, StringReplacementTokensGet(SR_CHAT_CHANNEL), StringTableGetStringByKey("csr_chat") );
				break;
			case CHAT_TYPE_SERVER:
			case CHAT_TYPE_SHOUT:
				if(pConsole->m_wszLastInstancingChannelName[0] != 0)
				{
					PStrPrintf(pConsole->m_wszChatPrefixCommand, arrsize(pConsole->m_wszChatPrefixCommand), L"/%s", pConsole->m_wszLastInstancingChannelName);
					WCHAR szInstancedChannel[MAX_CHAT_CNL_NAME];
					CHANNELID idChannel = GetInstancedChannel(pConsole->m_wszLastInstancingChannelName, szInstancedChannel);
					if (idChannel != INVALID_CHANNELID)
					{
						PStrCopy( wszContextLabel, GlobalStringGet(GS_CHAT_CHANNEL_FORMAT_INPUT), CONSOLE_MAX_STR );
						PStrReplaceToken( wszContextLabel, CONSOLE_MAX_STR, StringReplacementTokensGet(SR_CHAT_CHANNEL), szInstancedChannel );
					}
				}
				else
				{
					PStrPrintf(pConsole->m_wszChatPrefixCommand, arrsize(pConsole->m_wszChatPrefixCommand), L"/%s", puszCommand);
				}
				if( AppIsTugboat() )
				{
					PStrCopy( wszContextLabel, GlobalStringGet(GS_CHAT_CHANNEL_FORMAT_INPUT), CONSOLE_MAX_STR );
					PStrReplaceToken( wszContextLabel, CONSOLE_MAX_STR, StringReplacementTokensGet(SR_CHAT_CHANNEL), StringTableGetStringByKey("ui chat context shout") );
				}
				break;
			default:
				break;
		};
	}

	UI_COMPONENT * pConsoleEdit = UIComponentGetByEnum(UICOMP_CONSOLE_EDIT);
	UI_LABELEX * pChatContextLabel = UICastToLabel(UIComponentGetByEnum(UICOMP_CONSOLE_EDIT_LABEL));

	if (pConsoleEdit && pConsoleEdit->m_pParent && pChatContextLabel)
	{
		const float CHAT_ENTRY_BUFFER_SPACE = 5.0f;

		if (wszContextLabel[0])
		{
			UIComponentSetVisible(pChatContextLabel, TRUE);
			UILabelSetText(pChatContextLabel, wszContextLabel);
			pConsoleEdit->m_Position.m_fX = pChatContextLabel->m_fWidth + CHAT_ENTRY_BUFFER_SPACE;
			pConsoleEdit->m_fWidth = pConsoleEdit->m_pParent->m_fWidth - pChatContextLabel->m_fWidth - CHAT_ENTRY_BUFFER_SPACE;
		}
		else
		{
			UIComponentSetVisible(pChatContextLabel, FALSE);
			pConsoleEdit->m_Position.m_fX = 0.0f;
			pConsoleEdit->m_fWidth = pConsoleEdit->m_pParent->m_fWidth;
		}
	}

	ConsoleSetInputColor(ChatGetTypeColor(ConsoleGetChatContext()));

	pConsole->m_bKeepChatContext = FALSE;
	
}

static BOOL sConsoleCheckForContextChange(
	const WCHAR * szText )
{
#if !ISVERSION(DEVELOPMENT)
	if(AppIsHellgate() && AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{
		return FALSE;
	}
#endif

	WCHAR szInstancedChannel[MAX_CHAT_CNL_NAME];
	CHANNELID idChannel = GetInstancedChannel(szText, szInstancedChannel);
	if (idChannel != INVALID_CHANNELID)
	{
		CONSOLE * pConsole = gApp.m_pConsole;
		ConsoleSetInputLine(NULL, pConsole->m_dwInputColor);
		ConsoleSetLastInstancingChannelName(szText);
		ConsoleSetChatContext(CHAT_TYPE_SHOUT);
		sConsoleSetChatContext();
		return TRUE;
	}

	// special check first for a whisper with a recipient
	int nTextLen = PStrLen(szText);
	for (int i = 0; i < sgnNumChatContextLookup; ++i)
	{
		const CHAT_CONTEXT_COMMAND_LOOKUP *pLookup = &sgtChatContextLookup[ i ];
		const WCHAR * szCmdString = GlobalStringGet( pLookup->eGlobalString );
		int nCmdLen = PStrLen(szCmdString);
		if (!PStrICmp(szText, szCmdString, nCmdLen))
		{
			CONSOLE * pConsole = gApp.m_pConsole;
			if (pLookup->eChatType == CHAT_TYPE_WHISPER &&
				nTextLen > nCmdLen)
			{
				int nSpacePos = PStrCSpn(szText, L" ", arrsize(szText));	// find the space between the command and the recipient
				if (nSpacePos != -1 && nSpacePos+1 < nTextLen)
				{
					PStrCopy(pConsole->m_wszNextConversationPlayerName, &(szText[nSpacePos+1]), arrsize(pConsole->m_wszNextConversationPlayerName));
					ConsoleSetInputLine(NULL, pConsole->m_dwInputColor);
					ConsoleSetChatContext(pLookup->eChatType);
					sConsoleSetChatContext();
					return TRUE;
				}
			}
		}
	}

	for (int i = 0; i < sgnNumChatContextLookup; ++i)
	{
		const CHAT_CONTEXT_COMMAND_LOOKUP *pLookup = &sgtChatContextLookup[ i ];
		if (!PStrICmp(szText, GlobalStringGet( pLookup->eGlobalString )))
		{
			CONSOLE * pConsole = gApp.m_pConsole;
			CHAT_TYPE eChatType = pLookup->eChatType;
			if (eChatType == CHAT_TYPE_WHISPER)
			{
				// a whisper without a recipient, don't do anything yet
				return FALSE;
			}
			else if (eChatType == CHAT_TYPE_REPLY)
			{
				PStrCopy(pConsole->m_wszNextConversationPlayerName, gApp.m_wszLastWhisperSender, arrsize(pConsole->m_wszNextConversationPlayerName));
				eChatType = CHAT_TYPE_WHISPER;
			}
			ConsoleSetInputLine(NULL, pConsole->m_dwInputColor);
			ConsoleSetChatContext(eChatType);
			sConsoleSetChatContext();
			return TRUE;
		}
	}
	return FALSE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void ConsoleSetChatContext(
	CHAT_TYPE eChatContext)
{
	gApp.m_pConsole->m_eChatTypeContext= eChatContext;
	gApp.m_pConsole->m_bKeepChatContext = FALSE;
	ConsoleSetInputColor(ChatGetTypeColor(ConsoleGetChatContext()));
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CHAT_TYPE ConsoleGetChatContext(
	void )
{
	return gApp.m_pConsole->m_eChatTypeContext;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void ConsoleSetLastTalkedToPlayerName(
	const WCHAR * name )
{
	CONSOLE * pConsole = gApp.m_pConsole;

	PStrCopy(pConsole->m_wszLastConversationPlayerName,name,MAX_CHARACTER_NAME);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleSetCSRPlayerName(
	const WCHAR * name )
{
	CONSOLE * pConsole = gApp.m_pConsole;

	PStrCopy(pConsole->m_wszCSRPlayerName,name,MAX_CHARACTER_NAME);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleSetLastInstancingChannelName(
	const WCHAR * name )
{
	CONSOLE * pConsole = gApp.m_pConsole;

	PStrCopy(pConsole->m_wszLastInstancingChannelName,name,MAX_INSTANCING_CHANNEL_NAME);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ConsoleSubmitInput(
	const WCHAR * szText, 
	const WCHAR * szHistoryText, /*= NULL*/
	UI_LINE * pLine /*= NULL*/)
{
	if (szText && szText[0])
	{
		WCHAR szTempText[256];
		PStrCopy(szTempText, szText, 256);		// the text pointer might go away as a result of the command

		CONSOLE * console = gApp.m_pConsole;
		ASSERT_RETVAL(console, CR_FAILED);

		if (szHistoryText)
		{
			sConsoleAddHistory(console, szHistoryText);
		}
		else
		{
			sConsoleAddHistory(console, szTempText);
		}

		DWORD dwCResult = (ConsoleParseChat(szTempText, pLine));
		if(dwCResult == CR_NOT_COMMAND)
			dwCResult = ConsoleParseCommand(AppGetCltGame(), NULL, szTempText, pLine);
		BOOL bHandled = CRSUCCEEDED(dwCResult);

		// If we're on the client, and this is a client-only command, then don't even bother
		// sending it to the server.
		BOOL bSendCommand = !CRNOSEND(dwCResult);

		if (szTempText[0] != L'/')
		{
			bSendCommand = FALSE;
		}

		if (bSendCommand)
		{
			c_SendCheat(szTempText);
		}
		else
		{
			if (0 != (dwCResult & CRFLAG_CLEAR_CONSOLE))
			{
				ConsoleClear();
			}
			else if (!bHandled)
			{
				sConsoleHandleDefault(szTempText, pLine);
			}
		}

		return dwCResult;
	}

	return CR_NOT_COMMAND;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleHandleInputKeyDown(
	GAME * game,
	DWORD wParam,
	DWORD lParam)
{
#if !ISVERSION(SERVER_VERSION)

	UI_EDITCTRL *pEdit = sGetConsoleEdit();
	UIEditCtrlSetFocus(pEdit);

	UIComponentHandleUIMessage(pEdit, UIMSG_KEYDOWN, wParam, lParam);

#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleHandleInputChar(
	GAME * game,
	DWORD wParam,
	DWORD lParam)
{
#if !ISVERSION(SERVER_VERSION)

	UI_EDITCTRL *pEdit = sGetConsoleEdit();
	UIEditCtrlSetFocus(pEdit);

	CONSOLE * console = gApp.m_pConsole;
	ASSERT_RETURN(console);

	switch (wParam)
	{
	case VK_NEXT:  // Page down
	{
		if (lParam & 0x01000000 && AppIsHellgate())
		{
			if (!AppIsSinglePlayer())
			{
				UIChatOpen();
				UIPrevChatTab();
				ConsoleActivateEdit(FALSE, TRUE);
			}
		}
		else
		{
			ConsoleAddInput((WCHAR)wParam);
		}
		break;
	}
	case VK_PRIOR: // Page up
		{
			if (lParam & 0x01000000 && AppIsHellgate())
			{
				if (!AppIsSinglePlayer())
				{
					UIChatOpen();
					UINextChatTab();
					ConsoleActivateEdit(FALSE, TRUE);
				}
			}
			else
			{
				ConsoleAddInput((WCHAR)wParam);
			}
		break;
	}
	case VK_RETURN:
	{
		if (UIIsImeWindowsActive()) {
			return;
		}

		const WCHAR *szText = UILabelGetText(pEdit);

		if (console->m_wszChatPrefixCommand && console->m_wszChatPrefixCommand[0] && szText && szText[0] && szText[0] != L'/')
		{
			WCHAR szNewText[CONSOLE_MAX_STR];
			PStrPrintf(szNewText, CONSOLE_MAX_STR, L"%s %s", console->m_wszChatPrefixCommand, szText);
			ConsoleSubmitInput(szNewText, szText, &(pEdit->m_Line));
		}
		else
		{
			ConsoleSubmitInput(szText, NULL, &(pEdit->m_Line));
		}

		// Tricky, but get pEdit again on the off chance that the command reloaded the UI and invalidated the pointer
		pEdit = sGetConsoleEdit();

		if (pEdit)
		{
			const BOOL bWasEmpty = (!pEdit->m_Line.HasText());
			UILabelSetText(pEdit, L"");
			if (AppIsHellgate())
			{
				UIComponentActivate(pEdit, FALSE);
				UIComponentActivate(UICOMP_CHAT_TEXTENTRY_ACTIVE, FALSE, 0, NULL, FALSE, FALSE, TRUE, TRUE);
				UIComponentSetVisibleByEnum(UICOMP_CHAT_TEXTENTRY_ACTIVE, FALSE);

				if (UIComponentGetVisibleByEnum(UICOMP_CHAT_TEXT))
				{
					UIChatFadeOut(FALSE);
				}
				else
				{
					if (bWasEmpty)
					{
						UIComponentActivate(UICOMP_CHAT_PANEL, FALSE, 0, NULL, FALSE, FALSE, TRUE, TRUE);
					}
					else
					{
						UIChatOpen();
						UIChatFadeOut(TRUE);
					}
				}
			}
			else
			{
				UIComponentActivate(pEdit, FALSE);

				if( AppIsTugboat() )
				{
					UI_COMPONENT *pEditLabel = UIComponentGetByEnum(UICOMP_CONSOLE_EDIT_LABEL);
					UIComponentSetVisible(pEditLabel, FALSE);
				}

			}
		}
		break;
	}
	case VK_SPACE:
	{
		const WCHAR *szText = UILabelGetText(pEdit);
		if (szText && szText[0] == L'/' && szText[1])
		{
			// user typed a slash command followed by a space - check to see if it is one of the context commands
			if (sConsoleCheckForContextChange(&szText[1]))
			{
				break;
			}
		}
		UIEditCtrlOnKeyChar(pEdit, UIMSG_KEYCHAR, wParam, lParam);
		break;
	}
	case VK_ESCAPE:
	{
		if (UIIsImeWindowsActive()) {
			return;
		}

		UILabelSetText(pEdit, L"");

		if (UIComponentGetVisibleByEnum(UICOMP_CHAT_TEXT))
		{
			UIChatFadeOut(FALSE);
		}

//		UIComponentSetVisible(pEdit, FALSE);
		break;
	}
	case VK_TAB:
	{
		if (UIIsImeWindowsActive()) {
			return;
		}

		ConsoleAutoComplete(game, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
		break;
	}
	case VK_UP:
		if (UIIsImeWindowsActive()) {
			return;
		}

		if (lParam & 0x01000000)
		{
			sConsoleHistoryScrollUp(console);
		}
		else
		{
			ConsoleAddInput((WCHAR)wParam);
		}
		break;
	case VK_DOWN:
		if (UIIsImeWindowsActive()) {
			return;
		}

		if (lParam & 0x01000000)
		{
			sConsoleHistoryScrollDown(console);
		}
		else
		{
			ConsoleAddInput((WCHAR)wParam);
		}
		break;
	//default:
	//	ConsoleAddInput((WCHAR)wParam);
	//	break;
	//}
	default:
	{
		UIEditCtrlOnKeyChar(pEdit, UIMSG_KEYCHAR, wParam, lParam);
		break;
	}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConsoleActivateEdit(
	BOOL bAddSlash,
	BOOL setChatContext)
{
#if !ISVERSION(SERVER_VERSION)
	c_PlayerClearAllMovement(AppGetCltGame());


	ConsoleSetEditActive(TRUE);

	if(setChatContext)
	{
		sConsoleSetChatContext();
	}
	else 
	{
		UI_COMPONENT * pConsoleEdit = UIComponentGetByEnum(UICOMP_CONSOLE_EDIT);
		UI_LABELEX * pChatContextLabel = UICastToLabel(UIComponentGetByEnum(UICOMP_CONSOLE_EDIT_LABEL));
		if (pConsoleEdit && pChatContextLabel)
		{
			UIComponentSetVisible(pChatContextLabel, FALSE);
			pConsoleEdit->m_Position.m_fX = 0.0f;
			pConsoleEdit->m_fWidth = pConsoleEdit->m_pParent->m_fWidth;
			ConsoleSetInputColor(GetFontColor(FONTCOLOR_CHAT_GENERAL));
		}
	}

	if (bAddSlash) 
	{
		ConsoleAddInput((WCHAR)'/');
	}
#endif
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static inline void sConsoleString(
	DWORD dwColor,
	const WCHAR *str)
{
	ASSERT_RETURN(str);
	UI_TEXTBOX *pConsole = sGetConsole();

	if ( pConsole )
	{
//		ConsoleActivate(TRUE);
		UITextBoxAddLine(pConsole, str, dwColor);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static inline void sConsoleString(
	DWORD dwColor,
	const char * str)
{
	ASSERT_RETURN(str);
	WCHAR buf[CONSOLE_MAX_STR];
	PStrCvt(buf, str, CONSOLE_MAX_STR);
	sConsoleString(dwColor, buf);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleStringArgs(
	DWORD dwColor,
	const WCHAR * format,
	va_list & args)
{
	ASSERT_RETURN(format);
#if !ISVERSION(SERVER_VERSION)
	WCHAR buf[CONSOLE_MAX_STR];
	PStrVprintf(buf, CONSOLE_MAX_STR, format, args);

	sConsoleString(dwColor, buf);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleString(
	const WCHAR * format,
	...)
{
#if !ISVERSION(SERVER_VERSION)
	va_list args;
	va_start(args, format);

	ConsoleStringArgs(CONSOLE_SYSTEM_COLOR, format, args);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleString(
	DWORD dwColor,
	const WCHAR * format,
	...)
{
#if !ISVERSION(SERVER_VERSION)
	va_list args;
	va_start(args, format);

	ConsoleStringArgs(dwColor, format, args);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleStringForceOpen(
	DWORD dwColor,
	const WCHAR * format,
	...)
{
#if !ISVERSION(SERVER_VERSION)
	va_list args;
	va_start(args, format);

	ConsoleActivate(TRUE);
	ConsoleStringArgs(dwColor, format, args);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleString1(
	DWORD dwColor,
	const WCHAR * str)
{
#if !ISVERSION(SERVER_VERSION)
	sConsoleString(dwColor, str);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleStringArgs(
	DWORD dwColor,
	const char * format,
	va_list & args)
{
	ASSERT_RETURN(format);
#if !ISVERSION(SERVER_VERSION)
	char buf[CONSOLE_MAX_STR];
	PStrVprintf(buf, CONSOLE_MAX_STR, format, args);

	sConsoleString(dwColor, buf);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleString(
	const char * format,
	...)
{
#if !ISVERSION(SERVER_VERSION)
	va_list args;
	va_start(args, format);

	ConsoleStringArgs(CONSOLE_SYSTEM_COLOR, format, args);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleString(
	DWORD dwColor,
	const char *format,
	...)
{
#if !ISVERSION(SERVER_VERSION)
	va_list args;
	va_start(args, format);

	ConsoleStringArgs(dwColor, format, args);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleString1(
	DWORD dwColor,
	const char *str)
{
#if !ISVERSION(SERVER_VERSION)
	sConsoleString(dwColor, str);
#endif
}


//----------------------------------------------------------------------------
// TODO: this could be better by having it print directly to the msg buffer
// if chat.h were to provide a version which takes a MSG_SCMD_CHAT as an arg
//----------------------------------------------------------------------------
void ConsoleString(
	GAME * game,
	GAMECLIENT * client,
	DWORD color,
	const WCHAR * format,
	...)
{
	if (game && IS_SERVER(game))
	{
		va_list args;
		va_start(args, format);

		WCHAR strText[MAX_CHEAT_LEN];
		PStrVprintf(strText, MAX_CHEAT_LEN, format, args);
		s_SendSysTextFmt1(CHAT_TYPE_WHISPER, TRUE, game, client, color, strText);
	}
#if !ISVERSION(SERVER_VERSION)
	else
	{
		va_list args;
		va_start(args, format);

		ConsoleStringArgs(color, format, args);
	}
#endif
}


//----------------------------------------------------------------------------
// TODO: this could be better by having it print directly to the msg buffer
// if chat.h were to provide a version which takes a MSG_SCMD_CHAT as an arg
//----------------------------------------------------------------------------
void ConsoleString1(
	GAME * game,
	GAMECLIENT * client,
	DWORD color,
	const WCHAR * str)
{
	if (game && IS_SERVER(game))
	{
		s_SendSysTextFmt1(CHAT_TYPE_WHISPER, TRUE, game, client, color, str);
	}
#if !ISVERSION(SERVER_VERSION)
	else
	{
		ConsoleString1(color, str);
	}
#endif
}


//----------------------------------------------------------------------------
// TODO: this could be better by having it print directly to the msg buffer
// if chat.h were to provide a version which takes a MSG_SCMD_CHAT as an arg
//----------------------------------------------------------------------------
void ConsoleStringAndTrace(
	GAME * game,
	GAMECLIENT * client,
	DWORD color,
	const WCHAR * format,
	...)
{
	if (game && IS_SERVER(game))
	{
		va_list args;
		va_start(args, format);

		WCHAR strText[MAX_CHEAT_LEN];
		PStrVprintf(strText, arrsize(strText), format, args);
		s_SendSysTextFmt1(CHAT_TYPE_WHISPER, TRUE, game, client, color, strText);

		char chText[MAX_CHEAT_LEN];
		PStrCvt(chText, strText, arrsize(chText));
		trace1(chText);
	}
#if !ISVERSION(SERVER_VERSION)
	else
	{
		va_list args;
		va_start(args, format);

		WCHAR strText[MAX_CHEAT_LEN];
		PStrVprintf(strText, arrsize(strText), format, args);
		ConsoleString1(color, strText);

		char chText[MAX_CHEAT_LEN];
		PStrCvt(chText, strText, arrsize(chText));
		trace1(chText);
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sConsoleDebugString(
	OUTPUT_PRIORITY ePriority,
	const WCHAR * str)
{
#if ISVERSION(DEVELOPMENT)
	if (ePriority == OP_DEBUG)
	{
		int nLen = PStrLen(str, CONSOLE_MAX_STR);
		if (str[nLen] != L'\n')
		{
			trace("%S\n", str);
		}
		else
		{
			trace("%S", str);
		}
	}
#endif

#if !ISVERSION(SERVER_VERSION)
	if (!gApp.m_pConsole)
	{
		return;
	}
	GLOBAL_DEFINITION * global = DefinitionGetGlobal();
	ASSERT_RETURN(global);

	if (!(ePriority == OP_DEBUG || global->nDebugOutputLevel <= (int)ePriority))
	{
		return;
	}

	static const DWORD OUTPUT_COLORS[NUM_OUTPUT_PRIORITY_LEVELS] =
	{
		GFXCOLOR_CYAN,		// debug
		GFXCOLOR_GRAY,		// low
		GFXCOLOR_WHITE,		// mid
		GFXCOLOR_YELLOW,	// high
		GFXCOLOR_RED,		// error
	};

	ConsoleString(OUTPUT_COLORS[ePriority], str);

	// right now, only do this when adding a string
	GAME * game = AppGetCltGame();
	if (!game || AppGetState() != APP_STATE_IN_GAME || game->eGameState != GAMESTATE_LOADING)
	{
		return;
	}

	UIUpdateLoadingScreen();
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sConsoleDebugString(
	OUTPUT_PRIORITY ePriority,
	const char * str)
{
	WCHAR buf[CONSOLE_MAX_STR];
	PStrCvt(buf, str, CONSOLE_MAX_STR);
	sConsoleDebugString(ePriority, buf);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleDebugStringV(
	OUTPUT_PRIORITY ePriority,
	const WCHAR * format,
	va_list & args)
{
	WCHAR buf[CONSOLE_MAX_STR];
	PStrVprintf(buf, CONSOLE_MAX_STR, format, args);

	sConsoleDebugString(ePriority, buf);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleDebugString(
	OUTPUT_PRIORITY ePriority,
	const WCHAR * format,
	...)
{
	va_list args;
	va_start(args, format);

	ConsoleDebugStringV(ePriority, format, args);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleDebugString1(
	OUTPUT_PRIORITY ePriority,
	const WCHAR * str)
{
	sConsoleDebugString(ePriority, str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleDebugStringV(
	OUTPUT_PRIORITY ePriority,
	const char * format,
	va_list & args)
{
	char buf[CONSOLE_MAX_STR];
	PStrVprintf(buf, CONSOLE_MAX_STR, format, args);

	sConsoleDebugString(ePriority, buf);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleDebugString(
	OUTPUT_PRIORITY ePriority,
	const char * format,
	...)
{
	va_list args;
	va_start(args, format);

	ConsoleDebugStringV(ePriority, format, args);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleDebugString(
	OUTPUT_PRIORITY ePriority,
	const char * str)
{
	sConsoleDebugString(ePriority, str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ConsoleMessage(
	UNIT *pUnit,
	GLOBAL_STRING eGlobalString)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// get client id
	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	ASSERTX_RETURN( idClient != INVALID_ID, "Expected client id" );
	
	// setup message
	MSG_SCMD_CONSOLE_MESSAGE tMessage;
	tMessage.nGlobalString = (int)eGlobalString;
	
	// send to client
	s_SendUnitMessageToClient( pUnit, idClient, SCMD_CONSOLE_MESSAGE, &tMessage );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ConsoleMessage(
	GLOBAL_STRING eGlobalString)
{
	const WCHAR *puszText = GlobalStringGet( eGlobalString );
	ConsoleString( puszText );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ConsoleDialogMessage(
	UNIT *pUnit,
	int nDialogID)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// get client id
	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	ASSERTX_RETURN( idClient != INVALID_ID, "Expected client id" );
	
	// setup message
	MSG_SCMD_CONSOLE_DIALOG_MESSAGE tMessage;
	tMessage.nDialogID = nDialogID;
	
	// send to client
	s_SendUnitMessageToClient( pUnit, idClient, SCMD_CONSOLE_DIALOG_MESSAGE, &tMessage );
	
}	