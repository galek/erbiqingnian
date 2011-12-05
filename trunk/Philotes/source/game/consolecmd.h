//----------------------------------------------------------------------------
// consolecmd.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _CONSOLECMD_H_
#define _CONSOLECMD_H_


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define CONSOLE_COMMAND_PREFIX			L'/'

// console comand return values
#define CR_NOT_COMMAND					0x00000000
#define CR_FAILED						0x00000000
#define CR_OK							MAKE_MASK(1)
#define CRFLAG_BADPARAMS				MAKE_MASK(2)
#define CRFLAG_NOSEND					MAKE_MASK(3)
#define CRFLAG_SCRIPT					MAKE_MASK(4)
#define CRFLAG_CLEAR_CONSOLE			MAKE_MASK(5)

// and associated macros
#define CRSUCCEEDED(cr)					(CR_OK == ((cr) & CR_OK))
#define CRBADPARAMS(cr)					(CRFLAG_BADPARAMS == ((cr) & CRFLAG_BADPARAMS))
#define CRNOSEND(cr)					(CRFLAG_NOSEND == ((cr) & CRFLAG_NOSEND))		

#define LOCALIZED_CMD_OUTPUT(str, ...)  UIAddChatLineArgs (CHAT_TYPE_LOCALIZED_CMD, ChatGetTypeColor(CHAT_TYPE_SERVER), str, __VA_ARGS__)

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
class UI_LINE;

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
enum FONTCOLOR;

void ConsoleCommandTableInit(
	void);

void ConsoleCommandTableFree(
	void);

BOOL ConsoleAutoComplete(
	GAME * game,
	BOOL bReverse);

DWORD ConsoleParseCommand(
	struct GAME * game,
	struct GAMECLIENT * client,
	const WCHAR * str,
	UI_LINE * pLine = NULL);

DWORD ConsoleParseChat(
	const WCHAR * strInput,
	UI_LINE * pLine = NULL);

void c_ConsoleGetUsageString(
	WCHAR * strUsage,
	const int nMaxLen,
	const WCHAR * strCmd,
	FONTCOLOR eTextColor);

void c_ConsoleGetDescString(
	WCHAR * strDesc,
	const int nMaxLen,
	const WCHAR * strCmd,
	FONTCOLOR eTextColor);

void ConsoleAddItem(
	UNIT *pItem);

void c_sUIReload(
	BOOL bReloadDirect);

#endif // _CONSOLECMD_H_

