//----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_minspec.h"
#include "dxC_caps.h"
#include "prime.h"
#include "globalindex.h"	// CHB 2007.08.01
#include "dxC_pixo.h"

//----------------------------------------------------------------------------

using namespace FSSE;

namespace
{

//----------------------------------------------------------------------------

struct Specs
{
	unsigned nCPUClockInMHz;
	DWORD nVertexShaderVersion;
	DWORD nPixelShaderVersion;
	BOOL bHasSSE;
};

//----------------------------------------------------------------------------

bool sCheckMinSpec(const Specs & In, WCHAR * pszErrorMsg, int nMsgBufLen)
{
	if (pszErrorMsg != 0)
	{
		pszErrorMsg[0] = L'\0';
	}

#ifdef ENGINE_TARGET_DX9
	// CHB 2007.03.01 - Check for minimum shader version.
	// This WILL fail for SWVP.
	DWORD dwMinVS = D3DVS_VERSION(1,1);
	DWORD dwMinPS = D3DPS_VERSION(1,1);

#ifdef USE_PIXO
	if ( dxC_CanUsePixomatic() )
	{
		// Pixomatic removes this minspec requirement
		dwMinVS = 0;
		dwMinPS = 0;
	}
#endif

	if ((In.nVertexShaderVersion < dwMinVS) || (In.nPixelShaderVersion < dwMinPS))
	{
		if ( pszErrorMsg )
		{
			const WCHAR * const pText = GlobalStringGetEx(GS_MSG_ERROR_INSUFFICIENT_SHADER_VERSION_TEXT, L"This application requires Vertex Shader and Pixel Shader support above the capabilities of this system's video hardware.");
			PStrPrintf( pszErrorMsg, nMsgBufLen, pText,
				D3DSHADER_VERSION_MAJOR( dwMinVS ),
				D3DSHADER_VERSION_MINOR( dwMinVS ),
				D3DSHADER_VERSION_MAJOR( dwMinPS ),
				D3DSHADER_VERSION_MINOR( dwMinPS ),
				D3DSHADER_VERSION_MAJOR( In.nVertexShaderVersion ),
				D3DSHADER_VERSION_MINOR( In.nVertexShaderVersion ),
				D3DSHADER_VERSION_MAJOR( In.nPixelShaderVersion ),
				D3DSHADER_VERSION_MINOR( In.nPixelShaderVersion ) );
		}
		return false;
	}
#endif

	//
	const unsigned nMinCPU = 990;
	if (In.nCPUClockInMHz < nMinCPU)
	{
		if ( pszErrorMsg )
		{
			// CHB !!! 2007.03.01 - The user's going to get sick of seeing this message every run...
			const WCHAR * const pText = GlobalStringGetEx(GS_MSG_WARNING_INSUFFICIENT_CPU_SPEED_TEXT, L"This application requires a CPU of speed faster than this system's CPU.");
			PStrPrintf( pszErrorMsg, nMsgBufLen, pText,
				nMinCPU,
				nMinCPU / 1000,
				nMinCPU % 1000 / 100,
				In.nCPUClockInMHz,
				In.nCPUClockInMHz / 1000,
				In.nCPUClockInMHz % 1000 / 100 );
		}
	}

	if ( ! In.bHasSSE )
	{
		if ( pszErrorMsg )
		{
			const WCHAR * const pText = GlobalStringGetEx(GS_MSG_WARNING_INSUFFICIENT_CPU_SPEED_TEXT, L"This application requires a CPU that supports the SSE instruction set.");
			PStrPrintf( pszErrorMsg, nMsgBufLen, pText );
		}
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------

};	// anonymous namespace

//----------------------------------------------------------------------------

void e_MinSpec_Check(void)
{
	if ( e_IsNoRender() )
		return;

	Specs Us;

#ifdef ENGINE_TARGET_DX9
	Us.nVertexShaderVersion = dx9_CapGetValue(DX9CAP_MAX_VS_VERSION);
	Us.nPixelShaderVersion = dx9_CapGetValue(DX9CAP_MAX_PS_VERSION);
#else
	Us.nVertexShaderVersion = VS_4_0;
	Us.nPixelShaderVersion  = PS_4_0;
#endif

	Us.nCPUClockInMHz = e_CapGetValue(CAP_CPU_SPEED_MHZ);
	Us.bHasSSE = CPUHasSSE();

	WCHAR szErrorMsg[256];
	bool bQuit = !sCheckMinSpec(Us, szErrorMsg, _countof(szErrorMsg));

	if ( szErrorMsg[0] )
	{
		GLOBAL_DEFINITION * pDefinition = (GLOBAL_DEFINITION *) DefinitionGetGlobal();
		if (pDefinition)
		{
			if ( 0 == ( pDefinition->dwFlags & GLOBAL_FLAG_NO_POPUPS ) )
			{
				const WCHAR * const pCaption = GlobalStringGetEx(GS_MSG_MIN_SPEC_ISSUE_TITLE, L"Minimum System Requirements Not Met");
				::MessageBoxW( AppCommonGetHWnd(), szErrorMsg, pCaption, MB_OK | MB_ICONEXCLAMATION );	// CHB 2007.08.01 - String audit: USED IN RELEASE
			}
		}

		if ( bQuit )
		{
			PostQuitMessage( -139 );
		}
	}
}
