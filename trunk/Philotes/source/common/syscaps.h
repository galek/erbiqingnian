//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef _SYSCAPS_H_
#define _SYSCAPS_H_

// Formerly GeneralHardwareCaps
struct SYSCAPS
{
	DWORDLONG		dwlPhysicalSystemMemoryBytes;
	DWORD			dwPhysicalVideoMemoryBytes;
	DWORD			dwCPUSpeedMHz;
	DWORD			dwSysClockSpeedMHz;
	DWORD			dwNativeResolutionWidth;
	DWORD			dwNativeResolutionHeight;
};

const SYSCAPS & SysCaps_Get(void);
void SysCaps_Init(void);

#endif
