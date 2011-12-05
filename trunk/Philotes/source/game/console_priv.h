//----------------------------------------------------------------------------
// console_priv.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _CONSOLE_PRIV_H_
#define _CONSOLE_PRIV_H_


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

BOOL ConsoleInitBuffer(
	void);

void ConsoleFree(
	void);

#if !ISVERSION(SERVER_VERSION)

BOOL ConsoleInitInterface(
	void);

BOOL ConsoleIsEditActive(
	void);

void ConsoleSetEditActive(
	BOOL bActive);

void ConsoleHandleInputChar(
	struct GAME * game,
	DWORD wParam,
	DWORD lParam);

BOOL ConsoleActivateEdit(
	BOOL bAddSlash,
	BOOL setChatContext);

void ConsoleActivate(
	BOOL bActivate);

BOOL ConsoleIsActive(
	void);

void ConsoleToggle(
	void);

void ConsoleVersionString(
	void);

void ConsoleSetInputLine(
	const WCHAR * pszNewLine,
	DWORD dwColor,
	int offset = 0);

const WCHAR * ConsoleGetInputLine(
	void);
#endif //!SERVER_VERSION


DWORD ConsoleErrorColor(
	void);

DWORD ConsoleInputColor(
	void);

#endif // _CONSOLE_PRIV_H_