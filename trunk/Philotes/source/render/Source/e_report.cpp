//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#include "e_pch.h"
#include "sysinfo.h"
#include "syscaps.h"
#include "e_settings.h"
#include "dxC_caps.h"
#include "e_optionstate.h"

extern VideoDeviceInfo gtAdapterID;	// defined in dx9_caps.cpp

namespace
{

void e_ReportVideoCard(SysInfoLog * pLog)
{
	if ( ! pLog )
		return;

	pLog->NewLine();
	pLog->WriteLn(SYSINFO_TEXT("* Video Adapter"));

	const VideoDeviceInfo * pID = &gtAdapterID;

	//pLog->WriteLn(SYSINFO_TEXT(" Driver:      \"%hs\""), pID->Driver);
	pLog->WriteLn(SYSINFO_TEXT(" Device:      \"%hs\""), pID->szDeviceName);
	//pLog->WriteLn(SYSINFO_TEXT(" Device:      \"%hs\""), pID->DeviceName);

	SYSINFO_CHAR VerStr[SYSINFO_VERSION_STRING_SIZE];
	SysInfo_VersionToString(VerStr, _countof(VerStr), pID->DriverVersion.QuadPart);
	pLog->WriteLn(SYSINFO_TEXT(" Driver Ver.: %s"), VerStr);

	pLog->WriteLn(SYSINFO_TEXT(" Vendor ID:   %04x"), pID->VendorID);
	pLog->WriteLn(SYSINFO_TEXT(" Device ID:   %04x"), pID->DeviceID);
	pLog->WriteLn(SYSINFO_TEXT(" Subsys ID:   %08x"), pID->SubSysID);
	pLog->WriteLn(SYSINFO_TEXT(" Revision:    %04x"), pID->Revision);
	//pLog->WriteLn(SYSINFO_TEXT(" Device GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
	//	pID->DeviceIdentifier.Data1,
	//	pID->DeviceIdentifier.Data2,
	//	pID->DeviceIdentifier.Data3,
	//	pID->DeviceIdentifier.Data4[0],
	//	pID->DeviceIdentifier.Data4[1],
	//	pID->DeviceIdentifier.Data4[2],
	//	pID->DeviceIdentifier.Data4[3],
	//	pID->DeviceIdentifier.Data4[4],
	//	pID->DeviceIdentifier.Data4[5],
	//	pID->DeviceIdentifier.Data4[6],
	//	pID->DeviceIdentifier.Data4[7]
	//);
	//pLog->WriteLn(SYSINFO_TEXT(" WHQL Level:  %08x"), pID->SubSysID);

	pLog->NewLine();
	pLog->WriteLn(SYSINFO_TEXT(" Video Memory: %u MB"), SysCaps_Get().dwPhysicalVideoMemoryBytes / 1048576);

	const OptionState * pState = e_OptionState_GetActive();
	if ( pState )
	{
		unsigned nAdapter = pState->get_nAdapterOrdinal();
		pLog->NewLine();
		pLog->WriteLn(SYSINFO_TEXT(" Adapter Ord.: %u"), nAdapter);

#ifdef ENGINE_TARGET_DX9
		IDirect3D9 * pD3D = dx9_GetD3D();
		if (pD3D != 0)
		{
			D3DDISPLAYMODE DisplayMode;
			pD3D->GetAdapterDisplayMode(nAdapter, &DisplayMode);
			pLog->NewLine();
			pLog->WriteLn(SYSINFO_TEXT(" Display Mode: %u x %u"), DisplayMode.Width, DisplayMode.Height);
			pLog->WriteLn(SYSINFO_TEXT(" Refresh Rate: %u Hz"), DisplayMode.RefreshRate);
			pLog->WriteLn(SYSINFO_TEXT(" Format:       %u"), DisplayMode.Format);
		}
#endif
	}
}

void e_ReportCaps(SysInfoLog * pLog)
{
	pLog->NewLine();
	pLog->WriteLn(SYSINFO_TEXT("* Caps"));
	for (int i = DX9CAP_DISABLE + 1; i < NUM_PLATFORM_CAPS; ++i)
	{
		PLATFORM_CAP nFlag = static_cast<PLATFORM_CAP>(i);
		DWORD nVal = dx9_CapGetValue(nFlag);
		DWORD nMin, nMax;
		V(dx9_PlatformCapGetMinMax(nFlag, nMin, nMax));
		char buf[5];
		dx9_CapGetFourCCString(nFlag, buf, _countof(buf));
		pLog->WriteLn(SYSINFO_TEXT(" %2d: %hs = 0x%08x [0x%08x, 0x%08x]"), nFlag, buf, nVal, nMin, nMax);
	}
}

void e_ReportRenderFlags(SysInfoLog * pLog)
{
	pLog->NewLine();
	pLog->WriteLn(SYSINFO_TEXT("* Render Flags"));
	for (int i = 0; i < NUM_RENDER_FLAGS; ++i)
	{
		RENDER_FLAG nFlag = static_cast<RENDER_FLAG>(i);
		int nVal = e_GetRenderFlag(nFlag);
		pLog->WriteLn(SYSINFO_TEXT(" %2d: %-20hs = 0x%08x (%d)"), nFlag, e_GetRenderFlagName(nFlag), nVal, nVal);
	}
}

void e_ReportCallback(SysInfoLog * pLog, void * /*pContext*/)
{
	pLog->NewLine();
	pLog->WriteLn(SYSINFO_TEXT("-- BEGIN ENGINE REPORT --"));
	e_ReportVideoCard(pLog);
	e_ReportCaps(pLog);
	e_ReportRenderFlags(pLog);
	pLog->NewLine();
	pLog->WriteLn(SYSINFO_TEXT("-- END ENGINE REPORT --"));
}

};

void e_ReportInit(void)
{
	SysInfo_RegisterReportCallback(&e_ReportCallback, 0);
}
