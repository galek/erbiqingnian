//----------------------------------------------------------------------------
// c_input.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _C_INPUT_H_
#define _C_INPUT_H_

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

#define KEY_HANDLER_PARAMS		GAME* game, UNIT* unit, int command, DWORD wKey, DWORD lParam, DWORD_PTR data
typedef BOOL (*FP_KEY_HANDLER)(struct GAME* game, struct UNIT* unit, int command, DWORD wKey, DWORD lParam, DWORD_PTR data);


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
enum {
	MOUSE_BUTTON_LEFT = 0,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_MIDDLE,
	MOUSE_BUTTON_X1,
	MOUSE_BUTTON_X2,

	MOUSE_BUTTON_COUNT
};

enum APP_GAME;

// Hellgate virtual keys
	// windows VKs end at 0xFF
#define HGVK_MOUSE_WHEEL_DOWN	0x0100
#define HGVK_MOUSE_WHEEL_UP		0x0101
#define HGVK_SCREENSHOT_UI		0x0102
#define HGVK_SCREENSHOT_NO_UI	0x0103


#define KEY_COMMAND_ENUM
enum 
{
#include "key_command_priv.h"
	NUM_KEY_COMMANDS
};
#undef KEY_COMMAND_ENUM


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct KEY_HANDLER
{
	int				nCommand;
	BOOL			m_bGameRequired;
	BOOL			m_bUnitRequired;
	FP_KEY_HANDLER	fpOnKeyDown;
	FP_KEY_HANDLER	fpOnKeyUp;
	FP_KEY_HANDLER	fpOnKeyChar;
	DWORD_PTR		data;
	APP_GAME		eGame;

#if ISVERSION(DEVELOPMENT)
	WCHAR			szCommand[256];
	WCHAR			szOnKeyDown[256];
	WCHAR			szOnKeyUp[256];
	WCHAR			szOnKeyChar[256];
#endif

	BOOL			bNoRepeats;
	BOOL			bIsDown;
};

struct KEY_ASSIGNMENT
{
	int		nKey;
	int		nModifierKey;
};

#define NUM_KEYS_PER_COMMAND	2

struct KEY_COMMAND
{
	int				nCommand;
	KEY_ASSIGNMENT	KeyAssign[NUM_KEYS_PER_COMMAND];
	BOOL			bRemappable;
	BOOL			bCheckConflicts;
	DWORD			dwParam;
	const char *	szNameStringKey;
	int				nStringIndex;
	const char *	szCommandStr;
	APP_GAME		eGame;
};

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
BOOL c_InputInit(
	HWND hWnd,
	HINSTANCE hInstance);

void c_InputClose(
	void);

BOOL c_InputMouseAcquire(
	GAME * game,
	BOOL bAcquire);

void c_DirectInputUpdate(
	GAME* game);

void c_ToggleInvertMouse();


void InputSetMousePosition(
	float x,
	float y);

void InputResetMousePosition();

LRESULT c_InputHandleKeys(
	GAME* game,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

LRESULT c_InputHandleMouseButton(
	GAME* game,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

LRESULT c_InputHandleMouseMove(
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

LRESULT c_InputHandleNCMouseMove(
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

BOOL InputGetMouseButtonDown(
	void);

BOOL InputGetMouseButtonDown(
	int nButton);

KEY_COMMAND & InputGetKeyCommand(
	int nCommand);

const KEY_COMMAND * InputGetKeyCommand(
	const char *szCommanName);

BOOL InputExecuteKeyCommand(
	int nCommand,
	DWORD lParam = 0);

BOOL GetMouseButtonStateImmediate(
	BYTE *pByte);

void InputBeginKeyRemap(
	int nCommand,
	int nKeySlot);

void InputEndKeyRemap(
	void);

BOOL InputIsRemappingKey(
	void);

void InputFlushKeyboardBuffer(
	void);

BOOL InputGetKeyNameW(
	int nVirtualKey, 
	int nModifierKey, 
	WCHAR *szRetString, 
	int nStrLen);

void InputKeyIgnore(
	DWORD dwDuration);

void InputMouseBtnIgnore(
	DWORD dwDuration);

BOOL InputGetMouseInverted(
	void);

void InputSetMouseInverted(
	BOOL value);

BOOL InputIsCommandKeyDown(
	int nCommand);

void InputSetMouseSensitivity(
	float fSensitivity);

void InputSetMouseLookSensitivity(
	float fSensitivity);

void InputSetAdvancedCamera(
	BOOL value );

void InputSetLockedPitch(
	BOOL value );

void InputSetFollowCamera(
	BOOL value );

BOOL InputGetAdvancedCamera(
	void );

BOOL InputGetLockedPitch(
	void );


BOOL InputGetFollowCamera(
	void );

BOOL c_SetUsingDirectInput(
	BOOL bUse);

void InputSetInteracted(
	void );

#endif // _C_INPUT_H_
