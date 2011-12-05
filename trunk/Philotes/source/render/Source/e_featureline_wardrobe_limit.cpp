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

	class FeatureLine_Wardrobe_Limit : public FeatureLine
	{
	public:
		typedef FeatureLine_Wardrobe_Limit this_type;

		FeatureLine_Wardrobe_Limit(void);
		virtual ~FeatureLine_Wardrobe_Limit(void);

		virtual bool EnumerateStops(unsigned nIndex, DWORD & nNameOut) override;
		virtual PRESULT MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState) override;
		virtual PRESULT SelectStop(DWORD nStopName, OptionState * pState) override;

		static bool _EnumerateStops(unsigned nIndex, DWORD & nNameOut);
		PRESULT GetStopIndex(DWORD nStopName, unsigned & nIndexOut) const;
	};

	struct StopChoice
	{
		DWORD nStopName;
		const char szInternal[9];
		GLOBAL_STRING eGlobalStr;
		int nWardrobeLimit;
	};
	const StopChoice ChoiceTable[] =
	{
		//{ GS_SETTINGS_VERYLOW,	"verylow" },
		//	Stop Name						Name		Display Name			Wardrobe Limit
		//	-------------------------------	-----------	-------------------		--------------
		{	E_FEATURELINE_FOURCC("WLOW"),	"low",		GS_SETTINGS_LOW,		8			},
		{	E_FEATURELINE_FOURCC("WMED"),	"medium",	GS_SETTINGS_MEDIUM,		16			},
		{	E_FEATURELINE_FOURCC("WHGH"),	"high",		GS_SETTINGS_HIGH,		32			},
		{	E_FEATURELINE_FOURCC("WVHI"),	"veryhigh", GS_SETTINGS_VERYHIGH,	64			},
		{	E_FEATURELINE_FOURCC("NONE"),	"off",		GS_SETTINGS_OFF,		0			},
	};

	//-----------------------------------------------------------------------------

	bool FeatureLine_Wardrobe_Limit::_EnumerateStops(unsigned nIndex, DWORD & nNameOut)
	{
		if (nIndex >= _countof(ChoiceTable))
		{
			return false;
		}
		nNameOut = ChoiceTable[nIndex].nStopName;
		return true;
	}

	//-----------------------------------------------------------------------------

	bool FeatureLine_Wardrobe_Limit::EnumerateStops(unsigned nIndex, DWORD & nNameOut)
	{
		return _EnumerateStops(nIndex, nNameOut);
	}

	//-----------------------------------------------------------------------------

	PRESULT FeatureLine_Wardrobe_Limit::GetStopIndex(DWORD nStopName, unsigned & nIndexOut) const
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

	PRESULT FeatureLine_Wardrobe_Limit::MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState)
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

	PRESULT FeatureLine_Wardrobe_Limit::SelectStop(DWORD nStopName, OptionState * pState)
	{
		// Get the index.
		unsigned nIndex;
		V_RETURN(GetStopIndex(nStopName, nIndex));
		pState->SuspendUpdate();
		pState->set_nWardrobeLimit(ChoiceTable[nIndex].nWardrobeLimit);
		pState->ResumeUpdate();
		return S_OK;
	}

	//-----------------------------------------------------------------------------

	FeatureLine_Wardrobe_Limit::FeatureLine_Wardrobe_Limit(void)
	{
	}

	//-----------------------------------------------------------------------------

	FeatureLine_Wardrobe_Limit::~FeatureLine_Wardrobe_Limit(void)
	{
	}

	//-----------------------------------------------------------------------------

};	// anonymous namespace

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_Wardrobe_Limit_Init(void)
{
	CComPtr<FeatureLine> pLine(new FeatureLine_Wardrobe_Limit);
	return e_FeatureLine_Register(pLine, E_FEATURELINE_FOURCC("WLIM"));
}
