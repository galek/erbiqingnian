//-----------------------------------------------------------------------------
//
// System for storage and retrieval of persistent user settings.
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "settingsexchange.h"

#define SETTINGS_EX(se, v) SETTINGSEXCHANGE(se, v)

//
void Settings_GetDefaultSettingsFileName(OS_PATH_CHAR * pFileName, unsigned nFileNameChars);

// Be sure to validate all values after loading!
PRESULT Settings_Load(const char * pName, SETTINGSEXCHANGE_CALLBACK * pExchangeFunc, void * pContext);
PRESULT Settings_Save(const char * pName, SETTINGSEXCHANGE_CALLBACK * pExchangeFunc, void * pContext);

// Returns true if the settings file had to be created (e.g., the user's first time running the game).
bool Settings_IsFirstRun(void);

//
void Settings_Init(void);
void Settings_InitAndLoad(void);
void Settings_LoadAll(void);

#endif
