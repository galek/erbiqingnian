//----------------------------------------------------------------------------
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#if ISVERSION(SERVER_VERSION)

#include "dxC_device.h"
#include "dx9_device.h"

DXCADAPTER dxC_GetAdapter(void)
{
	return 0;
}

D3DC_DRIVER_TYPE dx9_GetDeviceType(void)
{
	return D3DDEVTYPE_REF;
}

PRESULT e_OptionState_InitCaps(void)
{
	return S_OK;
}

void e_PreNotifyWindowMessage(HWND /*hWnd*/, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
}

void e_NotifyWindowMessage(HWND /*hWnd*/, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
}

#endif

