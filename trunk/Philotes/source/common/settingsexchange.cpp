//-----------------------------------------------------------------------------
//
// System for storage and retrieval of settings.
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------


#include "settingsexchange.h"
#include "markup.h"
#include "fileio.h"

#pragma warning(disable:4481)	// warning C4481: nonstandard extension used: override specifier 'override'

//-----------------------------------------------------------------------------

namespace
{

//-----------------------------------------------------------------------------

const char s_RootElementName[] = "Settings";

//-----------------------------------------------------------------------------

class SettingsExchange_Private : public SettingsExchange
{
	public:
		virtual bool IsSaving(void) const override;
		
		SettingsExchange_Private(CMarkup & xml, bool bSaving);
		virtual ~SettingsExchange_Private(void);

		CMarkup & xml;
		bool bSaving;
};

bool SettingsExchange_Private::IsSaving(void) const
{
	return bSaving;
}

SettingsExchange_Private::SettingsExchange_Private(CMarkup & xmlIn, bool bSavingIn)
	: xml(xmlIn), bSaving(bSavingIn)
{
}

SettingsExchange_Private::~SettingsExchange_Private(void)
{
}

//-----------------------------------------------------------------------------

void _ToString(char * pBuffer, unsigned nBufferChars, int Value)
{
	_itoa_s(Value, pBuffer, nBufferChars, 10);
}

void _FromString(const char * pBuffer, int & Value)
{
	Value = atoi(pBuffer);
}

void _ToString(char * pBuffer, unsigned nBufferChars, float Value)
{
	PStrPrintf(pBuffer, nBufferChars, "%3.2f", Value);
	//const float fRound = (Value < 0.0f) ? -0.5f : 0.5f;
	//_ToString(pBuffer, nBufferChars, static_cast<int>(Value * 100.0f + fRound));
	//PStrCat(pBuffer, "%", nBufferChars);
}

void _FromString(const char * pBuffer, float & Value)
{
	if ( PStrChr( pBuffer, '%' ) )
	{
		int i = 0;
		_FromString(pBuffer, i);
		Value = i * (1.0f / 100.0f);
	}
	else
	{
		Value = (float)atof(pBuffer);
	}
}

//-----------------------------------------------------------------------------

template<class T>
void _ToString(char * pBuffer, unsigned nBufferChars, const T & Value);
template<class T>
void _FromString(const char * pBuffer, T & Value);

template<class T>
void _Settings_Ex(SettingsExchange & se, T & v, const char * pName)
{
	SettingsExchange_Private * p = static_cast<SettingsExchange_Private *>(&se);
	if (p->bSaving)
	{
		char ValStr[MAX_PATH];
		_ToString(ValStr, _countof(ValStr), v);
		p->xml.AddChildElem(pName, ValStr);
	}
	else
	{
		if (p->xml.FindChildElem(pName))
		{
			_FromString(p->xml.GetChildData(), v);
		}
	}
}

//-----------------------------------------------------------------------------

};

//-----------------------------------------------------------------------------

BOOL SettingsExchange_Category(SettingsExchange & se, const char * pName)
{
	SettingsExchange_Private * p = static_cast<SettingsExchange_Private *>(&se);
	if (p->bSaving)
	{
		p->xml.AddChildElem(pName);
		p->xml.IntoElem();
		return TRUE;
	}
	else
	{
		if (p->xml.FindChildElem(pName))
		{
			p->xml.IntoElem();
			return TRUE;
		}
	}

	return FALSE;
}

void SettingsExchange_CategoryEnd(SettingsExchange & se)
{
	SettingsExchange_Private * p = static_cast<SettingsExchange_Private *>(&se);
	p->xml.OutOfElem();
}

