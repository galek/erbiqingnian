//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#include "e_pch.h"
#include "e_featureline.h"
#include "e_optionstate.h"
#include "dx9_shadowtypeconstants.h"
#include "e_shadow.h"

#pragma warning(disable:4481)	// warning C4481: nonstandard extension used: override specifier 'override'

namespace
{

//-----------------------------------------------------------------------------

struct StopData
{
	DWORD nName;
	int nResFactor;		// how many times to halve the resolution
};

//-----------------------------------------------------------------------------

#define _FCC(x) E_FEATURELINE_FOURCC(x)
const StopData s_Stops[] =
{
	{	_FCC("NONE"),	0 },
	{	_FCC("SDWL"),	1 },
	{	_FCC("SDWH"),	0 },
};
#undef _FCC

//-----------------------------------------------------------------------------

class FeatureLine_Shadow : public FeatureLine
{
	public:
		typedef FeatureLine_Shadow this_type;

		FeatureLine_Shadow(void);
		virtual ~FeatureLine_Shadow(void);

		virtual bool EnumerateStops(unsigned nIndex, DWORD & nNameOut) override;
		virtual PRESULT MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState) override;
		virtual PRESULT SelectStop(DWORD nStopName, OptionState * pState) override;

		bool EnumerateStops(unsigned nIndex, DWORD & nNameOut) const;
		PRESULT GetStopIndex(DWORD nStopName, unsigned & nIndexOut) const;
};

//-----------------------------------------------------------------------------

bool FeatureLine_Shadow::EnumerateStops(unsigned nIndex, DWORD & nNameOut) const
{
	if (nIndex < _countof(s_Stops))
	{
		nNameOut = s_Stops[nIndex].nName;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

bool FeatureLine_Shadow::EnumerateStops(unsigned nIndex, DWORD & nNameOut)
{
	return static_cast<const this_type *>(this)->EnumerateStops(nIndex, nNameOut);
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_Shadow::GetStopIndex(DWORD nStopName, unsigned & nIndexOut) const
{
	DWORD nName;
	for (unsigned n = 0; EnumerateStops(n, nName); ++n)
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

PRESULT FeatureLine_Shadow::MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState)
{
#define _FCC(x) E_FEATURELINE_FOURCC(x)

	unsigned int nIndex;

	struct
	{
		DWORD dwName;
		GLOBAL_STRING eGlobalStr;
		const char szName[32];
	} static const tMapping[] =
	{
		{ _FCC("SDWH"), GS_SETTINGS_HIGH,	"high"		},
		{ _FCC("SDWL"), GS_SETTINGS_MEDIUM,	"medium"	},
		{ _FCC("NONE"), GS_SETTINGS_LOW,	"low"		},
	};
	
	for ( unsigned int nChoice = 0; nChoice < _countof( tMapping ); nChoice++ )
	{
		V_DO_SUCCEEDED( GetStopIndex( tMapping[ nChoice ].dwName, nIndex ) )
		{
			// CHB 2007.06.29 - Only permit "high" and "low" if shadows available.
			if (nIndex > 0)
			{
				if (!(e_IsShadowTypeAvailable(pState, SHADOW_TYPE_DEPTH) || e_IsShadowTypeAvailable(pState, SHADOW_TYPE_COLOR)))
				{
					continue;
				}
			}

			V_RETURN( e_FeatureLine_AddChoice( pMap, nLineName, 
				tMapping[ nChoice ].dwName, tMapping[ nChoice ].szName, tMapping[ nChoice ].eGlobalStr ) );
		}
	}

	return S_OK;

#undef _FCC
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_Shadow::SelectStop(DWORD nStopName, OptionState * pState)
{
	// Get the index.
	unsigned nIndex;
	V_RETURN( GetStopIndex( nStopName, nIndex ) );
	pState->SuspendUpdate();

	if ( nIndex == 0 )
		pState->set_nShadowType( SHADOW_TYPE_NONE );
	else
	{
		if ( e_IsShadowTypeAvailable( pState, SHADOW_TYPE_DEPTH ) )
			pState->set_nShadowType( SHADOW_TYPE_DEPTH );
		else if ( e_IsShadowTypeAvailable( pState, SHADOW_TYPE_COLOR ) )
			pState->set_nShadowType( SHADOW_TYPE_COLOR );
		else
			pState->set_nShadowType( SHADOW_TYPE_NONE );
	}

	pState->set_nShadowReductionFactor(s_Stops[nIndex].nResFactor);
	pState->ResumeUpdate();

	return S_OK;
}

//-----------------------------------------------------------------------------

FeatureLine_Shadow::FeatureLine_Shadow(void)
{
}

//-----------------------------------------------------------------------------

FeatureLine_Shadow::~FeatureLine_Shadow(void)
{
}

//-----------------------------------------------------------------------------

};	// anonymous namespace

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_Shadow_Init(void)
{
	CComPtr<FeatureLine> pLine(new FeatureLine_Shadow);
	return e_FeatureLine_Register(pLine, E_FEATURELINE_FOURCC("SHDW"));
}
