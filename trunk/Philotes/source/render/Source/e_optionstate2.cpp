//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

// This file is intentionally segregated to isolate dependencies.

#include "e_pch.h"
#include "e_optionstate.h"

#include "dxC_caps.h"
#include "dxC_settings.h"
#include "e_settings.h"
#include "e_screen.h"
#include "lod.h"

//-----------------------------------------------------------------------------

PRESULT dxC_Settings_RegisterEventHandler(void);
PRESULT dxC_Device_RegisterEventHandler(void);
PRESULT dxC_Shadow_RegisterEventHandler(void);
PRESULT dxC_Effect_RegisterEventHandler(void);
PRESULT dxC_TextureLOD_RegisterEventHandler(void);

void e_OptionState_InitModule(void);

//-----------------------------------------------------------------------------

namespace
{

PRESULT e_OptionState_RegisterEventHandlers(void)
{
	// Order is important.
	V(dxC_Settings_RegisterEventHandler());		// this should be first
	V(dxC_Device_RegisterEventHandler());
	V(dxC_Shadow_RegisterEventHandler());
	V(dxC_Effect_RegisterEventHandler());
	V(dxC_TextureLOD_RegisterEventHandler());
	return S_OK;
}

void e_OptionState_SetCaps(OptionState * pState)
{
	// CHB 2007.06.29 - The max effect target should be initialized
	// only once.  This was being set multiple times, importantly,
	// after the user's shader model setting had already been
	// loaded -- thus overwriting the user's choice.
	if (pState->get_nMaxEffectTarget() == FXTGT_INVALID)
	{
		pState->set_nMaxEffectTarget( (DWORD)dxC_GetEffectTargetFromShaderVersions( dxC_CapsGetMaxVertexShaderVersion(), dxC_CapsGetMaxPixelShaderVersion() ) );
	}
	pState->set_bGlow(e_IsGlowAvailable( pState ));
}

PRESULT e_OptionState_Init(OptionState * pState)
{
	bool bWindowed = false;
	if ( e_IsNoRender() )
		bWindowed = true;

	// Override the initial windowed mode based on command-line flags.
	if ( AppCommonGetForceWindowed() )
		bWindowed = true;

	pState->set_nAdapterOrdinal(DXC_9_10(D3DADAPTER_DEFAULT, 0));
	pState->set_nDeviceDriverType(DXC_9_10(D3DDEVTYPE_HAL, D3D10_DRIVER_TYPE_HARDWARE));
	pState->set_nFrameBufferColorFormat(dxC_GetDefaultDisplayFormat(bWindowed));
	//trace( "### %s:%d -- Setting backbuffer color format to %d -- %s\n", __FILE__, __LINE__, dxC_GetDefaultDisplayFormat(bWindowed), __FUNCSIG__ );
	pState->set_nFrameBufferWidth(SETTINGS_DEF_WINDOWED_RES_WIDTH);
	pState->set_nFrameBufferHeight(SETTINGS_DEF_WINDOWED_RES_HEIGHT);
	pState->set_nScreenRefreshHzNumerator(SETTINGS_DEF_FULLSCREEN_REFRESH_RATE_NUMERATOR);
	pState->set_nScreenRefreshHzDenominator(SETTINGS_DEF_FULLSCREEN_REFRESH_RATE_DENOMINATOR);
	pState->set_nScanlineOrdering(DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED);
	pState->set_nScaling(DXGI_MODE_SCALING_UNSPECIFIED);
	pState->set_bFlipWaitVerticalRetrace(true);
	pState->set_bWindowed(bWindowed);
	pState->set_nBackBufferCount(1);
	pState->set_bTripleBuffer(false);
	pState->set_nMultiSampleType(DXC_9_10(D3DMULTISAMPLE_NONE, 1));
	pState->set_nMultiSampleQuality(0);

//	pState->set_nShadowType(SHADOW_TYPE_NONE);
	pState->set_nShadowReductionFactor(0);
	pState->set_nMaxEffectTarget(FXTGT_INVALID);
	pState->set_fEffectLODBias(0.f);
	pState->set_fEffectLODScale(1.f);
	pState->set_bGlow(true);
	pState->set_bSpecular(true);
	pState->set_fTextureQuality(1.0f);
	pState->set_fGammaPower(1.f);
	pState->set_bDepthOnlyPass(false);
	pState->set_fLODBias(0.0f);
	pState->set_fLODScale(1.0f);
	pState->set_bAnisotropic(true);
	pState->set_bDynamicLights(true);
	pState->set_bTrilinear(true);
	pState->set_fCullSizeScale(1.0f);
	pState->set_nBackgroundShowForceLOD(LOD_HIGH);
	pState->set_nBackgroundLoadForceLOD(LOD_HIGH);
	pState->set_nAnimatedShowForceLOD(LOD_NONE);
	pState->set_nAnimatedLoadForceLOD(LOD_NONE);
	pState->set_bAsyncEffects(true);
	pState->set_bFluidEffects(true);

	if ( ! bWindowed )
	{
		E_SCREEN_DISPLAY_MODE tMode;
		if ( FAILED( e_DetectDefaultFullscreenResolution( tMode, pState ) ) )
		{
			pState->set_bWindowed(TRUE);
		}
		else
		{
			pState->set_nFrameBufferWidth(tMode.Width);
			pState->set_nFrameBufferHeight(tMode.Height);
			pState->set_nScreenRefreshHzNumerator(tMode.RefreshRate.Numerator);
			pState->set_nScreenRefreshHzDenominator(tMode.RefreshRate.Denominator);
			pState->set_nScaling(tMode.Scaling);
			pState->set_nScanlineOrdering(tMode.ScanlineOrdering);
		}
	}

	return S_OK;
}

}; // nameless namespace

//-----------------------------------------------------------------------------

PRESULT e_OptionState_Init(void)
{
	e_OptionState_InitModule();

	V_RETURN(e_OptionState_RegisterEventHandlers());

	CComPtr<OptionState> pState;
	PRESULT nResult = e_OptionState_CloneActive(&pState);
	V_RETURN(nResult);
	ASSERT_RETVAL(pState != 0, E_FAIL);

	pState->SuspendUpdate();
	e_OptionState_Init(pState);
	pState->ResumeUpdate();
	V_RETURN(e_OptionState_CommitToActive(pState));
	return S_OK;
}

PRESULT e_OptionState_InitCaps(void)
{
	CComPtr<OptionState> pState;
	PRESULT nResult = e_OptionState_CloneActive(&pState);
	V_RETURN(nResult);
	ASSERT_RETVAL(pState != 0, E_FAIL);

	pState->SuspendUpdate();
	e_OptionState_SetCaps(pState);
	pState->ResumeUpdate();
	V_RETURN(e_OptionState_CommitToActive(pState));
	return S_OK;
}