PRESULT SettingsExchange_LoadFromFile(const OS_PATH_CHAR * pFileName, const char * pName, SETTINGSEXCHANGE_CALLBACK * pExchangeFunc, void * pContext)
{
	CMarkup xml;

	// Load it up first.
	if (!xml.Load(pFileName))
	{
		return S_FALSE;
	}

	// Find root element.
	if (!xml.FindElem(s_RootElementName))
	{
		goto corrupt;
	}

	// Does the section exist?
	if (!xml.FindChildElem(pName))
	{
		// CHB 2007.02.28 - In reality, this doesn't indicate corruption.
		// Perhaps a game update has introduced a new settings section that doesn't exist in the file?
		// Should the user be penalized with deletion of all their settings because of this?
		// Or a better example: The same file can be used to store different independent sections of settings.
		// This function only loads one section.  It may be valid from the caller's perspective that its
		// particular section does not exist (it could fill in defaults, etc.).  Deleting the whole file
		// just because a section is missing is overkill.
		//goto corrupt;
		return S_FALSE;
	}

	// Transfer data.
	xml.IntoElem();
	{
		SettingsExchange_Private se(xml, false);
		(*pExchangeFunc)(se, pContext);
	}
	xml.OutOfElem();

	// Success.
	return S_OK;

corrupt:
	// file is corrupt/invalid  -- delete it
	FileDelete( pFileName );
	return E_FAIL;
}

PRESULT SettingsExchange_SaveToFile(const OS_PATH_CHAR * pFileName, const char * pName, SETTINGSEXCHANGE_CALLBACK * pExchangeFunc, void * pContext)
{
	CMarkup xml;

	if (FileExists(pFileName))
	{
		// Load it up first.
		if (!xml.Load(pFileName))
		{
			return E_FAIL;
		}
	}

	// CMarkup::SetDoc() appears to have a bug/limitation of a
	// single root element, so make one here.
	if (!xml.FindElem(s_RootElementName))
	{
		xml.AddElem(s_RootElementName);
	}

	// If the section exists, remove it first.
	if (xml.FindChildElem(pName))
	{
		xml.RemoveChildElem();
	}

	// Add the element.
	xml.AddChildElem(pName);

	// Transfer data.
	xml.IntoElem();
	{
		SettingsExchange_Private se(xml, true);
		(*pExchangeFunc)(se, pContext);
	}
	xml.OutOfElem();

	// Create directory path.
	OS_PATH_CHAR DirName[MAX_PATH];
	PStrGetPath(DirName, _countof(DirName), pFileName);
	if (!FileDirectoryExists(DirName))
	{
		FileCreateDirectory(DirName);
	}

	// Save to disk.
	if (!xml.Save(pFileName))
	{
		return E_FAIL;
	}

	// Success.
	return S_OK;
}

//-----------------------------------------------------------------------------

void SettingsExchange_Do(SettingsExchange & se, int & v, const char * pName)
{
	_Settings_Ex(se, v, pName);
}

void SettingsExchange_Do(SettingsExchange & se, unsigned & v, const char * pName)
{
	//!!!
	C_ASSERT(sizeof(unsigned) == sizeof(int));
	int t = v;
	SettingsExchange_Do(se, t, pName);
	v = t;
}

void SettingsExchange_Do(SettingsExchange & se, float & v, const char * pName)
{
	_Settings_Ex(se, v, pName);
}

void SettingsExchange_Do(SettingsExchange & se, bool & v, const char * pName)
{
	int t = !!v;
	SettingsExchange_Do(se, t, pName);
	v = !!t;
}

void SettingsExchange_Do(SettingsExchange & se, unsigned long & v, const char * pName)
{
	C_ASSERT(sizeof(unsigned) == sizeof(unsigned long));
	unsigned t = v;
	SettingsExchange_Do(se, t, pName);
	v = t;
}

void SettingsExchange_Do(SettingsExchange & se, char * pStringBuffer, unsigned nStringBufferChars, const char * pName)
{
	ASSERT_RETURN(nStringBufferChars > 0);
	SettingsExchange_Private * p = static_cast<SettingsExchange_Private *>(&se);
	if (p->bSaving)
	{
		p->xml.AddChildElem(pName, pStringBuffer);
	}
	else
	{
		pStringBuffer[0] = '\0';
		if (p->xml.FindChildElem(pName))
		{
			PStrCopy(pStringBuffer, p->xml.GetChildData(), nStringBufferChars);
		}
	}
}
