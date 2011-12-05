//----------------------------------------------------------------------------
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "dwmdisabler.h"

#if 0
#include <dwmapi.h>
#else
// dwmapi.h not available - but it's in the Windows Vista Platform SDK
enum
{
	DWM_EC_DISABLECOMPOSITION,
	DWM_EC_ENABLECOMPOSITION,
};
#endif

namespace
{

HRESULT sDwmEnableComposition(UINT uCompositionAction)
{
	HRESULT nResult = S_FALSE;
	HMODULE hMod = ::LoadLibraryA("dwmapi.dll");
	if (hMod != 0)
	{
		FARPROC pProc = ::GetProcAddress(hMod, "DwmEnableComposition");
		if (pProc != 0)
		{
			typedef HRESULT (STDAPICALLTYPE DWMENABLECOMPOSITION_PROC)(UINT);
			DWMENABLECOMPOSITION_PROC * pDwmEnableComposition = static_cast<DWMENABLECOMPOSITION_PROC *>(static_cast<void *>(pProc));
			nResult = (*pDwmEnableComposition)(uCompositionAction);
		}
		::FreeLibrary(hMod);
	}
	return nResult;
}

};

DWMDisabler::DWMDisabler(void)
{
	sDwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
}

DWMDisabler::~DWMDisabler(void)
{
	sDwmEnableComposition(DWM_EC_ENABLECOMPOSITION);
}
