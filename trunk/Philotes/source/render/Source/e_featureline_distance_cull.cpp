//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#include "e_pch.h"
#include "e_featureline.h"
#include "e_optionstate.h"
#include "e_lod.h"

#pragma warning(disable:4481)	// warning C4481: nonstandard extension used: override specifier 'override'

namespace
{

//-----------------------------------------------------------------------------

/*
	The LOD feature line is composed of pseudo-stops that mimic the
	LOD slider's range of -1 to +1, and they go like this:

	N100 N099 ... N002 N001 ZERO P001 P002 ... P099 P100

	For a total of 201 stops. This convention is chosen so that stop
	names, which are mentioned explicitly in the init DB, can remain
	consistent throughout changes to the LOD feature line system.
	For example, if stop P050 is mentioned in the init DB, we can be
	confident it will mean the same thing today and tomorrow, even
	if, say, the spacing between stops for the allotted number of
	choices changes.

	To aid in remembering the naming convention, Nxxx can be thought
	of as meaning "negative" or "near," bringing the LOD bias plane
	closer with numerically increasing values.  Pxxx is positive,
	pushing the LOD bias plane further away with numerically
	increasing values.
*/

class FeatureLine_Distance_Cull : public FeatureLine
{
	public:
		typedef FeatureLine_Distance_Cull this_type;

		FeatureLine_Distance_Cull(void);
		virtual ~FeatureLine_Distance_Cull(void);

		virtual bool EnumerateStops(unsigned nIndex, DWORD & nNameOut) override;
		virtual PRESULT MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState) override;
		virtual PRESULT SelectStop(DWORD nStopName, OptionState * pState) override;

		bool EnumerateStops(unsigned nIndex, DWORD & nNameOut) const;
		PRESULT GetStopIndex(DWORD nStopName, unsigned & nIndexOut) const;

	private:
		static const unsigned nMaxIndex = 100;		// the maximum index value
};

//-----------------------------------------------------------------------------

bool FeatureLine_Distance_Cull::EnumerateStops(unsigned nIndex, DWORD & nNameOut) const
{
	if (nIndex > nMaxIndex)
	{
		return false;
	}
	else if (nIndex > 0)
	{
		char buf[5];
		PStrPrintf(buf, _countof(buf), "P%03u", nIndex);
		nNameOut = E_FEATURELINE_FOURCC(buf);
	}
	else
	{
		nNameOut = E_FEATURELINE_FOURCC("ZERO");
	}
	return true;
}

//-----------------------------------------------------------------------------

