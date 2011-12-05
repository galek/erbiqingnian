//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#include "e_pch.h"
#include "e_featureline.h"
#include "e_optionstate.h"
#include "e_lod.h"
#include "lod.h"

#pragma warning(disable:4481)	// warning C4481: nonstandard extension used: override specifier 'override'

namespace
{

//-----------------------------------------------------------------------------

class FeatureLine_Model_LOD : public FeatureLine
{
	public:
		typedef FeatureLine_Model_LOD this_type;

		FeatureLine_Model_LOD(void);
		virtual ~FeatureLine_Model_LOD(void);

		virtual bool EnumerateStops(unsigned nIndex, DWORD & nNameOut) override;
		virtual PRESULT MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState) override;
		virtual PRESULT SelectStop(DWORD nStopName, OptionState * pState) override;

		static bool _EnumerateStops(unsigned nIndex, DWORD & nNameOut);
		PRESULT GetStopIndex(DWORD nStopName, unsigned & nIndexOut) const;
};

struct StopChoice
{
	DWORD nStopName;
	const char szInternal[8];
	GLOBAL_STRING eGlobalStr;
	int nBackgroundForceLOD;	// for both display and loading
	int nAnimatedForceLOD;		// for both display and loading
	float fLODBias;
};
const StopChoice ChoiceTable[] =
{
	//{ GS_SETTINGS_VERYLOW,	"verylow" },
	//	Stop Name						Name		Display Name		Force BG	Force Anim	Bias
	//	-------------------------------	-----------	-------------------	-----------	-----------	-------
	{	E_FEATURELINE_FOURCC("LOWW"),	"low",		GS_SETTINGS_LOW,	LOD_LOW,	LOD_LOW,	1.0f	},
	{	E_FEATURELINE_FOURCC("MEDI"),	"medium",	GS_SETTINGS_MEDIUM,	LOD_LOW,	LOD_NONE,	0.6f	},
	{	E_FEATURELINE_FOURCC("HIGH"),	"high",		GS_SETTINGS_HIGH,	LOD_HIGH,	LOD_NONE,	1.0f	},
	//{ GS_SETTINGS_VERYHIGH,	"veryhigh" },
};

//-----------------------------------------------------------------------------

bool FeatureLine_Model_LOD::_EnumerateStops(unsigned nIndex, DWORD & nNameOut)
{
	if (nIndex >= _countof(ChoiceTable))
	{
		return false;
	}
	nNameOut = ChoiceTable[nIndex].nStopName;
	return true;
}

//-----------------------------------------------------------------------------

bool FeatureLine_Model_LOD::EnumerateStops(unsigned nIndex, DWORD & nNameOut)
{
	return _EnumerateStops(nIndex, nNameOut);
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_Model_LOD::GetStopIndex(DWORD nStopName, unsigned & nIndexOut) const
{
	DWORD nName;
	for (unsigned n = 0; _EnumerateStops(n, nName); ++n)
	{
		if (nName == nStopName)
		{
			nIndexOut = n;
			return S_OK;
		}
	}
	return E_NOT_FOUND;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_Model_LOD::MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState)
{
	// No need to check against the minimum and maximum stops
	// manually...  This is done for us in e_FeatureLine_AddChoice().
	for (int i = _countof(ChoiceTable) - 1; i >= 0; --i)
	{
		V_RETURN(e_FeatureLine_AddChoice(pMap, nLineName, ChoiceTable[i].nStopName, ChoiceTable[i].szInternal, ChoiceTable[i].eGlobalStr ));
	}
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_Model_LOD::SelectStop(DWORD nStopName, OptionState * pState)
{
	// Get the index.
	unsigned nIndex;
	V_RETURN(GetStopIndex(nStopName, nIndex));
	pState->SuspendUpdate();
	pState->set_nBackgroundShowForceLOD(ChoiceTable[nIndex].nBackgroundForceLOD);
	pState->set_nBackgroundLoadForceLOD(ChoiceTable[nIndex].nBackgroundForceLOD);
	pState->set_nAnimatedShowForceLOD(ChoiceTable[nIndex].nAnimatedForceLOD);
	pState->set_nAnimatedLoadForceLOD(ChoiceTable[nIndex].nAnimatedForceLOD);
	pState->set_fLODScale(ChoiceTable[nIndex].fLODBias);
	pState->set_fLODBias(0.0f);
	pState->ResumeUpdate();
	return S_OK;
}

//-----------------------------------------------------------------------------

FeatureLine_Model_LOD::FeatureLine_Model_LOD(void)
{
}

//-----------------------------------------------------------------------------

FeatureLine_Model_LOD::~FeatureLine_Model_LOD(void)
{
}

//-----------------------------------------------------------------------------

};	// anonymous namespace

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_Model_LOD_Init(void)
{
	CComPtr<FeatureLine> pLine(new FeatureLine_Model_LOD);
	return e_FeatureLine_Register(pLine, E_FEATURELINE_FOURCC("MLOD"));
}
