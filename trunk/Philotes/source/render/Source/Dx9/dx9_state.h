//----------------------------------------------------------------------------
// dx9_state.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_STATE__
#define __DX9_STATE__


#include "boundingbox.h"
#include "dxC_effect.h"
#include "dxC_texture.h"
#include "dxC_pixo.h"

using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// CLASSES
//--------------------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

struct D3D_MESH_DEFINITION;
struct D3D_MODEL_DEFINITION;
struct D3D_MODEL;

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern D3DMATERIAL9 gStubMtrl;
extern D3DLIGHT9 gStubLight;

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

/*void dx9_CleanupDebugRender ( int nOldRenderTarget );*/ //This is not useful
void dx9_PrepareDebugRender (
	MATRIX *pmatWorld,
	MATRIX *pmatView,
	MATRIX *pmatProj,
	VECTOR *pvEye,
	int * pnOldRenderTarget,
	SPD3DCSHADERRESOURCEVIEW pTexture = NULL );

void dx9_ResetVertDeclFVFCache();
const D3DC_INPUT_ELEMENT_DESC * dxC_GetVertexElements( VERTEX_DECL_TYPE eType, const struct D3D_EFFECT * pEffect = NULL );




//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline PRESULT dxC_SetRenderState( D3DRENDERSTATETYPE tState, DWORD dwValue, BOOL bForce = FALSE )
{
	if ( dxC_IsPixomaticActive() )
		return dxC_PixoSetRenderState( tState, dwValue );

	PRESULT pr = S_OK;
	if ( gpStateManager )
	{
		if ( bForce )
			gpStateManager->DirtyCachedRenderState( tState );
		V_SAVE( pr, gpStateManager->SetRenderState( tState, dwValue ) );
	}
	else
	{
		if ( ! dxC_IsPixomaticActive() )
		{
			V_SAVE( pr, dxC_GetDevice()->SetRenderState( tState, dwValue ) );
		}
	}

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetLight( DWORD dwIndex, const D3DLIGHT9 * pLight )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
		V_SAVE( pr, gpStateManager->SetLight( dwIndex, pLight ) )
	else
		V_SAVE( pr, dxC_GetDevice()->SetLight( dwIndex, pLight ) )

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetMaterial( const D3DMATERIAL9 * pMaterial )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
		V_SAVE( pr, gpStateManager->SetMaterial( pMaterial ) )
	else
		V_SAVE( pr, dxC_GetDevice()->SetMaterial( pMaterial ) )

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetNPatchMode( float fNumSegments )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
		V_SAVE( pr, gpStateManager->SetNPatchMode( fNumSegments ) )
	else
		V_SAVE( pr, dxC_GetDevice()->SetNPatchMode( fNumSegments ) )

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetPixelShader( LPDIRECT3DPIXELSHADER9 pShader )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
		V_SAVE( pr, gpStateManager->SetPixelShader( pShader ) )
	else
	{
		if ( ! dxC_IsPixomaticActive() )
		{
			V_SAVE( pr, dxC_GetDevice()->SetPixelShader( pShader ) )
		}
	}

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetPixelShaderConstantB( UINT nIndex, const BOOL * pConstantData, UINT nCount )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
		V_SAVE( pr, gpStateManager->SetPixelShaderConstantB( nIndex, pConstantData, nCount ) )
	else
		V_SAVE( pr, dxC_GetDevice()->SetPixelShaderConstantB( nIndex, pConstantData, nCount ) )

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetPixelShaderConstantF( UINT nIndex, const FLOAT * pConstantData, UINT nCount )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
		V_SAVE( pr, gpStateManager->SetPixelShaderConstantF( nIndex, pConstantData, nCount ) )
	else
		V_SAVE( pr, dxC_GetDevice()->SetPixelShaderConstantF( nIndex, pConstantData, nCount ) )

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetPixelShaderConstantI( UINT nIndex, const INT * pConstantData, UINT nCount )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
		V_SAVE( pr, gpStateManager->SetPixelShaderConstantI( nIndex, pConstantData, nCount ) )
	else
		V_SAVE( pr, dxC_GetDevice()->SetPixelShaderConstantI( nIndex, pConstantData, nCount ) )

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetSamplerState( DWORD dwSampler, D3DSAMPLERSTATETYPE tType, DWORD dwValue, BOOL bForce = FALSE )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
	{
		if ( bForce )
			gpStateManager->DirtyCachedSamplerState( dwSampler, tType );
		V_SAVE( pr, gpStateManager->SetSamplerState( dwSampler, tType, dwValue ) );
	}
	else
	{
		V_SAVE( pr, dxC_GetDevice()->SetSamplerState( dwSampler, tType, dwValue ) );
	}

	// If we are enabling anisotropic filtering, set the max anisotropy for this sampler at the same time.
	if ( ( tType == D3DSAMP_MINFILTER || tType == D3DSAMP_MAGFILTER || tType == D3DSAMP_MIPFILTER ) 
		&& dwValue == D3DTEXF_ANISOTROPIC )
	{
		V( dx9_SetSamplerState( dwSampler, D3DSAMP_MAXANISOTROPY, dx9_CapGetValue( DX9CAP_ANISOTROPIC_FILTERING), bForce ) );
	}

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetTextureStageState( DWORD dwStage, D3DTEXTURESTAGESTATETYPE tType, DWORD dwValue, BOOL bForce = FALSE )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
	{
		if ( bForce )
			gpStateManager->DirtyCachedTextureStageState( dwStage, tType );
		V_SAVE( pr, gpStateManager->SetTextureStageState( dwStage, tType, dwValue ) );
	}
	else
	{
		if ( ! dxC_IsPixomaticActive() )
		{
			V_SAVE( pr, dxC_GetDevice()->SetTextureStageState( dwStage, tType, dwValue ) );
		}
	}

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetVertexShader( LPDIRECT3DVERTEXSHADER9 pShader )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
		V_SAVE( pr, gpStateManager->SetVertexShader( pShader ) )
	else
	{
		if ( ! dxC_IsPixomaticActive() )
		{
			V_SAVE( pr, dxC_GetDevice()->SetVertexShader( pShader ) )
		}
	}

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetVertexShaderConstantB( UINT nIndex, const BOOL * pConstantData, UINT nCount )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
		V_SAVE( pr, gpStateManager->SetVertexShaderConstantB( nIndex, pConstantData, nCount ) )
	else
		V_SAVE( pr, dxC_GetDevice()->SetVertexShaderConstantB( nIndex, pConstantData, nCount ) )

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetVertexShaderConstantF( UINT nIndex, const FLOAT * pConstantData, UINT nCount )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
		V_SAVE( pr, gpStateManager->SetVertexShaderConstantF( nIndex, pConstantData, nCount ) )
	else
		V_SAVE( pr, dxC_GetDevice()->SetVertexShaderConstantF( nIndex, pConstantData, nCount ) )

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_SetVertexShaderConstantI( UINT nIndex, const INT * pConstantData, UINT nCount )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
		V_SAVE( pr, gpStateManager->SetVertexShaderConstantI( nIndex, pConstantData, nCount ) )
	else
		V_SAVE( pr, dxC_GetDevice()->SetVertexShaderConstantI( nIndex, pConstantData, nCount ) )

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

inline PRESULT dx9_LightEnable( DWORD dwIndex, BOOL bEnable )
{
	PRESULT pr = S_OK;
	if ( gpStateManager )
		V_SAVE( pr, gpStateManager->LightEnable( dwIndex, bEnable ) )
	else
		V_SAVE( pr, dxC_GetDevice()->LightEnable( dwIndex, bEnable ) )

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return pr;
}

#endif // __DX9_STATE__
