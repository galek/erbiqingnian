//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#include "e_pch.h"
#include "e_featureline.h"
#include "e_optionstate.h"
#include "e_effect_priv.h"
#include "e_initdb.h"
#include "dxC_caps.h"
#include "dxC_pixo.h"

#pragma warning(disable:4481)	// warning C4481: nonstandard extension used: override specifier 'override'

using namespace FSSE;

namespace
{

//-----------------------------------------------------------------------------

struct StopData
{
	DWORD			nName;
	GLOBAL_STRING	eGlobalStr;
	char			szName[32];
	int				nMaxEffectTarget;
	int				nMaxCompatibleEffectTarget;
	float			fEffectLODScale;			// scale on the shader LOD fallback distances
	float			fEffectLODBias;				// bias  on the shader LOD fallback distances
	bool			bDX10ScreenFX;				// DX10-only screen effects (DoF, motion blur)
};

//-----------------------------------------------------------------------------

#define _FCC(x) E_FEATURELINE_FOURCC(x)
const StopData s_Stops[] =
{
	{	_FCC("SMFF"),	GS_SETTINGS_LOW,			"9_fixed",			FXTGT_FIXED_FUNC,	FXTGT_FIXED_FUNC,	1.f,	0.f,	false		},
	{	_FCC("SM1X"),	GS_SETTINGS_LOW,			"9_low",			FXTGT_SM_11,		FXTGT_SM_30,		1.f,	0.f,	false		},
	{	_FCC("SM2L"),	GS_SETTINGS_MEDIUM,			"9_medium",			FXTGT_SM_20_LOW,	FXTGT_SM_30,		1.f,	0.f,	false		},
	{	_FCC("SM2H"),	GS_SETTINGS_HIGH,			"9_high",			FXTGT_SM_20_HIGH,	FXTGT_SM_30,		1.f,	0.f,	false		},
	{	_FCC("SM30"),	GS_SETTINGS_VERYHIGH,		"9_veryhigh",		FXTGT_SM_30,		FXTGT_SM_30,		1.f,	0.f,	false		},
	{	_FCC("SM4H"),	GS_SETTINGS_VERYHIGH,		"10_veryhigh",		FXTGT_SM_40,		FXTGT_SM_40,		1.f,	0.f,	false		},
	{	_FCC("SM4E"),	GS_SETTINGS_EXTREME,		"10_extreme",		FXTGT_SM_40,		FXTGT_SM_40,		1.f,	0.f,	true		},
};
#undef _FCC

//-----------------------------------------------------------------------------

class FeatureLine_Shader : public FeatureLine
{
public:
	typedef FeatureLine_Shader this_type;

	FeatureLine_Shader(void);
	virtual ~FeatureLine_Shader(void);

	virtual bool EnumerateStops(unsigned nIndex, DWORD & nNameOut) override;
	virtual PRESULT MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState) override;
	virtual PRESULT SelectStop(DWORD nStopName, OptionState * pState) override;

	bool EnumerateStops(unsigned nIndex, DWORD & nNameOut) const;
	PRESULT GetStopIndex(DWORD nStopName, unsigned & nIndexOut) const;
};

//-----------------------------------------------------------------------------

bool FeatureLine_Shader::EnumerateStops(unsigned nIndex, DWORD & nNameOut) const
{
	if (nIndex < _countof(s_Stops))
	{
		nNameOut = s_Stops[nIndex].nName;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

bool FeatureLine_Shader::EnumerateStops(unsigned nIndex, DWORD & nNameOut)
{
	return static_cast<const this_type *>(this)->EnumerateStops(nIndex, nNameOut);
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_Shader::GetStopIndex(DWORD nStopName, unsigned & nIndexOut) const
{
	DWORD nName;
	for (unsigned n = 0; EnumerateStops(n, nName); ++n)
	{
		if (nName == nStopName)
		{
			nIndexOut = n;
			return S_OK;
		}
	}
	return E_NOT_FOUND;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_Shader::MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState)
{
	if ( e_IsNoRender() )
		return S_FALSE;

	// Check initDB?

	EFFECT_TARGET eMaxTarget = dxC_GetEffectTargetFromShaderVersions( dxC_CapsGetMaxVertexShaderVersion(), dxC_CapsGetMaxPixelShaderVersion() );

	ASSERT_RETFAIL( dxC_ShouldUsePixomatic() || eMaxTarget != FXTGT_FIXED_FUNC );

	for ( int nIndex = _countof(s_Stops) - 1; nIndex >= 0; nIndex-- )
	{
		if ( s_Stops[nIndex].nMaxEffectTarget > eMaxTarget )
			continue;
		if ( s_Stops[nIndex].nMaxCompatibleEffectTarget < eMaxTarget )
			continue;
		V_RETURN( e_FeatureLine_AddChoice( pMap, nLineName, s_Stops[nIndex].nName, s_Stops[nIndex].szName, s_Stops[nIndex].eGlobalStr ) );
	}
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLine_Shader::SelectStop(DWORD nStopName, OptionState * pState)
{
	// Get the index.
	unsigned nIndex;
	V_RETURN(GetStopIndex(nStopName, nIndex));
	pState->SuspendUpdate();
	pState->set_nMaxEffectTarget(s_Stops[nIndex].nMaxEffectTarget);
	pState->set_fEffectLODBias(s_Stops[nIndex].fEffectLODBias);
	pState->set_fEffectLODScale(s_Stops[nIndex].fEffectLODScale);
	pState->set_bDX10ScreenFX(s_Stops[nIndex].bDX10ScreenFX);
	pState->ResumeUpdate();
	return S_OK;
}

//-----------------------------------------------------------------------------

FeatureLine_Shader::FeatureLine_Shader(void)
{
}

//-----------------------------------------------------------------------------

FeatureLine_Shader::~FeatureLine_Shader(void)
{
}

//-----------------------------------------------------------------------------

};	// anonymous namespace

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_Shader_Init(void)
{
	CComPtr<FeatureLine> pLine(new FeatureLine_Shader);
	return e_FeatureLine_Register(pLine, E_FEATURELINE_FOURCC("SHDR"));
}