bool FeatureLine_Distance_Cull::EnumerateStops(unsigned nIndex, DWORD & nNameOut)
{
	return static_cast<const this_type *>(this)->EnumerateStops(nIndex, nNameOut);
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_Distance_Cull::GetStopIndex(DWORD nStopName, unsigned & nIndexOut) const
{
	if (nStopName == E_FEATURELINE_FOURCC("ZERO"))
	{
		nIndexOut = 0;
		return S_OK;
	}
	char buf[5];
	FOURCCTOSTR(nStopName, buf);

	int nVal = 0;
	if (('0' <= buf[3]) && (buf[3] <= '9'))
	{
		nVal += buf[3] - '0';
	}
	else
	{
		return E_NOT_FOUND;
	}
	if (('0' <= buf[2]) && (buf[2] <= '9'))
	{
		nVal += 10 * (buf[2] - '0');
	}
	else
	{
		return E_NOT_FOUND;
	}
	if (('0' <= buf[1]) && (buf[1] <= '9'))
	{
		nVal += 100 * (buf[1] - '0');
	}
	else
	{
		return E_NOT_FOUND;
	}
	if ((nVal < 1) || (nVal > nMaxIndex))
	{
		return E_NOT_FOUND;
	}

	if (buf[0] == 'P')
	{
		//nVal = nVal;
	}
	else
	{
		return E_NOT_FOUND;
	}

	nIndexOut = nVal;
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_Distance_Cull::MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState)
{
	// The highest choice correlates to the maximum stop as
	// influenced by the init DB.  Each choice is nSpacing
	// stops away from its nearest neighbor.  This forms a
	// "sliding window" over the range of all LOD stops,
	// and positioning of said window can be controlled via
	// the init DB.  That way the range of available LOD
	// choices can be adjusted depending on the current
	// hardware's capabilities.


	struct 
	{
		GLOBAL_STRING eGlobalStr;
		const char szInternal[32];
	} static const ChoiceTable[] =
	{
		{ GS_SETTINGS_VERYHIGH,	"veryhigh" },
		{ GS_SETTINGS_HIGH,		"high" },
		{ GS_SETTINGS_MEDIUM,	"medium" },
		{ GS_SETTINGS_LOW,		"low" },
		//{ GS_SETTINGS_VERYLOW,	"verylow" },
	};


	DWORD nMinStopName, nMaxStopName;
	V_RETURN(pMap->GetMinimumAndMaximumStops(nLineName, nMinStopName, nMaxStopName));
	unsigned nMinStopIndex, nMaxStopIndex;
	V_RETURN(GetStopIndex(nMinStopName, nMinStopIndex));
	V_RETURN(GetStopIndex(nMaxStopName, nMaxStopIndex));


	// CML 2007.04.20 - This code centers the spacing-separated range of choices
	//unsigned nTotalRange = (_countof(ChoiceTable) - 1) * nSpacing;
	//unsigned nMidRange = ( nMinStopIndex + nMaxStopIndex ) / 2;
	//ASSERT( nMidRange >= nTotalRange / 2 );
	//unsigned nRangeMax = nMidRange + ( nTotalRange / 2 );
	//unsigned nRangeMin = nMidRange - ( nTotalRange / 2 );
	//ASSERT( nRangeMax <= nMaxStopIndex );
	//ASSERT( nRangeMin >= nMinStopIndex );


	// CML 2007.04.20 - This code evenly distributes choices between the min..max spectrum
	unsigned nStep = (nMaxStopIndex - nMinStopIndex) / (_countof(ChoiceTable) - 1);
	unsigned nRangeMax = nMaxStopIndex;
	unsigned nRangeMin = nMinStopIndex;


	bool bAdded = false;
	for (unsigned i = 0; i < _countof(ChoiceTable); ++i)
	{
		unsigned nValue = nRangeMax - i * nStep; // nSpacing;  CML 2007.04.20
		if ((nMinStopIndex <= nValue) && (nValue <= nMaxStopIndex))
		{
			DWORD nStopName = 0;
			EnumerateStops(nValue, nStopName);
			V_RETURN(e_FeatureLine_AddChoice(pMap, nLineName, nStopName, ChoiceTable[i].szInternal, ChoiceTable[i].eGlobalStr ));
			bAdded = true;
		}
	}
	ASSERT(bAdded);	// at least one should have been added

	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_Distance_Cull::SelectStop(DWORD nStopName, OptionState * pState)
{
	// Get the index.
	unsigned nIndex;
	V_RETURN(GetStopIndex(nStopName, nIndex));
	pState->SuspendUpdate();
	float fValue = nIndex * (1.0f / 100.0f);
	pState->set_fCullSizeScale(fValue);
	pState->ResumeUpdate();

	return S_OK;
}

//-----------------------------------------------------------------------------

FeatureLine_Distance_Cull::FeatureLine_Distance_Cull(void)
{
}

//-----------------------------------------------------------------------------

FeatureLine_Distance_Cull::~FeatureLine_Distance_Cull(void)
{
}

//-----------------------------------------------------------------------------

};	// anonymous namespace

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_Distance_Cull_Init(void)
{
	CComPtr<FeatureLine> pLine(new FeatureLine_Distance_Cull);
	return e_FeatureLine_Register(pLine, E_FEATURELINE_FOURCC("DIST"));
}
