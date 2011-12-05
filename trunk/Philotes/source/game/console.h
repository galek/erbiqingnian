//----------------------------------------------------------------------------
// console.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _CONSOLE_H_
#define _CONSOLE_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _FONTCOLOR_H_
#include "fontcolor.h"
#endif

#ifndef _C_FONT_H_
#include "c_font.h"
#endif

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
enum GLOBAL_STRING;
struct UNIT;
enum CHAT_TYPE;
class UI_LINE;

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define CONSOLE_SYSTEM_COLOR			GetFontColor(FONTCOLOR_CONSOLE_SYSTEM)
#define CONSOLE_ERROR_COLOR				GetFontColor(FONTCOLOR_CONSOLE_ERROR)
#define CONSOLE_AI_COLOR				GetFontColor(FONTCOLOR_LOG_AI)
#define CONSOLE_COMBATLOG_PLAYER		GetFontColor(FONTCOLOR_LOG_COMBAT_PLAYER)
#define CONSOLE_COMBATLOG_MONSTER		GetFontColor(FONTCOLOR_LOG_COMBAT_MONSTER)
#define CONSOLE_QUEST_LOG_COLOR			GetFontColor(FONTCOLOR_QUEST_LOG_TYPE_HEADER)
#define CONSOLE_CSR_COLOR				GetFontColor(FONTCOLOR_CHAT_CSR)
#define CONSOLE_MAX_STR					512


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//	client only methods
DWORD ConsoleSubmitInput(
	const WCHAR * szText,
	const WCHAR * szHistoryText = NULL,
	UI_LINE * pLine = NULL);

void ConsoleString(
	const WCHAR * format,
	...);

void ConsoleString(
	DWORD dwColor,
	const WCHAR * format,
	...);

void ConsoleString(
	const char * format,
	...);

void ConsoleString(
	DWORD dwColor,
	const char * format,
	...);

void ConsoleString1(
	DWORD dwColor,
	const WCHAR * str);

void ConsoleString1(
	DWORD dwColor,
	const char * str);

void ConsoleStringArgs(
	DWORD dwColor,
	const char * format,
	va_list & args);

void ConsoleDebugStringV(
	OUTPUT_PRIORITY ePriority,
	const WCHAR * format,
	va_list & args);

void ConsoleDebugStringV(
	OUTPUT_PRIORITY ePriority,
	const char * format,
	va_list & args);

void ConsoleDebugString(
	OUTPUT_PRIORITY ePriority,
	const WCHAR * format,
	...);

void ConsoleDebugString(
	OUTPUT_PRIORITY ePriority,
	const char * format,
	...);

void ConsoleDebugString1(
	OUTPUT_PRIORITY ePriority,
	const WCHAR * str);

void ConsoleDebugString1(
	OUTPUT_PRIORITY ePriority,
	const char * str);

void ConsoleHandleInputKeyDown(
	GAME * game,
	DWORD wParam,
	DWORD lParam);

void ConsoleHandleInputChar(
	struct GAME * game,
	DWORD wParam,
	DWORD lParam);

void ConsoleSetChatContext(
	CHAT_TYPE eChatContext);

CHAT_TYPE ConsoleGetChatContext(
	void );

void ConsoleSetLastTalkedToPlayerName(
	const WCHAR * name );

void ConsoleSetCSRPlayerName(
	const WCHAR * name );

void ConsoleSetLastInstancingChannelName(
	const WCHAR * name );

void ConsoleSetInputColor(
	DWORD color );

void ConsoleClear(
	void);

//	common methods
void ConsoleString(
	struct GAME * game,
	struct GAMECLIENT * client,
	DWORD color,
	const WCHAR * format,
	...);

void ConsoleString1(
	struct GAME * game,
	struct GAMECLIENT * client,
	DWORD color,
	const WCHAR * str);

void ConsoleStringAndTrace(
	struct GAME * game,
	struct GAMECLIENT * client,
	DWORD color,
	const WCHAR * format,
	...);

float ConsoleGetDamagePerSecond();

void s_ConsoleMessage(
	UNIT *pUnit,
	GLOBAL_STRING eGlobalString);

void s_ConsoleDialogMessage(
	UNIT *pUnit,
	int nDialogID);

void c_ConsoleMessage(
	GLOBAL_STRING eGlobalString);

void ConsoleStringForceOpen(
	DWORD dwColor,
	const WCHAR * format,
	...);

#endif // _CONSOLE_H_
