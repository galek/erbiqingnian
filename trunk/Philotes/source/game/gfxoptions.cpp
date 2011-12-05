//-----------------------------------------------------------------------------
//
// End-user configurable and persistent graphics and performance settings.
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

#if !ISVERSION(SERVER_VERSION)

#include "gfxoptions.h"
#include "settings.h"
#include "e_optionstate.h"	// CHB 2007.02.28
#include "e_featureline.h"	// CHB 2007.03.06
#include "e_lod.h"			// CHB 2007.03.16
#include "appcommon.h"
#include "pstring.h"		// CHB 2007.05.14
#include "e_main.h"			// CHB 2007.05.14

//-----------------------------------------------------------------------------

namespace
{

enum GFXOPTIONS_NAME
{
	GFXOPTIONS_NAME_STATE,
	GFXOPTIONS_NAME_STOPS,
};

const unsigned GFXOPTIONS_DEFAULT_SECTION_NAME_BUFFER_SIZE = 20;

bool GfxOptions_GetSectionName(GFXOPTIONS_NAME nWhich, char * pBuf, int nBufLen)
{
	ASSERT_RETFALSE(nBufLen > 0);
	pBuf[0] = '\0';
	const char * pFmt = 0;
	switch (nWhich)
	{
		case GFXOPTIONS_NAME_STATE:
			pFmt = "GraphicsState";
			break;
		case GFXOPTIONS_NAME_STOPS:
			pFmt = "GraphicsFeatures";
			break;
		default:
			ASSERT(false);
			return false;
	}
	PStrCopy(pBuf, pFmt, nBufLen);
	if (e_DX10IsEnabled())
	{
		PStrCat(pBuf, "10", nBufLen);
	}
	return true;
}

};	// anonymous namespace

//-----------------------------------------------------------------------------

void GfxOptions_Load(void)
{
	CComPtr<OptionState> pState;
	V_DO_FAILED(e_OptionState_CloneActive(&pState))
	{
		return;
	}
	ASSERT_RETURN(pState != 0);

	CComPtr<FeatureLineMap> pMap;
	V_DO_FAILED(e_FeatureLine_OpenMap(pState, &pMap))
	{
		return;
	}
	ASSERT_RETURN(pMap != 0);

	OS_PATH_CHAR FileName[MAX_PATH];
	Settings_GetDefaultSettingsFileName(FileName, _countof(FileName));

	// Load state settings.
	char SectionName[GFXOPTIONS_DEFAULT_SECTION_NAME_BUFFER_SIZE];
	GfxOptions_GetSectionName(GFXOPTIONS_NAME_STATE, SectionName, _countof(SectionName));
	V_DO_FAILED(e_OptionState_LoadFromFile(FileName, SectionName, pState))
	{
		return;
	}

	// Load stops settings.
	GfxOptions_GetSectionName(GFXOPTIONS_NAME_STOPS, SectionName, _countof(SectionName));
	V_DO_FAILED(pMap->LoadFromFile(FileName, SectionName))
	{
		return;
	}

	// Override the loaded windowed mode based on command-line flags.
	if ( AppCommonGetForceWindowed() )
	{
		pState->set_bWindowed(true);
	}

	// Make active.
	V(e_FeatureLine_CommitToActive(pMap));
	V(e_OptionState_CommitToActive(pState));
}

void GfxOptions_Save(const OptionState * pState, const FeatureLineMap * pMap)
{
	OS_PATH_CHAR FileName[MAX_PATH];
	Settings_GetDefaultSettingsFileName(FileName, _countof(FileName));
	char SectionName[GFXOPTIONS_DEFAULT_SECTION_NAME_BUFFER_SIZE];
	GfxOptions_GetSectionName(GFXOPTIONS_NAME_STATE, SectionName, _countof(SectionName));
	V(e_OptionState_SaveToFile(FileName, SectionName, pState));
	GfxOptions_GetSectionName(GFXOPTIONS_NAME_STOPS, SectionName, _countof(SectionName));
	V(pMap->SaveToFile(FileName, SectionName));
}

void GfxOptions_Save(void)
{
	CComPtr<OptionState> pState;
	V_DO_FAILED(e_OptionState_CloneActive(&pState))
	{
		return;
	}
	ASSERT_RETURN(pState != 0);

	CComPtr<FeatureLineMap> pMap;
	V_DO_FAILED(e_FeatureLine_OpenMap(pState, &pMap))
	{
		return;
	}
	ASSERT_RETURN(pMap != 0);

	GfxOptions_Save(pState, pMap);
}

static CComPtr<FeatureLineMap> s_pDefaultMap;

void GfxOptions_SaveDefaults(void)
{
	CComPtr<OptionState> pState;
	V_DO_FAILED(e_OptionState_CloneActive(&pState))
	{
		return;
	}
	ASSERT_RETURN(pState != 0);

	CComPtr<FeatureLineMap> pMap;
	V_DO_FAILED(e_FeatureLine_OpenMap(pState, &pMap))
	{
		return;
	}
	ASSERT_RETURN(pMap != 0);

	s_pDefaultMap = pMap;
}

void GfxOptions_GetDefaults(OptionState * * ppStateOut, FeatureLineMap * * ppMapOut)
{
	*ppStateOut = 0;
	*ppMapOut = 0;

	ASSERT_RETURN(s_pDefaultMap != 0);

	CComPtr<FeatureLineMap> pMap;
	V_DO_FAILED(s_pDefaultMap->Clone(&pMap))
	{
		return;
	}
	ASSERT_RETURN(pMap != 0);

	CComPtr<OptionState> pState;
	V_DO_FAILED(pMap->GetState(&pState))
	{
		return;
	}
	ASSERT_RETURN(pState != 0);

	*ppMapOut = pMap;
	pMap.p->AddRef();
	*ppStateOut = pState;
	pState.p->AddRef();
}

void GfxOptions_Deinit(void)
{
	// CHB 2007.10.22 - Standard C++ Library-like containers are
	// based on the game's memory manager and need to be freed
	// before the memory manager is shut down.
	s_pDefaultMap = 0;
}

//-----------------------------------------------------------------------------

#endif
