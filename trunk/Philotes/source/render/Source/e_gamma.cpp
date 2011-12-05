//----------------------------------------------------------------------------
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_gamma.h"
#include "e_optionstate.h"
#include "dxC_caps.h"
#include "dxC_target.h"
#include "dxC_state.h"
#include "dxC_2d.h"

//----------------------------------------------------------------------------

static float s_fGammaShadow = 1.0f;

//----------------------------------------------------------------------------

// The gamma function with inputs in [0,255], scaled to a range with the
// default range appropriate for D3DGAMMARAMP.
#ifdef ENGINE_TARGET_DX10
inline float ramp_val(UINT i, float divisor, float recip_gamma, int scale = 65535)
{
	return powf( i / divisor, recip_gamma );
}
#else if defined( ENGINE_TARGET_DX9 )
inline WORD ramp_val(UINT i, float divisor, float recip_gamma, int scale = 65535)
{
	return static_cast<WORD>( scale * powf( i / divisor, recip_gamma ) );
}
#endif


PRESULT e_SetGammaPercent( float fGammaPct )
{
#ifdef ENGINE_TARGET_DX9
	if ( ! dxC_GetDevice() )
		return S_FALSE;
#endif

	fGammaPct = PIN(fGammaPct, 0.0f, 1.0f);
	float fGamma = MAP_VALUE_TO_RANGE( fGammaPct, 0.f, 1.f, MIN_GAMMA_ADJUST, MAX_GAMMA_ADJUST );

	return e_SetGamma( fGamma );
}

PRESULT e_SetGamma( float fGamma )
{
#ifdef ENGINE_TARGET_DX9
	if ( ! dxC_GetDevice() )
		return S_FALSE;
#endif

	fGamma = PIN(fGamma, MIN_GAMMA_ADJUST, MAX_GAMMA_ADJUST);

	// compute 1/gamma
	//const float recip_gray = recip_gamma(m_gamma[G_GRAY]);
	const float fRecipGamma = 1.0f / fGamma;
	s_fGammaShadow = fRecipGamma;

	D3DC_GAMMA_CONTROL tGammaRamp;
	// compute i**(1/gamma) for all i and scale to range
	UINT nCount = 256;

#ifdef ENGINE_TARGET_DX10
	if( !e_IsFullscreen() )  //Can't handle windowed
		return S_OK;

	IDXGIOutput *pNewFavoriteOutput = NULL;
	V_RETURN( dxC_GetD3DSwapChain()->GetContainingOutput( &pNewFavoriteOutput) );

	DXGI_GAMMA_CONTROL_CAPABILITIES gammaCap;
	pNewFavoriteOutput->GetGammaControlCapabilities( &gammaCap );
	nCount = gammaCap.NumGammaControlPoints;
	pNewFavoriteOutput->GetGammaControl( &tGammaRamp );
	
#endif

	for (UINT i = 0; i < nCount; i++)
	{
		DX9_BLOCK( tGammaRamp.red[i] = tGammaRamp.green[i] = tGammaRamp.blue[i] = )
		DX10_BLOCK( tGammaRamp.GammaCurve[i].Red = tGammaRamp.GammaCurve[i].Blue = tGammaRamp.GammaCurve[i].Green = )
			ramp_val(i, nCount - 1, fRecipGamma) ;
	}

	DX9_BLOCK( dxC_GetDevice()->SetGammaRamp( 0, D3DSGR_NO_CALIBRATION, &tGammaRamp ); )

#ifdef ENGINE_TARGET_DX10
	pNewFavoriteOutput->SetGammaControl( &tGammaRamp ); 
	SAFE_RELEASE( pNewFavoriteOutput );
#endif

	return S_OK;
}


float e_MapGammaPctToPower( float fGammaPct )
{
	fGammaPct = PIN(fGammaPct, 0.f, 1.f);
	return MAP_VALUE_TO_RANGE( fGammaPct, 0.f, 1.f, MIN_GAMMA_ADJUST, MAX_GAMMA_ADJUST );
}

float e_MapGammaPowerToPct( float fGammaPower )
{
	fGammaPower = PIN(fGammaPower, MIN_GAMMA_ADJUST, MAX_GAMMA_ADJUST);
	return MAP_VALUE_TO_RANGE( fGammaPower, MIN_GAMMA_ADJUST, MAX_GAMMA_ADJUST, 0.f, 1.f );
}


