//*****************************************************************************
//
// Central control for run-time selectable graphical features.
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#ifndef __E_FEATURES_H__
#define __E_FEATURES_H__

//-----------------------------------------------------------------------------
//
// High-level API
//
//-----------------------------------------------------------------------------

enum FEATURES_SLIDER
{
	FEATURES_SLIDER_GRAPHIC_QUALITY,	// individual graphics features
	FEATURES_SLIDER_COUNT				// not an actual slider
};

// Note, slider min & max can change between runs (depending on hardware).
// Therefore, when saving a slider value to non-volatile storage, it's
// probably best to represent it as a percentage of the complete range.
void e_FeaturesGetSliderMinMax(FEATURES_SLIDER nSlider, int & nMin, int & nMax);
int e_FeaturesGetSliderValue(FEATURES_SLIDER nSlider);
BOOL e_FeaturesSetSliderValue(FEATURES_SLIDER nSlider, float fRange, DWORD & dwActionFlags);
// Returns TRUE if value changed, FALSE if not
BOOL e_FeaturesSetSliderValue(FEATURES_SLIDER nSlider, int nValue, DWORD & dwActionFlags);

void e_FeaturesInit(void);
void e_FeaturesUpdate(void);

//-----------------------------------------------------------------------------
//
// Lower-level API
//
//-----------------------------------------------------------------------------

// Forward declarations.
struct D3D_MODEL;

//-----------------------------------------------------------------------------

// Low-level feature options available.
enum FEATURES_FLAG
{
	FEATURES_FLAG_SPHERICAL_HARMONICS,
	FEATURES_FLAG_POINT_LIGHTS,
	FEATURES_FLAG_SPECULAR,
};

//-----------------------------------------------------------------------------

// Encapsulates selectable features.
class Features
{
	public:
		void SetFlag(FEATURES_FLAG nFlag, bool bValue);
		bool GetFlag(FEATURES_FLAG nFlag) const;

		const Features operator&(const Features & that) const;
		const Features operator|(const Features & that) const;

		void SetNone(void);
		void SetAll(void);

		Features(bool bAll);
		Features() {};

	private:
		typedef unsigned char flag_type;
		Features(flag_type n);

		flag_type nFlags;
};

//-----------------------------------------------------------------------------

//
// Mid-level API
//

DWORD e_FeaturesSetGeneralFlag(FEATURES_FLAG nFlag, bool bEnable, bool bMainCharacterOnly);

inline DWORD e_FeaturesSetSphericalHarmonics(bool bEnable, bool bMainCharacterOnly)	{ return e_FeaturesSetGeneralFlag(FEATURES_FLAG_SPHERICAL_HARMONICS,	bEnable, bMainCharacterOnly); }
inline DWORD e_FeaturesSetPointLights(bool bEnable, bool bMainCharacterOnly)		{ return e_FeaturesSetGeneralFlag(FEATURES_FLAG_POINT_LIGHTS,			bEnable, bMainCharacterOnly); }
inline DWORD e_FeaturesSetSpecular(bool bEnable, bool bMainCharacterOnly)			{ return e_FeaturesSetGeneralFlag(FEATURES_FLAG_SPECULAR,				bEnable, bMainCharacterOnly); }

//
// Low-level API
//

void e_FeaturesSetModelRequestFlag(int nModelID, FEATURES_FLAG nFlag, bool bValue);
bool e_FeaturesGetModelRequestFlag(int nModelID, FEATURES_FLAG nFlag);
void e_FeaturesSetModelForceFlag(int nModelID, FEATURES_FLAG nFlag, bool bValue);
bool e_FeaturesGetModelForceFlag(int nModelID, FEATURES_FLAG nFlag);

// Features = ((ModelRequestFeatures & GlobalSelectedFeatures) | ModelForceFeatures) & GlobalAllowFeatures
void e_FeaturesGetEffective(int nModelID, class Features & tFeatures );

BOOL e_FeaturesGetGeneralFlag(FEATURES_FLAG eFlag);


//-----------------------------------------------------------------------------

#endif
