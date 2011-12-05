//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef _GAMEOPTIONS_H_
#define _GAMEOPTIONS_H_

#if !ISVERSION(SERVER_VERSION)

class SettingsExchange;

#define MOUSE_CURSOR_SENSITIVITY_MIN	0.1f
#define MOUSE_CURSOR_SENSITIVITY_MAX	2.0f

#define MOUSE_LOOK_SENSITIVITY_MIN		0.1f
#define MOUSE_LOOK_SENSITIVITY_MAX		2.0f

#define CHAT_TRANSPARENCY_MIN			16
#define CHAT_TRANSPARENCY_MAX			255

struct GAMEOPTIONS
{
	bool bCensorshipEnabled;
	bool bAltTargetInfo;
	bool bAltTargeting;
	bool bInvertMouse;
	float fMouseSensitivity;
	float fMouseLookSensitivity;
	float fAutomapScale;
	int  nAutomapZoomLevel;
	int  nChatAlpha;
	int  nMapAlpha;
	int  nUIScaler;
	bool bRotateAutomap;
	bool bEnableCinematicSubtitles;
	bool bDisableAdUpdates;
	bool bChatAutoFade;
	bool bShowChatBubbles;
	bool bAbbreviateChatName;
	bool bAdvancedCamera;
	bool bLockedPitch;
	bool bFollowCamera;

	GAMEOPTIONS(void);
	void Exchange(SettingsExchange & se);
	void Validate(void);
	bool operator==(const GAMEOPTIONS & rhs) const;
	bool operator!=(const GAMEOPTIONS & rhs) const;
};

void GameOptions_Get(GAMEOPTIONS & OptsOut);
void GameOptions_Set(const GAMEOPTIONS & Opts);
void GameOptions_Load(void);
void GameOptions_Save(void);

#else

#define GameOptions_Get(x)				((void)0)
#define GameOptions_Set(x)				((void)0)
#define GameOptions_Load()				((void)0)
#define GameOptions_Save()				((void)0)

#endif

#endif