PRESULT e_UpdateGamma(void)
{
	return e_SetGamma(e_OptionState_GetActive()->get_fGammaPower());
}

//----------------------------------------------------------------------------

bool e_CanDoFullscreenGamma(void)
{
#if defined(ENGINE_TARGET_DX9)
	return dx9_CapGetValue(DX9CAP_HARDWARE_GAMMA_RAMP) != 0;
#elif defined(ENGINE_TARGET_DX10)
	return true;
#else
	return false;
#endif
}

bool e_CanDoWindowedGamma(void)
{
	// No windowed gamma in hardware.
	return false;
}

bool e_CanDoManualGamma(const OptionState * pState)
{
	// Gamma shader requires 2.0.
	return pState->get_nMaxEffectTarget() >= FXTGT_SM_20_LOW;
}

bool e_CanDoGamma(const OptionState * pState, bool bWindowed)
{
	return e_CanDoManualGamma(pState) || (bWindowed ? e_CanDoWindowedGamma() : e_CanDoFullscreenGamma());
}

bool e_ShouldUseManualGamma(const OptionState * pState, bool bWindowed)
{
	return e_CanDoManualGamma(pState) && !(bWindowed ? e_CanDoWindowedGamma() : e_CanDoFullscreenGamma());
}

PRESULT e_RenderManualGamma(void)
{
	const OptionState * pState = e_OptionState_GetActive();

	if (!e_ShouldUseManualGamma(pState, pState->get_bWindowed()))
	{
		return S_FALSE;
	}

	// No-op.
	if (s_fGammaShadow == 1.0f)
	{
		return S_FALSE;
	}

	// turn off fog
	V( e_SetFogEnabled( FALSE ) );

	// get the effect
	D3D_EFFECT * pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_GAMMA ) );
	if ( ! pEffect )
		return S_FALSE;
	if ( ! dxC_EffectIsLoaded( pEffect ) )
		return S_FALSE;

	EFFECT_TECHNIQUE * ptTechnique = NULL;
	TECHNIQUE_FEATURES tFeatures;
	tFeatures.Reset();
	tFeatures.nInts[ TF_INT_INDEX ] = 0;
	V( dxC_EffectGetTechniqueByFeatures( pEffect, tFeatures, &ptTechnique ) );
	ASSERT_RETFAIL( ptTechnique && ptTechnique->hHandle );

	V(dx9_EffectSetFloat(pEffect, *ptTechnique, EFFECT_PARAM_GAMMA_POWER, s_fGammaShadow));

	UINT nPassCount;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &nPassCount ) );
	ASSERT_RETFAIL( nPassCount == 1 );

	UINT nPass = 0;
	{
		D3DC_EFFECT_PASS_HANDLE hPass = dxC_EffectGetPassByIndex( pEffect->pD3DEffect, ptTechnique->hHandle, nPass );
		ASSERT(hPass != 0);
		if (hPass != 0)
		{
			RENDER_TARGET_INDEX eFullRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FULL );
			V_RETURN( dxC_CopyBackbufferToTexture( dxC_RTResourceGet( eFullRT ), NULL, NULL ) );
			V( dxC_RTSetDirty( eFullRT ) );
			SPD3DCBASESHADERRESOURCEVIEW pSrc = dxC_RTShaderResourceGet( eFullRT );
			ASSERT( pSrc );

			RENDER_TARGET_INDEX eFinalRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FINAL_BACKBUFFER );

			V_RETURN( dxC_EffectBeginPass( pEffect, nPass ) );
			V_RETURN( dxC_SetRenderTargetWithDepthStencil( eFinalRT, DEPTH_TARGET_NONE ) );
			V( dxC_EffectSetPixelAccurateOffset( pEffect, *ptTechnique ) );
			V( dxC_EffectSetScreenSize( pEffect, *ptTechnique, eFinalRT ) );
			V( dxC_EffectSetViewport( pEffect, *ptTechnique, eFinalRT ) );
			V( dx9_EffectSetTexture( pEffect, *ptTechnique, EFFECT_PARAM_DIFFUSEMAP0, pSrc ) );
			dxC_SetRenderState( D3DRS_ZENABLE, FALSE );
			dxC_SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
			V_RETURN(dx9_ScreenDrawCover(pEffect));
		}
	}
	return S_OK;
}
