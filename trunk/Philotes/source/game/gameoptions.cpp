//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

#if !ISVERSION(SERVER_VERSION)

#include "gameoptions.h"
#include "settings.h"
#include "c_particles.h"
#include "uix_priv.h"
#include "c_input.h"
#include "config.h"
#include "uix_chat.h"

namespace
{

const char GAMEOPTIONS_NAME[] = "Game";

GAMEOPTIONS sOptions;

//-----------------------------------------------------------------------------

bool GameOptions_SetNoSave(const GAMEOPTIONS & OptsIn)
{
	GAMEOPTIONS tOpts = OptsIn;

	AppSetCensorFlag( AF_CENSOR_PARTICLES, tOpts.bCensorshipEnabled, FALSE );
	AppSetCensorFlag( AF_CENSOR_NO_BONE_SHRINKING, tOpts.bCensorshipEnabled, FALSE );

	UISetFlag(UI_FLAG_ALT_TARGET_INFO, TRUE /*tOpts.bAltTargetInfo*/);
	UISetFlag(UI_FLAG_SHOW_TARGET_BRACKETS, TRUE/*tOpts.bAltTargeting*/);
	UISetUIScaler(tOpts.nUIScaler);
	InputSetMouseInverted(tOpts.bInvertMouse);
	InputSetMouseSensitivity(tOpts.fMouseSensitivity);
	InputSetMouseLookSensitivity(tOpts.fMouseLookSensitivity);
	UIChatSetAlpha((BYTE)tOpts.nChatAlpha);
	UIMapSetAlpha((BYTE)tOpts.nMapAlpha);
	InputSetAdvancedCamera(tOpts.bAdvancedCamera);
	InputSetLockedPitch(tOpts.bLockedPitch);
	InputSetFollowCamera(tOpts.bFollowCamera);

	GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
	if (tOpts.bRotateAutomap)
	{
		pGlobal->dwFlags |= GLOBAL_FLAG_AUTOMAP_ROTATE;
	}
	else
	{
		pGlobal->dwFlags &= ~GLOBAL_FLAG_AUTOMAP_ROTATE;
	}

	tOpts.Validate();

	bool bDiffer = (sOptions != tOpts);
	sOptions = tOpts;

	return bDiffer;
}

//-----------------------------------------------------------------------------

};

//-----------------------------------------------------------------------------

void GameOptions_Get(GAMEOPTIONS & OptsOut)
{
	OptsOut = sOptions;
}

//-----------------------------------------------------------------------------

void GameOptions_Set(const GAMEOPTIONS & OptsIn)
{
	if (GameOptions_SetNoSave(OptsIn))
	{
		GameOptions_Save();
	}
}

//-----------------------------------------------------------------------------

GAMEOPTIONS::GAMEOPTIONS(void)
{
	bCensorshipEnabled = FALSE;
	bAltTargetInfo = TRUE;
	bAltTargeting = TRUE;
	bInvertMouse = FALSE;
	bRotateAutomap = FALSE;
	fAutomapScale = 0.0f;
	nAutomapZoomLevel = 0;
	fMouseSensitivity = 1.0f;
	fMouseLookSensitivity = 1.0f;
	nChatAlpha = 150;
	nMapAlpha = 80;
	nUIScaler = 0;
	bEnableCinematicSubtitles = FALSE;
	bDisableAdUpdates = FALSE;
	bChatAutoFade = TRUE;
	bShowChatBubbles = AppIsHellgate() ? FALSE : TRUE;
	bAbbreviateChatName = FALSE;
	bAdvancedCamera = FALSE;
	bLockedPitch = TRUE;
	bFollowCamera = FALSE;
}

void GAMEOPTIONS::Exchange(SettingsExchange & se)
{
	SETTINGS_EX(se, bCensorshipEnabled);
	SETTINGS_EX(se, bAltTargetInfo);
	SETTINGS_EX(se, bAltTargeting);
	SETTINGS_EX(se, bInvertMouse);
	SETTINGS_EX(se, bRotateAutomap);
	SETTINGS_EX(se, fAutomapScale);
	SETTINGS_EX(se, nAutomapZoomLevel);
	SETTINGS_EX(se, bEnableCinematicSubtitles);
	SETTINGS_EX(se, bDisableAdUpdates);
	SETTINGS_EX(se, bChatAutoFade);
	SETTINGS_EX(se, bShowChatBubbles);
	SETTINGS_EX(se, fMouseSensitivity);
	SETTINGS_EX(se, fMouseLookSensitivity);
	SETTINGS_EX(se, nChatAlpha);
	SETTINGS_EX(se, nUIScaler);
	SETTINGS_EX(se, bAbbreviateChatName);
	SETTINGS_EX(se, nMapAlpha);
	SETTINGS_EX(se, bAdvancedCamera);
	SETTINGS_EX(se, bLockedPitch);
	SETTINGS_EX(se, bFollowCamera);
}

void GAMEOPTIONS::Validate(void)
{
	// nothing to validate yet...
}

bool GAMEOPTIONS::operator==(const GAMEOPTIONS & rhs) const
{
	return memcmp(this, &rhs, sizeof(*this)) == 0;
}

bool GAMEOPTIONS::operator!=(const GAMEOPTIONS & rhs) const
{
	return !(*this == rhs);
}

//-----------------------------------------------------------------------------

static
void sExchangeFunc(SettingsExchange & se, void * pContext)
{
	static_cast<GAMEOPTIONS *>(pContext)->Exchange(se);
}

void GameOptions_Load(void)
{
	GAMEOPTIONS tOpts;
	if (FAILED(Settings_Load(GAMEOPTIONS_NAME, &sExchangeFunc, &tOpts)))
	{
		// Nothing to do.
	}
	else
	{
		GameOptions_SetNoSave( tOpts );
	}
}

void GameOptions_Save(void)
{
	if (FAILED(Settings_Save(GAMEOPTIONS_NAME, &sExchangeFunc, &sOptions)))
	{
		//???
	}
}

//-----------------------------------------------------------------------------

#endif
