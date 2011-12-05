//-----------------------------------------------------------------------------
//
// System for storage and retrieval of persistent user settings.
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "settings.h"
#include "filepaths.h"

//-----------------------------------------------------------------------------

namespace
{

bool s_bFirstRun = false;

};

//-----------------------------------------------------------------------------

void Settings_GetDefaultSettingsFileName(OS_PATH_CHAR * pFileName, unsigned nFileNameChars)
{
	PStrPrintf(pFileName, nFileNameChars, OS_PATH_TEXT("%ssettings.xml"), FilePath_GetSystemPath(FILE_PATH_USER_SETTINGS));
}

void Settings_InitAndLoad(void)
{
	Settings_Init();
	if (!Settings_IsFirstRun())
	{
		Settings_LoadAll();
	}
}

void Settings_Init(void)
{
	OS_PATH_CHAR FileName[MAX_PATH];
	Settings_GetDefaultSettingsFileName(FileName, _countof(FileName));
	s_bFirstRun = !FileExists(FileName);
}

PRESULT Settings_Load(const char * pName, SETTINGSEXCHANGE_CALLBACK * pExchangeFunc, void * pContext)
{
	OS_PATH_CHAR FileName[MAX_PATH];
	Settings_GetDefaultSettingsFileName(FileName, _countof(FileName));
	return SettingsExchange_LoadFromFile(FileName, pName, pExchangeFunc, pContext);
}

PRESULT Settings_Save(const char * pName, SETTINGSEXCHANGE_CALLBACK * pExchangeFunc, void * pContext)
{
	OS_PATH_CHAR FileName[MAX_PATH];
	Settings_GetDefaultSettingsFileName(FileName, _countof(FileName));
	return SettingsExchange_SaveToFile(FileName, pName, pExchangeFunc, pContext);
}

bool Settings_IsFirstRun(void)
{
	return s_bFirstRun;
}
