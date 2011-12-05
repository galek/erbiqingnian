//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_screen.h"
#include "e_settings.h"
#include "dxC_target.h"
#include "dxC_settings.h"
#include <algorithm>


//-----------------------------------------------------------------------------

bool E_SCREEN_DISPLAY_MODE_REFRESH::operator==(const E_SCREEN_DISPLAY_MODE_REFRESH & rhs) const
{
	return
		(static_cast<unsigned __int64>(Numerator) * static_cast<unsigned __int64>(rhs.Denominator))
			==
		(static_cast<unsigned __int64>(rhs.Numerator) * static_cast<unsigned __int64>(Denominator));
}

bool E_SCREEN_DISPLAY_MODE_REFRESH::operator!=(const E_SCREEN_DISPLAY_MODE_REFRESH & rhs) const
{
	return !(*this == rhs);
}

bool E_SCREEN_DISPLAY_MODE_REFRESH::operator<(const E_SCREEN_DISPLAY_MODE_REFRESH & rhs) const
{
	return
		(static_cast<unsigned __int64>(Numerator) * static_cast<unsigned __int64>(rhs.Denominator))
			<
		(static_cast<unsigned __int64>(rhs.Numerator) * static_cast<unsigned __int64>(Denominator));
}

bool E_SCREEN_DISPLAY_MODE_REFRESH::operator>(const E_SCREEN_DISPLAY_MODE_REFRESH & rhs) const
{
	return
		(static_cast<unsigned __int64>(Numerator) * static_cast<unsigned __int64>(rhs.Denominator))
			>
		(static_cast<unsigned __int64>(rhs.Numerator) * static_cast<unsigned __int64>(Denominator));
}

float E_SCREEN_DISPLAY_MODE_REFRESH::AsFloat(void) const
{
	return static_cast<float>(static_cast<double>(Numerator) / static_cast<double>(Denominator));
}

int E_SCREEN_DISPLAY_MODE_REFRESH::AsInt(void) const
{
	return static_cast<int>(AsFloat() + 0.5f);
}

//-----------------------------------------------------------------------------

E_SCREEN_DISPLAY_MODE::E_SCREEN_DISPLAY_MODE(void)
{
	Width = 0;
	Height = 0;
	RefreshRate.Numerator = 0;
	RefreshRate.Denominator = 1;
	Format = D3DFMT_UNKNOWN;
	ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
}

bool E_SCREEN_DISPLAY_MODE::IsWindowed(void) const
{
	return (RefreshRate.Numerator == 0) || (RefreshRate.Denominator == 0);
}

void E_SCREEN_DISPLAY_MODE::SetWindowed(void)
{
	RefreshRate.Numerator = 0;
	RefreshRate.Denominator = 1;
}

void E_SCREEN_DISPLAY_MODE::DefaultFullscreen(void)
{
	RefreshRate.Numerator = SETTINGS_DEF_FULLSCREEN_REFRESH_RATE_NUMERATOR;
	RefreshRate.Denominator = SETTINGS_DEF_FULLSCREEN_REFRESH_RATE_DENOMINATOR;
}

//-----------------------------------------------------------------------------

static bool sResolutionShouldEnumerate( const E_SCREEN_DISPLAY_MODE & tMode )
{
	return e_ResolutionShouldEnumerate( tMode );
}

//-----------------------------------------------------------------------------

const E_SCREEN_DISPLAY_MODE_GROUP e_Screen_EnumerateDisplayModesGroupedByRefreshRate(void)
{
	E_SCREEN_DISPLAY_MODE_GROUP g;

	E_SCREEN_DISPLAY_MODE_VECTOR v = e_Screen_EnumerateDisplayModes();
	std::sort(v.begin(), v.end());
	v.push_back(E_SCREEN_DISPLAY_MODE());	// CHB 2007.10.02 - Sentinel item.

	E_SCREEN_DISPLAY_MODE_VECTOR::const_iterator iRangeStart = v.begin();
	for (E_SCREEN_DISPLAY_MODE_VECTOR::const_iterator i = v.begin() + 1; i != v.end(); ++i)
	{
		if (!e_Screen_AreDisplayModesEqualAsideFromRefreshRate(*iRangeStart, *i))
		{
			// Unequal aside from refresh rate.
			E_SCREEN_DISPLAY_MODE_VECTOR t(iRangeStart, i);
			g.push_back(t);
			iRangeStart = i;
		}
	}

	return g;
}

bool operator<(const E_SCREEN_DISPLAY_MODE & lhs, const E_SCREEN_DISPLAY_MODE & rhs)
{
	unsigned nAreaLHS = lhs.Width * lhs.Height;
	unsigned nAreaRHS = rhs.Width * rhs.Height;

	// Larger areas first.
	if (nAreaLHS < nAreaRHS)
	{
		return false;
	}
	if (nAreaLHS > nAreaRHS)
	{
		return true;
	}

	// Larger widths next.
	if (lhs.Width < rhs.Width)
	{
		return false;
	}
	if (lhs.Width > rhs.Width)
	{
		return true;
	}

	// Larger heights next.
	if (lhs.Height < rhs.Height)
	{
		return false;
	}
	if (lhs.Height > rhs.Height)
	{
		return true;
	}

	// Larger refresh rates next.
	if (lhs.RefreshRate < rhs.RefreshRate)
	{
		return false;
	}
	if (lhs.RefreshRate > rhs.RefreshRate)
	{
		return true;
	}

	// Equal.
	return false;
}

