//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef __E_SCREEN__
#define __E_SCREEN__

#include "safe_vector.h"

class OptionState;

struct E_SCREEN_DISPLAY_MODE_REFRESH
{
	unsigned Numerator;
	unsigned Denominator;

	bool operator==(const E_SCREEN_DISPLAY_MODE_REFRESH & rhs) const;
	bool operator!=(const E_SCREEN_DISPLAY_MODE_REFRESH & rhs) const;
	bool operator<(const E_SCREEN_DISPLAY_MODE_REFRESH & rhs) const;
	bool operator>(const E_SCREEN_DISPLAY_MODE_REFRESH & rhs) const;

	float AsFloat(void) const;
	int AsInt(void) const;
};

struct E_SCREEN_DISPLAY_MODE
{
	// Note, this deliberately mirrors DXGI_MODE_DESC
	int Width;
	int Height;
	E_SCREEN_DISPLAY_MODE_REFRESH RefreshRate;
	unsigned Format;
	unsigned ScanlineOrdering;
	unsigned Scaling;

	E_SCREEN_DISPLAY_MODE(void);

	void SetWindowed(void);
	void DefaultFullscreen(void);
	bool IsWindowed(void) const;
};

bool operator<(const E_SCREEN_DISPLAY_MODE & lhs, const E_SCREEN_DISPLAY_MODE & rhs);
bool e_Screen_AreDisplayModesEqualAsideFromRefreshRate(const E_SCREEN_DISPLAY_MODE & lhs, const E_SCREEN_DISPLAY_MODE & rhs);

typedef safe_vector<E_SCREEN_DISPLAY_MODE> E_SCREEN_DISPLAY_MODE_VECTOR;
typedef safe_vector<E_SCREEN_DISPLAY_MODE_VECTOR> E_SCREEN_DISPLAY_MODE_GROUP;

const E_SCREEN_DISPLAY_MODE e_Screen_GetCurrentDisplayMode(void);
//bool e_Screen_SetCurrentDisplayMode(const E_SCREEN_DISPLAY_MODE & Mode);
const E_SCREEN_DISPLAY_MODE_VECTOR e_Screen_EnumerateDisplayModes(void);
const E_SCREEN_DISPLAY_MODE_GROUP e_Screen_EnumerateDisplayModesGroupedByRefreshRate(void);

void e_Screen_DisplayModeFromState(const OptionState * pState, E_SCREEN_DISPLAY_MODE & Mode);
void e_Screen_DisplayModeToState(OptionState * pState, const E_SCREEN_DISPLAY_MODE & Mode);
void e_Screen_DisplayModeToStateNoSuspend(OptionState * pState, const E_SCREEN_DISPLAY_MODE & Mode);

bool e_Screen_IsModeValidForWindowedDisplay(const E_SCREEN_DISPLAY_MODE & Mode);

#endif

