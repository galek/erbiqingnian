//-----------------------------------------------------------------------------
//
// System for storage and retrieval of settings.
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef _SETTINGSEXCHANGE_H_
#define _SETTINGSEXCHANGE_H_

//
#define SETTINGSEXCHANGE(se, v)	SettingsExchange_Do((se), (v), #v)

//
class SettingsExchange
{
	public:
		bool IsLoading(void) const { return !IsSaving(); }
		virtual bool IsSaving(void) const = 0;
		virtual ~SettingsExchange(void) throw() {}
};

// Type for exchange callback function.
typedef void (SETTINGSEXCHANGE_CALLBACK)(SettingsExchange & se, void * pContext);

// Load and save.
PRESULT SettingsExchange_LoadFromFile(const OS_PATH_CHAR * pFileName, const char * pName, SETTINGSEXCHANGE_CALLBACK * pExchangeFunc, void * pContext);
PRESULT SettingsExchange_SaveToFile(const OS_PATH_CHAR * pFileName, const char * pName, SETTINGSEXCHANGE_CALLBACK * pExchangeFunc, void * pContext);

// Various standard exchange functions.
// Prefer using the SETTINGSEXCHANGE macro unless otherwise necessary.
void SettingsExchange_Do(SettingsExchange & se, int & v, const char * pName);
void SettingsExchange_Do(SettingsExchange & se, unsigned & v, const char * pName);
void SettingsExchange_Do(SettingsExchange & se, float & v, const char * pName);
void SettingsExchange_Do(SettingsExchange & se, bool & v, const char * pName);
void SettingsExchange_Do(SettingsExchange & se, unsigned long & v, const char * pName);
void SettingsExchange_Do(SettingsExchange & se, char * pStringBuffer, unsigned nStringBufferChars, const char * pName);

BOOL SettingsExchange_Category(SettingsExchange & se, const char * pName);
void SettingsExchange_CategoryEnd(SettingsExchange & se);

#endif