bool e_Screen_AreDisplayModesEqualAsideFromRefreshRate(const E_SCREEN_DISPLAY_MODE & lhs, const E_SCREEN_DISPLAY_MODE & rhs)
{
	return
		(lhs.Width == rhs.Width) &&
		(lhs.Height == rhs.Height) &&
		(lhs.Format == rhs.Format);
}

static
bool operator==(const E_SCREEN_DISPLAY_MODE & lhs, const E_SCREEN_DISPLAY_MODE & rhs)
{
	return
		e_Screen_AreDisplayModesEqualAsideFromRefreshRate(lhs, rhs) &&
		(lhs.RefreshRate == rhs.RefreshRate);
}

const E_SCREEN_DISPLAY_MODE e_Screen_GetCurrentDisplayMode(void)
{
	E_SCREEN_DISPLAY_MODE CurrentDM;
	if (SUCCEEDED(dxC_GetD3DDisplayMode(CurrentDM)))
	{
		return CurrentDM;
	}
	return E_SCREEN_DISPLAY_MODE();
}

const E_SCREEN_DISPLAY_MODE_VECTOR e_Screen_EnumerateDisplayModes(void)
{
	E_SCREEN_DISPLAY_MODE_VECTOR v;

	DXC_FORMAT nFormat = D3DFMT_A8R8G8B8;

#ifdef ENGINE_TARGET_DX9
	// DX10 doesn't support getting the current display mode, but 32 bit should always be valid...
	// ... and I'll regret writing that, for sure
	E_SCREEN_DISPLAY_MODE CurrentDM;
	if (SUCCEEDED(dxC_GetD3DDisplayMode(CurrentDM)))
	{
		nFormat = static_cast<DXC_FORMAT>(CurrentDM.Format);
	}
#endif

	unsigned nCount = dxC_GetDisplayModeCount(nFormat);
	v.reserve(nCount);

	for (unsigned i = 0; i < nCount; ++i)
	{
		E_SCREEN_DISPLAY_MODE em;
		HRESULT hr = dxC_EnumDisplayModes(nFormat, i, em, true);
		if (FAILED(hr))
		{
			break;
		}

		// CHB 2007.03.06 - Skip ones that are too small.
		if ((em.Width < SETTINGS_MIN_RES_WIDTH) || (em.Height < SETTINGS_MIN_RES_HEIGHT))
		{
			continue;
		}

		// CML 2007.09.21 - Skip ones that don't meet our "display" criteria.
		if ( ! sResolutionShouldEnumerate( em ) )
		{
			continue;
		}

		// CHB 2007.02.06 - Some drivers report duplicates; do not allow them.
		E_SCREEN_DISPLAY_MODE_VECTOR::iterator j = std::lower_bound(v.begin(), v.end(), em);
		if (j != v.end())
		{
			if (*j == em)
			{
				continue;
			}
		}
		v.insert(j, em);
	}

	return v;
}

bool e_Screen_IsModeValidForWindowedDisplay(const E_SCREEN_DISPLAY_MODE & Mode)
{
	int nDesktopWidth, nDesktopHeight;
	e_GetDesktopSize(nDesktopWidth, nDesktopHeight);
	return (Mode.Width <= nDesktopWidth) && (Mode.Height <= nDesktopHeight);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#include "e_optionstate.h"

void e_Screen_DisplayModeFromState(const OptionState * pState, E_SCREEN_DISPLAY_MODE & Mode)
{
	Mode.Width = pState->get_nFrameBufferWidth();
	Mode.Height = pState->get_nFrameBufferHeight();
	if (pState->get_bWindowed())
	{
		Mode.SetWindowed();
	}
	else
	{
		ASSERTV( ! e_IsNoRender(), "Trying to set fullscreen in a no-render run!" );

		Mode.RefreshRate.Numerator = pState->get_nScreenRefreshHzNumerator();
		Mode.RefreshRate.Denominator = pState->get_nScreenRefreshHzDenominator();
		ASSERT_DO(!Mode.IsWindowed())
		{
			Mode.DefaultFullscreen();
		}
	}
	Mode.Format = pState->get_nFrameBufferColorFormat();
	Mode.ScanlineOrdering = pState->get_nScanlineOrdering();
    Mode.Scaling = pState->get_nScaling();
}

void e_Screen_DisplayModeToStateNoSuspend(OptionState * pState, const E_SCREEN_DISPLAY_MODE & Mode)
{
	pState->set_nFrameBufferWidth(Mode.Width);
	pState->set_nFrameBufferHeight(Mode.Height);
	if (Mode.IsWindowed())
	{
		pState->set_bWindowed(true);
	}
	else
	{
		pState->set_bWindowed(false);
		pState->set_nScreenRefreshHzNumerator(Mode.RefreshRate.Numerator);
		pState->set_nScreenRefreshHzDenominator(Mode.RefreshRate.Denominator);
	}
	pState->set_nFrameBufferColorFormat(static_cast<DXC_FORMAT>(Mode.Format));
	pState->set_nScanlineOrdering(Mode.ScanlineOrdering);
    pState->set_nScaling(Mode.Scaling);
}

void e_Screen_DisplayModeToState(OptionState * pState, const E_SCREEN_DISPLAY_MODE & Mode)
{
	pState->SuspendUpdate();
	e_Screen_DisplayModeToStateNoSuspend(pState, Mode);
	pState->ResumeUpdate();
}
