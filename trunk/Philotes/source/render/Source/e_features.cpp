//*****************************************************************************
//
// Central control for run-time selectable graphical features.
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#include "e_pch.h"
#include "e_features.h"
#include "e_model.h"
#include "e_budget.h"
#include "e_shadow.h"
#include "e_main.h"
#include "e_settings.h"
#include "e_optionstate.h"	// CHB 2007.03.14

//-----------------------------------------------------------------------------

static
struct
{
} sFeaturesState;

//-----------------------------------------------------------------------------

struct Slider
{
	typedef char value_type;
	value_type nValue;
	value_type nMin;
	value_type nMax;
};

static Slider sSliders[FEATURES_SLIDER_COUNT];

void e_FeaturesGetSliderMinMax(FEATURES_SLIDER nSlider, int & nMin, int & nMax)
{
	ASSERT((0 <= nSlider) && (nSlider < FEATURES_SLIDER_COUNT));
	nMin = sSliders[nSlider].nMin;
	nMax = sSliders[nSlider].nMax;
}

int e_FeaturesGetSliderValue(FEATURES_SLIDER nSlider)
{
	ASSERT((0 <= nSlider) && (nSlider < FEATURES_SLIDER_COUNT));
	return sSliders[nSlider].nValue;
}

static
DWORD e_FeaturesSetGraphicQuality(int nValue)
{
	static const int nTable[] = { 0x000, 0x001, 0x011, 0x111, 0x112, 0x122, 0x222 };
	const int nEntries = sizeof(nTable) / sizeof(nTable[0]);
	ASSERT((0 <= nValue) && (nValue <= nEntries));

	DWORD dwActionFlags = 0;

	int nMask = nTable[nValue];

	int t = nMask & 0x0f;
	nMask >>= 4;
	dwActionFlags |= e_FeaturesSetPointLights(t > 0, t == 1);

	t = nMask & 0x0f;
	nMask >>= 4;
	dwActionFlags |= e_FeaturesSetSpecular(t > 0, t == 1);

	t = nMask & 0x0f;
	nMask >>= 4;
#if !ISVERSION( SERVER_VERSION )
	dwActionFlags |= e_FeaturesSetSphericalHarmonics(t > 0, t == 1);
#endif
	return dwActionFlags;
}

BOOL e_FeaturesSetSliderValue(FEATURES_SLIDER nSlider, float fRange, DWORD & dwActionFlags)
{
#if !ISVERSION( SERVER_VERSION )
	int nValue = (int)( fRange * ( sSliders[nSlider].nMax - sSliders[nSlider].nMin ) );
	return e_FeaturesSetSliderValue( nSlider, nValue, dwActionFlags );
#else
	return 0;
#endif
}

// Returns TRUE if value changed, FALSE if not
BOOL e_FeaturesSetSliderValue(FEATURES_SLIDER nSlider, int nValue, DWORD & dwActionFlags)
{
#if !ISVERSION( SERVER_VERSION )
	int nMin, nMax;
	e_FeaturesGetSliderMinMax(nSlider, nMin, nMax);
	ASSERT((nMin <= nValue) && (nValue <= nMax));

	Slider::value_type tValue = static_cast<Slider::value_type>(nValue);
	BOOL bChanged = !!( sSliders[nSlider].nValue != tValue );

	sSliders[nSlider].nValue = tValue;

	switch (nSlider)
	{
		case FEATURES_SLIDER_GRAPHIC_QUALITY:
			dwActionFlags |= e_FeaturesSetGraphicQuality(nValue);
			break;
		default:
			break;
	}

	return bChanged;
#else
	return FALSE;
#endif
}

void e_FeaturesInit(void)
{
#if !ISVERSION( SERVER_VERSION )
	sSliders[FEATURES_SLIDER_GRAPHIC_QUALITY].nMin = 0;
	sSliders[FEATURES_SLIDER_GRAPHIC_QUALITY].nMax = 6;

	DWORD dwFlags = 0;

	// Set all sliders to maximum.
	for (int i = 0; i < FEATURES_SLIDER_COUNT; ++i) {
		e_FeaturesSetSliderValue(static_cast<FEATURES_SLIDER>(i), sSliders[i].nMax, dwFlags);
	}
#endif
}

//-----------------------------------------------------------------------------

Features e_gFeaturesGlobalSelected(true);
Features e_gFeaturesGlobalAllowed(true);

//-----------------------------------------------------------------------------

