//----------------------------------------------------------------------------
// dxC_state_defaults.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "perfhier.h"
//#include "dxC_statemanager.h"
#include "dxC_state.h"
#include "dxC_state_defaults.h"
#include "dxC_profile.h"
#include "dxC_settings.h"
#include "dxC_caps.h"
#include "e_optionstate.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

// fallthrough
#ifdef ENGINE_TARGET_DX9
	#define D3DTADDRESS_NONE				D3DTADDRESS_WRAP
#elif defined(ENGINE_TARGET_DX10)
	#define D3D10_TEXTURE_ADDRESS_NONE		D3D10_TEXTURE_ADDRESS_WRAP
#endif

#ifdef ENGINE_TARGET_DX9
#define DEFINE_SAMPLER( enumm, type, minf, magf, mipf, addru, addrv, addrw, srgb ) \
	{	SAMPLER_STATE_DEFAULT tTemp = { (DXCRESOURCE_TYPE)D3DRTYPE_##type, D3DTEXF_##minf, D3DTEXF_##magf, D3DTEXF_##mipf, D3DTADDRESS_##addru, D3DTADDRESS_##addrv, D3DTADDRESS_##addrw, srgb }; \
		gtSamplerStateDefaults[ enumm ] = tTemp; }
#elif defined(ENGINE_TARGET_DX10)
#define DEFINE_SAMPLER( enumm, type, minf, magf, mipf, addru, addrv, addrw, srgb ) \
	{	SAMPLER_STATE_DEFAULT tTemp = { (DXCRESOURCE_TYPE)D3DRTYPE_##type, D3DTEXF_##minf, D3DTEXF_##magf, D3DTEXF_##mipf, D3D10_TEXTURE_ADDRESS_##addru, D3D10_TEXTURE_ADDRESS_##addrv, D3D10_TEXTURE_ADDRESS_##addrw, srgb }; \
		gtSamplerStateDefaults[ enumm ] = tTemp; }
#endif

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

SAMPLER_STATE_DEFAULT gtSamplerStateDefaults[ NUM_SAMPLER_TYPES ];

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_SetSamplerDefaults( SAMPLER_TYPE eType, BOOL bForce )
{
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	ASSERT_RETFAIL((0 <= eType) && (eType < NUM_SAMPLER_TYPES));	// CHB 2007.02.16
#ifdef ENGINE_TARGET_DX9
	V( dx9_SetSamplerState( eType, D3DSAMP_MINFILTER, gtSamplerStateDefaults[ eType ].eMinFilter, bForce ) );
	V( dx9_SetSamplerState( eType, D3DSAMP_MAGFILTER, gtSamplerStateDefaults[ eType ].eMagFilter, bForce ) );
	V( dx9_SetSamplerState( eType, D3DSAMP_MIPFILTER, gtSamplerStateDefaults[ eType ].eMipFilter, bForce ) );
	V( dx9_SetSamplerState( eType, D3DSAMP_ADDRESSU, gtSamplerStateDefaults[ eType ].eAddressU, bForce ) );
	V( dx9_SetSamplerState( eType, D3DSAMP_ADDRESSV, gtSamplerStateDefaults[ eType ].eAddressV, bForce ) );
	if ( gtSamplerStateDefaults[ eType ].eType == D3DRTYPE_CUBETEXTURE ||
		 gtSamplerStateDefaults[ eType ].eType == D3DRTYPE_VOLUMETEXTURE )
	{
		V( dx9_SetSamplerState( eType, D3DSAMP_ADDRESSW, gtSamplerStateDefaults[ eType ].eAddressW, bForce ) );
	}

	BOOL bSRGB = FALSE;
	if ( e_TexturesReadSRGB() )
		bSRGB = gtSamplerStateDefaults[ eType ].bSRGB;
	V( dx9_SetSamplerState( eType, D3DSAMP_SRGBTEXTURE, bSRGB ) );

#elif defined( ENGINE_TARGET_DX10 )
	D3D10_FILTER tFilter = D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	UINT nMaxAnisotropy = 1;

	if ( gtSamplerStateDefaults[ eType ].eMinFilter == D3DTEXF_ANISOTROPIC )
	{
		tFilter = D3D10_FILTER_ANISOTROPIC;
		nMaxAnisotropy = 16;
	}
	else if ( gtSamplerStateDefaults[ eType ].eMipFilter == D3DTEXF_LINEAR )
	{
		tFilter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
	}

	dx10_SetSamplerState( eType, tFilter, gtSamplerStateDefaults[ eType ].eAddressU, gtSamplerStateDefaults[ eType ].eAddressV, gtSamplerStateDefaults[ eType ].eAddressW, nMaxAnisotropy );
#endif
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_DefaultStatesInit()
{
	// default sampler states
	//					enum						type			min			mag			mip			addrU		addrV		addrW	sRGB
	DEFINE_SAMPLER(		SAMPLER_DIFFUSE,			TEXTURE,		LINEAR,		LINEAR,		POINT,		WRAP,		WRAP,		NONE,	TRUE	);
	DEFINE_SAMPLER(		SAMPLER_DIFFUSE2,			TEXTURE,		LINEAR,		LINEAR,		POINT,		CLAMP,		CLAMP,		NONE,	TRUE	);
	DEFINE_SAMPLER(		SAMPLER_LIGHTMAP,			TEXTURE,		LINEAR,		LINEAR,		POINT,		CLAMP,		CLAMP,		NONE,	FALSE	);
	DEFINE_SAMPLER(		SAMPLER_SELFILLUM,			TEXTURE,		LINEAR,		LINEAR,		POINT,		WRAP,		WRAP,		NONE,	TRUE	);
	DEFINE_SAMPLER(		SAMPLER_SPECULAR,			TEXTURE,		LINEAR,		LINEAR,		POINT,		WRAP,		WRAP,		NONE,	TRUE	);
	DEFINE_SAMPLER(		SAMPLER_NORMAL,				TEXTURE,		LINEAR,		LINEAR,		POINT,		WRAP,		WRAP,		NONE,	FALSE	);
	DEFINE_SAMPLER(		SAMPLER_CUBEENVMAP,			CUBETEXTURE,	LINEAR,		LINEAR,		POINT,		WRAP,		WRAP,		WRAP,	TRUE	);
	DEFINE_SAMPLER(		SAMPLER_SHADOW,				TEXTURE,		LINEAR,		LINEAR,		POINT,		CLAMP,		CLAMP,		NONE,	FALSE	);
	DEFINE_SAMPLER(		SAMPLER_SPECLOOKUP,			TEXTURE,		LINEAR,		LINEAR,		POINT,		CLAMP,		CLAMP,		NONE,	FALSE	);
	DEFINE_SAMPLER(		SAMPLER_SPHEREENVMAP,		TEXTURE,		LINEAR,		LINEAR,		POINT,		WRAP,		WRAP,		NONE,	TRUE	);
	DEFINE_SAMPLER(		SAMPLER_SHADOWDEPTH,		TEXTURE,		LINEAR,		LINEAR,		POINT,		CLAMP,		CLAMP,		NONE,	FALSE	);
	DEFINE_SAMPLER(		SAMPLER_PARTICLE_LIGHTMAP,	TEXTURE,		LINEAR,		LINEAR,		POINT,		CLAMP,		CLAMP,		NONE,	TRUE	);
	DEFINE_SAMPLER(		SAMPLER_DIFFUSE_RAIN,		TEXTURE,		LINEAR,		LINEAR,		POINT,		WRAP,		WRAP,		NONE,	TRUE	);
	DEFINE_SAMPLER(		SAMPLER_BUMP_RAIN,			TEXTURE,		LINEAR,		LINEAR,		POINT,		MIRROR,		MIRROR,		NONE,	FALSE	);
	DEFINE_SAMPLER(		SAMPLER_DX10_FULLSCREEN,	TEXTURE,		LINEAR,		LINEAR,		POINT,		CLAMP,		CLAMP,		NONE,	TRUE	);	// CHB 2007.08.30 - Not sure if these are the most appropriate defaults.

	// if supports anisotropic and it's activated, set it up!
	if ( dx9_CapGetValue( DX9CAP_ANISOTROPIC_FILTERING ) && e_OptionState_GetActive()->get_bAnisotropic() )
	{
		gtSamplerStateDefaults[ SAMPLER_DIFFUSE   ].eMinFilter = (D3DC_FILTER) D3DTEXF_ANISOTROPIC;
		gtSamplerStateDefaults[ SAMPLER_DIFFUSE2  ].eMinFilter = (D3DC_FILTER) D3DTEXF_ANISOTROPIC;
		gtSamplerStateDefaults[ SAMPLER_LIGHTMAP  ].eMinFilter = (D3DC_FILTER) D3DTEXF_ANISOTROPIC;
		gtSamplerStateDefaults[ SAMPLER_SELFILLUM ].eMinFilter = (D3DC_FILTER) D3DTEXF_ANISOTROPIC;
		gtSamplerStateDefaults[ SAMPLER_SPECULAR  ].eMinFilter = (D3DC_FILTER) D3DTEXF_ANISOTROPIC;
		gtSamplerStateDefaults[ SAMPLER_NORMAL	  ].eMinFilter = (D3DC_FILTER) D3DTEXF_ANISOTROPIC;
	}

	// if supports trilinear and it's activated, set it up!
	if ( dx9_CapGetValue( DX9CAP_TRILINEAR_FILTERING ) && e_OptionState_GetActive()->get_bTrilinear() )
	{
		gtSamplerStateDefaults[ SAMPLER_DIFFUSE				].eMipFilter = (D3DC_FILTER) D3DTEXF_LINEAR;
		gtSamplerStateDefaults[ SAMPLER_DIFFUSE2			].eMipFilter = (D3DC_FILTER) D3DTEXF_LINEAR;
		gtSamplerStateDefaults[ SAMPLER_LIGHTMAP			].eMipFilter = (D3DC_FILTER) D3DTEXF_LINEAR;
		gtSamplerStateDefaults[ SAMPLER_SELFILLUM			].eMipFilter = (D3DC_FILTER) D3DTEXF_LINEAR;
		gtSamplerStateDefaults[ SAMPLER_SPECULAR			].eMipFilter = (D3DC_FILTER) D3DTEXF_LINEAR;
		gtSamplerStateDefaults[ SAMPLER_NORMAL				].eMipFilter = (D3DC_FILTER) D3DTEXF_LINEAR;
		gtSamplerStateDefaults[ SAMPLER_CUBEENVMAP			].eMipFilter = (D3DC_FILTER) D3DTEXF_LINEAR;
		gtSamplerStateDefaults[ SAMPLER_PARTICLE_LIGHTMAP	].eMipFilter = (D3DC_FILTER) D3DTEXF_LINEAR;
		gtSamplerStateDefaults[ SAMPLER_DIFFUSE_RAIN		].eMipFilter = (D3DC_FILTER) D3DTEXF_LINEAR;
		gtSamplerStateDefaults[ SAMPLER_BUMP_RAIN			].eMipFilter = (D3DC_FILTER) D3DTEXF_LINEAR;
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_DefaultStatesDestroy()
{
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_ResetSamplerStates( BOOL bForce )
{
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	D3D_PROFILE_REGION( L"Reset Sampler States" );

	for ( int i = 0; i < NUM_SAMPLER_TYPES; i++ )
	{
		V( dxC_SetSamplerDefaults( (SAMPLER_TYPE)i, bForce ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_SetDiffuse2SamplerStates( D3DC_TEXTURE_ADDRESS_MODE eAddressU, D3DC_TEXTURE_ADDRESS_MODE eAddressV, BOOL bForce )
{
	ASSERT_RETINVALIDARG( eAddressU > 0 && eAddressU <= 5 );
	ASSERT_RETINVALIDARG( eAddressV > 0 && eAddressV <= 5 );
#ifdef ENGINE_TARGET_DX9
	V( dx9_SetSamplerState( SAMPLER_DIFFUSE2, D3DSAMP_ADDRESSU, eAddressU, bForce ) );
	V( dx9_SetSamplerState( SAMPLER_DIFFUSE2, D3DSAMP_ADDRESSV, eAddressV, bForce ) );
#elif defined( ENGINE_TARGET_DX10 )
	D3D10_FILTER tFilter = D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	UINT nMaxAnisotropy = 1;

	if ( gtSamplerStateDefaults[ SAMPLER_DIFFUSE2 ].eMinFilter == D3DTEXF_ANISOTROPIC )
	{
		tFilter = D3D10_FILTER_ANISOTROPIC;
		nMaxAnisotropy = 16;
	}
	else if ( gtSamplerStateDefaults[ SAMPLER_DIFFUSE2 ].eMipFilter == D3DTEXF_LINEAR )
	{
		tFilter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
	}
	dx10_SetSamplerState( SAMPLER_DIFFUSE2, tFilter, eAddressU, eAddressV, gtSamplerStateDefaults[ SAMPLER_DIFFUSE2 ].eAddressW, nMaxAnisotropy );
#endif

	return S_OK;
}
