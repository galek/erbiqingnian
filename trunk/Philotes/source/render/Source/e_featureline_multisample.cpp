//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#include "e_pch.h"
#include "e_featureline.h"
#include "e_optionstate.h"
#include "dxC_settings.h"	// for e_IsMultiSampleAvailable()

#pragma warning(disable:4481)	// warning C4481: nonstandard extension used: override specifier 'override'

namespace
{

//-----------------------------------------------------------------------------

struct AntialiasingQuality
{
	/*
	enum TYPE
	{
		MSAA = 0,		// standard multisample antialiasing
		CSAA,			// uses the Coverage Sampling features of certain cards
		NUM_TYPES
	};

	*/

	int nType;
	DWORD dwQuality;
};

struct StopData
{
	DWORD nName;
	GLOBAL_STRING eGlobalStr;
	char szName[32];
	int nQualityIndex;
};

//-----------------------------------------------------------------------------
#define CSAA_MODE_START 3

static AntialiasingQuality sgtAntialiasingQuality[] = //Please make sure to change CSAA_MODE_START above if you change these.  CSAA_MODE_START indicates the first slot that is a CSAA mode
{//		  MSAA		  CSAA
	{ D3DMULTISAMPLE_NONE,		0   },					// off
	{ D3DMULTISAMPLE_2_SAMPLES, 0   },					// 2x MSAA
	{ D3DMULTISAMPLE_4_SAMPLES, 0   },					// 4x MSAA
	{ D3DMULTISAMPLE_4_SAMPLES, DXC_9_10( 4, 16 ) },	// 16x CSAA
	{ D3DMULTISAMPLE_8_SAMPLES, DXC_9_10( 2, 16 ) },	// 16xQ CSAA
/*	Can't add more than 5 modes right now
	{ D3DMULTISAMPLE_8_SAMPLES, DXC_9_10( 0, 8  ) },	// 8 x Q CSAA
	{ D3DMULTISAMPLE_8_SAMPLES, DXC_9_10( 2, 16 ) },	// 16 x Q CSAA
*/
};

#define _FCC(x) E_FEATURELINE_FOURCC(x)
const StopData s_Stops[] = //Please make sure to change CSAA_MODE_START above if you change these.  CSAA_MODE_START indicates the first slot that is a CSAA mode
{
	{	_FCC("NONE"),		GS_SETTINGS_OFF,		"off",			0	},
	{	_FCC("AA01"),		GS_SETTINGS_LOW,		"2x MSAA",		1 	},
	{	_FCC("AA02"),		GS_SETTINGS_MEDIUM,		"4x MSAA",		2	},
	{	_FCC("AA03"),		GS_SETTINGS_HIGH,		"16x CSAA",		3	},
	{	_FCC("AA04"),		GS_SETTINGS_VERYHIGH,	"16xQ CSAA",	4	},
/*	Can't add more than 5 modes right now
	{	_FCC("AA05"),		GS_SETTINGS_VERYHIGH,	"8xQ CSAA",		5	},
	{	_FCC("AA06"),		GS_SETTINGS_VERYHIGH,	"16xQ CSAA",	6   },
*/
};
#undef _FCC

//-----------------------------------------------------------------------------

class FeatureLine_MultiSample : public FeatureLine
{
	public:
		typedef FeatureLine_MultiSample this_type;

		FeatureLine_MultiSample(void);
		virtual ~FeatureLine_MultiSample(void);

		virtual bool EnumerateStops(unsigned nIndex, DWORD & nNameOut) override;
		virtual PRESULT MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState) override;
		virtual PRESULT SelectStop(DWORD nStopName, OptionState * pState) override;

		bool EnumerateStops(unsigned nIndex, DWORD & nNameOut) const;
		PRESULT GetStopIndex(DWORD nStopName, unsigned & nIndexOut) const;
};

//-----------------------------------------------------------------------------

bool FeatureLine_MultiSample::EnumerateStops(unsigned nIndex, DWORD & nNameOut) const
{
	if (nIndex < _countof(s_Stops))
	{
		nNameOut = s_Stops[nIndex].nName;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

bool FeatureLine_MultiSample::EnumerateStops(unsigned nIndex, DWORD & nNameOut)
{
	return static_cast<const this_type *>(this)->EnumerateStops(nIndex, nNameOut);
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_MultiSample::GetStopIndex(DWORD nStopName, unsigned & nIndexOut) const
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

PRESULT FeatureLine_MultiSample::MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState)
{
#define _FCC(x) E_FEATURELINE_FOURCC(x)

	BOOL bCSAA = e_IsCSAAAvailable();

	// These must be added in the reverse order they are declared in the stops list (above).
	static const DWORD tMapping[] =
	{
/*	Can't add more than 5 modes right now
		_FCC("AA06"),
		_FCC("AA05"),
*/
		_FCC("AA04"),
		_FCC("AA03"),
		_FCC("AA02"),
		_FCC("AA01"),
		_FCC("NONE"),
	};

	for ( unsigned int nChoice = 0; nChoice < _countof( tMapping ); nChoice++ )
	{
		unsigned int nIndex;
		V_DO_SUCCEEDED( GetStopIndex( tMapping[ nChoice ], nIndex ) )
		{
			int index = s_Stops[nIndex].nQualityIndex;

			if( !bCSAA && index >= CSAA_MODE_START )
				continue;
			
			// CHB 2007.09.14 - The line should always have at least
			// one stop. Here, make sure "NONE" is always available.
			if (sgtAntialiasingQuality[index].nType != D3DMULTISAMPLE_NONE)
			{
				if ( ! e_IsMultiSampleAvailable( pState,
												 static_cast<DXC_MULTISAMPLE_TYPE>(sgtAntialiasingQuality[index].nType),
												 static_cast<DXC_MULTISAMPLE_TYPE>(sgtAntialiasingQuality[index].dwQuality) ) )
					continue;
			}

			V_RETURN( e_FeatureLine_AddChoice( pMap, nLineName, 
				s_Stops[ nIndex ].nName, s_Stops[ nIndex ].szName, s_Stops[ nIndex ].eGlobalStr ) );
		}
	}

	return S_OK;

#undef _FCC
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_MultiSample::SelectStop(DWORD nStopName, OptionState * pState)
{
	//BOOL bCSAA = e_IsCSAAAvailable();

	// Get the index.
	unsigned nIndex;
	V_RETURN(GetStopIndex(nStopName, nIndex));
	pState->SuspendUpdate();
	pState->set_nMultiSampleType(static_cast<DXC_MULTISAMPLE_TYPE>(sgtAntialiasingQuality[s_Stops[nIndex].nQualityIndex].nType));
	pState->set_nMultiSampleQuality(static_cast<DXC_MULTISAMPLE_TYPE>(sgtAntialiasingQuality[s_Stops[nIndex].nQualityIndex].dwQuality));
	pState->ResumeUpdate();
	return S_OK;
}

//-----------------------------------------------------------------------------

FeatureLine_MultiSample::FeatureLine_MultiSample(void)
{
}

//-----------------------------------------------------------------------------

FeatureLine_MultiSample::~FeatureLine_MultiSample(void)
{
}

//-----------------------------------------------------------------------------

};

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_MultiSample_Init(void)
{
	CComPtr<FeatureLine> pLine(new FeatureLine_MultiSample);
	return e_FeatureLine_Register(pLine, E_FEATURELINE_FOURCC("MSAA"));
}