void Features::SetFlag(FEATURES_FLAG nFlag, bool bValue)
{
	nFlags = bValue ? (nFlags | (1 << nFlag)) : (nFlags & (~(1 << nFlag)));
}

bool Features::GetFlag(FEATURES_FLAG nFlag) const
{
	return (nFlags & (1 << nFlag)) != 0;
}

const Features Features::operator&(const Features & that) const
{
	return Features(static_cast<flag_type>(nFlags & that.nFlags));
}

const Features Features::operator|(const Features & that) const
{
	return Features(static_cast<flag_type>(nFlags | that.nFlags));
}

void Features::SetNone(void)
{
	nFlags = 0;
}

void Features::SetAll(void)
{
	nFlags = static_cast<flag_type>(-1);
}

Features::Features(flag_type n)
	: nFlags(n)
{
}

Features::Features(bool bAll)
{
	bAll ? SetAll() : SetNone();
}

//-----------------------------------------------------------------------------

void e_FeaturesSetModelRequestFlag(int nModelID, FEATURES_FLAG nFlag, bool bValue)
{
#if !ISVERSION( SERVER_VERSION )
	V( e_ModelSetFeatureRequestFlag( nModelID, nFlag, bValue ) );
#endif
}

bool e_FeaturesGetModelRequestFlag(int nModelID, FEATURES_FLAG nFlag)
{
#if !ISVERSION( SERVER_VERSION )
	return !!e_ModelGetFeatureRequestFlag( nModelID, nFlag );
#else
	return false;
#endif
}

void e_FeaturesSetModelForceFlag(int nModelID, FEATURES_FLAG nFlag, bool bValue)
{
#if !ISVERSION( SERVER_VERSION )
	V( e_ModelSetFeatureForceFlag( nModelID, nFlag, bValue ) );
#endif	
}

bool e_FeaturesGetModelForceFlag(int nModelID, FEATURES_FLAG nFlag)
{
#if !ISVERSION( SERVER_VERSION )
	return !!e_ModelGetFeatureForceFlag( nModelID, nFlag );
#else
	return false;
#endif
}

// Features = ((ModelRequestFeatures & GlobalSelectedFeatures) | ModelForceFeatures) & GlobalAllowFeatures
void e_FeaturesGetEffective(int nModelID, class Features & tFeatures )
{
#if !ISVERSION( SERVER_VERSION )
	V( e_ModelFeaturesGetEffective( nModelID, tFeatures ) );
#endif
}

//-----------------------------------------------------------------------------

#include "prime.h"
#include "clients.h"
#include "units.h"

DWORD e_FeaturesSetGeneralFlag(FEATURES_FLAG nFlag, bool bEnable, bool bMainCharacterOnly)
{
#if !ISVERSION( SERVER_VERSION )
	// Find main character model.
	int nModelId = INVALID_ID;
	GAME * pGame = AppGetCltGame();
	if (pGame != 0) {
		UNIT * pUnit = GameGetControlUnit(pGame);
		if (pUnit != 0) {
			nModelId = c_UnitGetModelIdThirdPerson(pUnit);	// Hmmm, what about other models?
		}
	}

	BOOL bGlobalChanged = !!( bEnable == e_gFeaturesGlobalAllowed.GetFlag(nFlag) );
	//
	e_gFeaturesGlobalAllowed.SetFlag(nFlag, bEnable);
	if (bEnable) {
		e_gFeaturesGlobalSelected.SetFlag(nFlag, !bMainCharacterOnly);
		if (nModelId != INVALID_ID) {
			e_FeaturesSetModelForceFlag(nModelId, nFlag, bMainCharacterOnly);
		}
	}

	DWORD dwActionFlags = 0;

	if ( bGlobalChanged )
	{
		switch (nFlag)
		{
		case FEATURES_FLAG_POINT_LIGHTS:
			{
				//
			}
			break;
		case FEATURES_FLAG_SPECULAR:
			{
				CComPtr<OptionState> pState;
				V_DO_SUCCEEDED(e_OptionState_CloneActive(&pState))
				{
					pState->set_bSpecular(bEnable);
					V(e_OptionState_CommitToActive(pState));
				}
			}
			break;
		case FEATURES_FLAG_SPHERICAL_HARMONICS:
			{
				//
			}
			break;
		}
	}

	return dwActionFlags;
#else
	return 0;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_FeaturesGetGeneralFlag(FEATURES_FLAG eFlag)
{
	return e_gFeaturesGlobalAllowed.GetFlag(eFlag);
}
