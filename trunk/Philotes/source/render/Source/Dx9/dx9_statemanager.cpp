//----------------------------------------------------------------------------
// dx9_statemanager.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dx9_common.h"
#include "dxC_caps.h"
#include "dxC_target.h"
#include "dxC_model.h"
#include "dxC_texture.h"
#include "e_texture_priv.h"
#include "dxC_effect.h"
#include "dxC_profile.h"
#include "dxC_material.h"
#include "log.h"
#include "perfhier.h"
#include "dxC_state.h"
#include "dxC_state_defaults.h"

#include "dxC_statemanager.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

CStateManagerInterface *		gpStateManager = NULL;
STATE_MANAGER_TYPE				geStateManagerType = STATE_MANAGER_INVALID;
int								gnStateStatsTotalCalls = 0;
int								gnStateStatsNonEffectCalls = 0;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------



//--------------------------------------------------------------------------------------
// Base implementation of a custom ID3DXEffectStateManager interface
// This implementation does nothing more than forward all state change commands to the
// appropriate D3D handler.
// An interface that implements custom state change handling may be derived from this
// (such as statistical collection, or filtering of redundant state change commands for
// a subset of states)
//--------------------------------------------------------------------------------------

class CBaseStateManager : public CStateManagerInterface
{
protected:
    LONG              m_lRef;
    UINT              m_nTotalStateChanges;
    UINT              m_nTotalStateChangesPerFrame;
	UINT              m_nTotalStateChangesPerType[ NUM_CACHEABLES ];
	UINT              m_nActualStateChangesPerType[ NUM_CACHEABLES ];
    WCHAR             m_wszFrameStats[EFFECT_STATE_MANAGER_STATS_LEN];
public:
    //CBaseStateManager( )
    //    : m_lRef( 1UL ),
    //      m_nTotalStateChanges( 0 ),
    //      m_nTotalStateChangesPerFrame( 0 )
    //{
    //    m_wszFrameStats[ 0 ] = 0;
    //}

	virtual void Init()
	{
		m_lRef = 1UL;
		m_nTotalStateChanges = 0;
		m_nTotalStateChangesPerFrame = 0;
	}

    // Must be invoked by the application anytime it allows state to be
    // changed outside of the D3DX Effect system.
    // An entry-point for this should be provided if implementing custom filtering of redundant
    // state changes.
    virtual void DirtyCachedValues()
    {
    }

	virtual void ResetStats()
	{
		m_nTotalStateChanges = 0;
		ZeroMemory( m_nTotalStateChangesPerType, sizeof(m_nTotalStateChangesPerType) );
		ZeroMemory( m_nActualStateChangesPerType, sizeof(m_nActualStateChangesPerType) );
	}

    virtual LPWSTR EndFrameStats()
    {
        if ( m_nTotalStateChangesPerFrame != m_nTotalStateChanges )
        {
			PStrPrintf( m_wszFrameStats, 
                        EFFECT_STATE_MANAGER_STATS_LEN,
						L"D3D States Stats:\nTotal calls: %6u  Passed to D3D: %6u",
                        m_nTotalStateChanges, m_nTotalStateChanges );

			dx9_AddStatesTypeStats( m_wszFrameStats, m_nTotalStateChangesPerType, m_nActualStateChangesPerType );
			dx9_AddStatesNonEffectStats( m_wszFrameStats, (UINT)gnStateStatsNonEffectCalls );

            m_wszFrameStats[ EFFECT_STATE_MANAGER_STATS_LEN - 1 ] = 0;
            m_nTotalStateChangesPerFrame = m_nTotalStateChanges;
        }

        return m_wszFrameStats;
    }

	void GetStateStats( STATE_SET_TYPE eStat, UINT & nActual, UINT & nTotal )
	{
		ASSERT_RETURN( IS_VALID_INDEX( eStat, NUM_CACHEABLES ) );
		nActual = m_nActualStateChangesPerType[ eStat ];
		nTotal  = m_nTotalStateChangesPerType[ eStat ];
	}

    // methods inherited from ID3DXEffectStateManager
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv)
    {
        if (iid == IID_IUnknown || iid == IID_ID3DXEffectStateManager)
        {
            *ppv = static_cast<ID3DXEffectStateManager*>(this);
        } 
        else
        {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
        
        reinterpret_cast<IUnknown*>(this)->AddRef();
        return S_OK;
    }
    STDMETHOD_(ULONG, AddRef)(THIS)
    {
        return (ULONG)InterlockedIncrement( &m_lRef );
    }
    STDMETHOD_(ULONG, Release)(THIS)
    {
        if ( 0L == InterlockedDecrement( &m_lRef ) )
        {
            //delete this;
			FREE( NULL, this );
            return 0L;
        }

        return m_lRef;
    }
    STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE d3dRenderState, DWORD dwValue )
    {
		//HITCOUNT(DRAW_STATES)
		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_RENDER_STATE ]++;
		m_nActualStateChangesPerType[ STATE_SET_RENDER_STATE ]++;
        return dxC_GetDevice()->SetRenderState( d3dRenderState, dwValue );
    }
    STDMETHOD(SetSamplerState)(THIS_ DWORD dwStage, D3DSAMPLERSTATETYPE d3dSamplerState, DWORD dwValue )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_SAMPLER_STATE ]++;
		m_nActualStateChangesPerType[ STATE_SET_SAMPLER_STATE ]++;
		return dxC_GetDevice()->SetSamplerState( dwStage, d3dSamplerState, dwValue );
    }
    STDMETHOD(SetTextureStageState)(THIS_ DWORD dwStage, D3DTEXTURESTAGESTATETYPE d3dTextureStageState, DWORD dwValue )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_TEXTURE_STAGE_STATE ]++;
		m_nActualStateChangesPerType[ STATE_SET_TEXTURE_STAGE_STATE ]++;
		return dxC_GetDevice()->SetTextureStageState( dwStage, d3dTextureStageState, dwValue );
    }
    STDMETHOD(SetTexture)(THIS_ DWORD dwStage, LPD3DCBASETEXTURE pTexture )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_TEXTURE ]++;
		m_nActualStateChangesPerType[ STATE_SET_TEXTURE ]++;
		return dxC_GetDevice()->SetTexture( dwStage, pTexture );
    }
    STDMETHOD(SetVertexShader)(THIS_ LPDIRECT3DVERTEXSHADER9 pShader )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_VERTEX_SHADER ]++;
		m_nActualStateChangesPerType[ STATE_SET_VERTEX_SHADER ]++;
		return dxC_GetDevice()->SetVertexShader( pShader );
    }
    STDMETHOD(SetPixelShader)(THIS_ LPDIRECT3DPIXELSHADER9 pShader )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_PIXEL_SHADER ]++;
		m_nActualStateChangesPerType[ STATE_SET_PIXEL_SHADER ]++;
		return dxC_GetDevice()->SetPixelShader( pShader );
    }
    STDMETHOD(SetFVF)(THIS_ DWORD dwFVF )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_FVF ]++;
		m_nActualStateChangesPerType[ STATE_SET_FVF ]++;
		return dxC_GetDevice()->SetFVF( dwFVF );
    }
    STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX *pMatrix )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_TRANSFORM ]++;
		m_nActualStateChangesPerType[ STATE_SET_TRANSFORM ]++;
		return dxC_GetDevice()->SetTransform( State, pMatrix );
    }
    STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9 *pMaterial )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_MATERIAL ]++;
		m_nActualStateChangesPerType[ STATE_SET_MATERIAL ]++;
		return dxC_GetDevice()->SetMaterial( pMaterial );
    }
    STDMETHOD(SetLight)(THIS_ DWORD Index, CONST D3DLIGHT9 *pLight )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_LIGHT ]++;
		m_nActualStateChangesPerType[ STATE_SET_LIGHT ]++;
		return dxC_GetDevice()->SetLight( Index, pLight );
    }
    STDMETHOD(LightEnable)(THIS_ DWORD Index, BOOL Enable )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_LIGHT_ENABLE ]++;
		m_nActualStateChangesPerType[ STATE_LIGHT_ENABLE ]++;
		return dxC_GetDevice()->LightEnable( Index, Enable );
    }
    STDMETHOD(SetNPatchMode)(THIS_ FLOAT NumSegments )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_NPATCH_MODE ]++;
		m_nActualStateChangesPerType[ STATE_SET_NPATCH_MODE ]++;
		return dxC_GetDevice()->SetNPatchMode( NumSegments );
    }
    STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT RegisterIndex,
                                        CONST FLOAT *pConstantData,
                                        UINT RegisterCount )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_VERTEX_SHADER_CONSTANT_F ]++;
		m_nActualStateChangesPerType[ STATE_SET_VERTEX_SHADER_CONSTANT_F ]++;
		return dxC_GetDevice()->SetVertexShaderConstantF( RegisterIndex,
														  pConstantData,
														  RegisterCount );
    }
    STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT RegisterIndex,
                                        CONST INT *pConstantData,
                                        UINT RegisterCount )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_VERTEX_SHADER_CONSTANT_I ]++;
		m_nActualStateChangesPerType[ STATE_SET_VERTEX_SHADER_CONSTANT_I ]++;
		return dxC_GetDevice()->SetVertexShaderConstantI( RegisterIndex,
														  pConstantData,
														  RegisterCount );
    }
    STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT RegisterIndex,
                                        CONST BOOL *pConstantData,
                                        UINT RegisterCount )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_VERTEX_SHADER_CONSTANT_B ]++;
		m_nActualStateChangesPerType[ STATE_SET_VERTEX_SHADER_CONSTANT_B ]++;
		return dxC_GetDevice()->SetVertexShaderConstantB( RegisterIndex,
														  pConstantData,
														  RegisterCount );
    }
    STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT RegisterIndex,
                                       CONST FLOAT *pConstantData,
                                       UINT RegisterCount )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_PIXEL_SHADER_CONSTANT_F ]++;
		m_nActualStateChangesPerType[ STATE_SET_PIXEL_SHADER_CONSTANT_F ]++;
		return dxC_GetDevice()->SetPixelShaderConstantF( RegisterIndex,
														 pConstantData,
														 RegisterCount );
    }
    STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT RegisterIndex,
                                       CONST INT *pConstantData,
                                       UINT RegisterCount )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_PIXEL_SHADER_CONSTANT_I ]++;
		m_nActualStateChangesPerType[ STATE_SET_PIXEL_SHADER_CONSTANT_I ]++;
		return dxC_GetDevice()->SetPixelShaderConstantI( RegisterIndex,
														 pConstantData,
														 RegisterCount );
    }
    STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT RegisterIndex,
                                       CONST BOOL *pConstantData,
                                       UINT RegisterCount )
    {
		//HITCOUNT(DRAW_STATES)
        m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_VERTEX_SHADER_CONSTANT_B ]++;
		m_nActualStateChangesPerType[ STATE_SET_VERTEX_SHADER_CONSTANT_B ]++;
		return dxC_GetDevice()->SetPixelShaderConstantB( RegisterIndex,
														 pConstantData,
														 RegisterCount );
    }
	STDMETHOD(SetVertexDeclaration)( LPD3DCIALAYOUT pVertDecl )
	{
		//HITCOUNT(DRAW_STATES)
		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_VERTEX_DECLARATION ]++;
		m_nActualStateChangesPerType[ STATE_SET_VERTEX_DECLARATION ]++;
		return dxC_GetDevice()->SetVertexDeclaration( pVertDecl );
	}
	STDMETHOD(SetStreamSource)( UINT StreamNumber, LPD3DCVB pStreamData, UINT OffsetInBytes, UINT Stride )
	{
		//HITCOUNT(DRAW_STATES)
		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_STREAM_SOURCE ]++;
		m_nActualStateChangesPerType[ STATE_SET_STREAM_SOURCE ]++;
		return dxC_GetDevice()->SetStreamSource( StreamNumber, pStreamData, OffsetInBytes, Stride );
	}
	STDMETHOD(SetIndices)( LPD3DCIB pIndexData )
	{
		//HITCOUNT(DRAW_STATES)
		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_INDICES ]++;
		m_nActualStateChangesPerType[ STATE_SET_INDICES ]++;
		return dxC_GetDevice()->SetIndices( pIndexData );
	}
};




//--------------------------------------------------------------------------------------
// Implementation of a state manager that filters redundant state change commands.
//--------------------------------------------------------------------------------------

class CPureDeviceStateManager : public CBaseStateManager
{
protected:
    RENDERSTATE_ARRAY				m_tRenderStates;						// cached Render-States
    SAMPLERSTATE_ARRAY				m_ptSamplerStates[ DX9_MAX_SAMPLERS ];	// cached Sampler States
    TEXTURESTATE_ARRAY *			m_ptTextureStates;						// cached Texture Stage States
	TEXTURE_ARRAY					m_tTextures;							// cached Textures
	LPDIRECT3DVERTEXSHADER9			m_pVertexShader;						// cached Vertex Shader
	LPDIRECT3DPIXELSHADER9			m_pPixelShader;							// cached Pixel Shader
	LPD3DCIALAYOUT	m_pVertDecl;							// cached Vertex Declaration
	DWORD							m_dwFVF;								// cached FVF
	VERTEXSTREAM_ARRAY				m_tStreams;								// cached Stream Sources
	LPD3DCIB						m_pIndices;								// cached Indices

    UINT    m_nFilteredStateChanges;                            // Statistics -- # of redundant
                                                                // states actually filtered
    UINT    m_nFilteredStateChangesPerFrame;
	DWORD	m_dwTextureStages;
	DWORD	m_dwVertexStreams;

public:

	void Init( DWORD dwStages, DWORD dwStreams )
	{
		dx9_StateArrayClear( m_tRenderStates );
		dx9_StateArrayClear( m_tTextures );

		for ( int i = 0; i < DX9_MAX_SAMPLERS; i++ )
			dx9_StateArrayClear( m_ptSamplerStates[ i ] );

		m_dwTextureStages = dwStages;
		m_ptTextureStates = (TEXTURESTATE_ARRAY *)MALLOCZ( NULL, sizeof(TEXTURESTATE_ARRAY) * m_dwTextureStages );
		ASSERT_RETURN( m_ptTextureStates );
		for ( int i = 0; (DWORD)i < m_dwTextureStages; i++ )
			dx9_StateArrayClear( m_ptTextureStates[ i ] );

		m_dwVertexStreams = dwStreams;
		dx9_StateArrayClear( m_tStreams );

		m_pVertexShader = NULL;
		m_pPixelShader = NULL;
		m_dwFVF = 0;
		m_pVertDecl = NULL;
		m_pIndices = NULL;

		m_nFilteredStateChanges = 0;
		m_nFilteredStateChangesPerFrame = 0;
	}

	void Destroy()
	{
		if ( m_ptTextureStates )
		{
			FREE( NULL, m_ptTextureStates );
			m_ptTextureStates = NULL;
		}

		// call parent Destroy
		CStateManagerInterface::Destroy();
	}

    virtual LPWSTR EndFrameStats()
    {
        // If either the 'total' state changes or the 'filtered' state changes
        // has changed, re-compute the frame statistics string
        if ( 0 != ( (m_nTotalStateChangesPerFrame    - m_nTotalStateChanges) |
				    (m_nFilteredStateChangesPerFrame - m_nFilteredStateChanges) ) )
        {
            PStrPrintf( m_wszFrameStats, 
                        EFFECT_STATE_MANAGER_STATS_LEN,
                        L"D3D States Stats:\nTotal calls: %6u  Passed to D3D: %6u   Redundants filtered: %d",
                        m_nTotalStateChanges, ( m_nTotalStateChanges - m_nFilteredStateChanges ), m_nFilteredStateChanges );

			dx9_AddStatesTypeStats( m_wszFrameStats, m_nTotalStateChangesPerType, m_nActualStateChangesPerType );
			dx9_AddStatesNonEffectStats( m_wszFrameStats, (UINT)gnStateStatsNonEffectCalls );

            m_wszFrameStats[ EFFECT_STATE_MANAGER_STATS_LEN-1 ] = 0;

            m_nTotalStateChangesPerFrame = m_nTotalStateChanges;
            m_nFilteredStateChangesPerFrame = m_nFilteredStateChanges;
        }

        return m_wszFrameStats;
    }

	// More targeted 'Dirty' commands may be useful.
    void DirtyCachedValues()
    {
		dx9_StateArrayClear( m_tRenderStates );
		dx9_StateArrayClear( m_tTextures );
		for ( int i = 0; i < DX9_MAX_SAMPLERS; i++ )
			dx9_StateArrayClear( m_ptSamplerStates[ i ] );
		ASSERT_RETURN( m_ptTextureStates );
		for ( int i = 0; (DWORD)i < m_dwTextureStages; i++ )
			dx9_StateArrayClear( m_ptTextureStates[ i ] );
		dx9_StateArrayClear( m_tStreams );

		m_pVertexShader = NULL;
		m_pPixelShader = NULL;
		m_pVertDecl = NULL;
		m_dwFVF = 0;
		m_pIndices = NULL;
    }

	void DirtyCachedRenderState( D3DRENDERSTATETYPE d3dRenderState )
	{
		dx9_StateArrayRemove( m_tRenderStates, (int)d3dRenderState );
	}

	void DirtyCachedSamplerState( DWORD dwStage, D3DSAMPLERSTATETYPE d3dSamplerState )
	{
		DWORD dwMaxSamplers = DX9_MAX_SAMPLERS;
		ASSERT_RETURN( dwStage < dwMaxSamplers );
		dx9_StateArrayRemove( m_ptSamplerStates[ dwStage ], (int)d3dSamplerState );
	}

	void DirtyCachedTextureStageState( DWORD dwStage, D3DTEXTURESTAGESTATETYPE d3dTextureStageState )
	{
		ASSERT_RETURN( dwStage < m_dwTextureStages );
		dx9_StateArrayRemove( m_ptTextureStates[ dwStage ], (int)d3dTextureStageState );
	}

	void ResetStats()
	{
		m_nTotalStateChanges = 0;
		m_nFilteredStateChanges = 0;
		ZeroMemory( m_nTotalStateChangesPerType, sizeof(m_nTotalStateChangesPerType) );
		ZeroMemory( m_nActualStateChangesPerType, sizeof(m_nActualStateChangesPerType) );
	}

	// methods inherited from ID3DXEffectStateManager
	STDMETHOD(SetTexture)(THIS_ DWORD Stage, LPD3DCBASETEXTURE pTexture )
	{
		//HITCOUNT(DRAW_STATES)

		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_TEXTURE ]++;

		// Update the cache
		// If the return value is 'true', the command must be forwarded to
		// the D3D Runtime.
		if ( dx9_StateCacheSet( m_tTextures, (int)Stage, pTexture ) )
		{
			m_nActualStateChangesPerType[ STATE_SET_TEXTURE ]++;
			return dxC_GetDevice()->SetTexture( Stage, pTexture );
		}

		m_nFilteredStateChanges++;

		return S_OK;
	}

	STDMETHOD(SetVertexShader)(THIS_ LPDIRECT3DVERTEXSHADER9 pShader )
	{
		//HITCOUNT(DRAW_STATES)
		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_VERTEX_SHADER ]++;

		// Update the cache
		// If the return value is 'true', the command must be forwarded to
		// the D3D Runtime.
		if ( m_pVertexShader != pShader )
		{
			m_pVertexShader = pShader;
			m_nActualStateChangesPerType[ STATE_SET_VERTEX_SHADER ]++;
			return dxC_GetDevice()->SetVertexShader( pShader );
		}

		m_nFilteredStateChanges++;

		return S_OK;
	}

	STDMETHOD(SetPixelShader)(THIS_ LPDIRECT3DPIXELSHADER9 pShader )
	{
		//HITCOUNT(DRAW_STATES)
		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_PIXEL_SHADER ]++;

		// Update the cache
		// If the return value is 'true', the command must be forwarded to
		// the D3D Runtime.
		if ( m_pPixelShader != pShader )
		{
			m_pPixelShader = pShader;
			m_nActualStateChangesPerType[ STATE_SET_PIXEL_SHADER ]++;
			return dxC_GetDevice()->SetPixelShader( pShader );
		}

		m_nFilteredStateChanges++;

		return S_OK;
	}

	STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE d3dRenderState, DWORD dwValue )
    {
		//HITCOUNT(DRAW_STATES)
			
		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_RENDER_STATE ]++;

        // Update the render state cache
        // If the return value is 'true', the command must be forwarded to
        // the D3D Runtime.
		if ( dx9_StateCacheSet( m_tRenderStates, (int)d3dRenderState, dwValue ) )
		{
			m_nActualStateChangesPerType[ STATE_SET_RENDER_STATE ]++;
			return dxC_GetDevice()->SetRenderState( d3dRenderState, dwValue );
		}

		m_nFilteredStateChanges++;

        return S_OK;
    }

    STDMETHOD(SetSamplerState)(THIS_ DWORD dwStage, D3DSAMPLERSTATETYPE d3dSamplerState, DWORD dwValue )
    {
		//HITCOUNT(DRAW_STATES)
	
		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_SAMPLER_STATE ]++;

        // If this dwStage is not cached, pass the value through and exit.
        // Otherwise, update the sampler state cache and if the return value is 'true', the 
        // command must be forwarded to the D3D Runtime.
		DWORD dwMaxSamplers = DX9_MAX_SAMPLERS;
		ASSERT( dwStage < dwMaxSamplers );
		if ( dwStage >= dwMaxSamplers || dx9_StateCacheSet( m_ptSamplerStates[ dwStage ], (int)d3dSamplerState, dwValue ) )
		{
			m_nActualStateChangesPerType[ STATE_SET_SAMPLER_STATE ]++;
            return dxC_GetDevice()->SetSamplerState( dwStage, d3dSamplerState, dwValue );
		}

        m_nFilteredStateChanges++;

        return S_OK;
    }

    STDMETHOD(SetTextureStageState)(THIS_ DWORD dwStage, D3DTEXTURESTAGESTATETYPE d3dTextureStageState, DWORD dwValue )
    {
		//HITCOUNT(DRAW_STATES)

		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_TEXTURE_STAGE_STATE ]++;

        // If this dwStage is not cached, pass the value through and exit.
        // Otherwise, update the texture stage state cache and if the return value is 'true', the 
        // command must be forwarded to the D3D Runtime.
		ASSERT_RETFAIL( dwStage < m_dwTextureStages );
		if ( dx9_StateCacheSet( m_ptTextureStates[ dwStage ], (int)d3dTextureStageState, dwValue ) )
		{
			m_nActualStateChangesPerType[ STATE_SET_TEXTURE_STAGE_STATE ]++;
			return dxC_GetDevice()->SetTextureStageState( dwStage, d3dTextureStageState, dwValue );
		}

		m_nFilteredStateChanges++;

        return S_OK;
    }
	STDMETHOD(SetVertexDeclaration)( LPD3DCIALAYOUT pVertDecl )
	{
		//HITCOUNT(DRAW_STATES)

		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_VERTEX_DECLARATION ]++;

		HRESULT hr = S_OK;
		if ( pVertDecl != m_pVertDecl )
		{
			m_nActualStateChangesPerType[ STATE_SET_VERTEX_DECLARATION ]++;
			hr = dxC_GetDevice()->SetVertexDeclaration( pVertDecl );
			m_pVertDecl = pVertDecl;
			m_dwFVF = 0;
			return hr;
		}

		m_nFilteredStateChanges++;
		return hr;
	}
	STDMETHOD(SetFVF)( DWORD dwFVF )
	{
		//HITCOUNT(DRAW_STATES)

		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_FVF ]++;

		HRESULT hr = S_OK;
		if ( dwFVF != m_dwFVF )
		{
			m_nActualStateChangesPerType[ STATE_SET_FVF ]++;
			hr = dxC_GetDevice()->SetFVF( dwFVF );
			m_pVertDecl = NULL;
			m_dwFVF = dwFVF;
			return hr;
		}

		m_nFilteredStateChanges++;
		return hr;
	}
	STDMETHOD(SetStreamSource)( UINT StreamNumber, LPD3DCVB pStreamData, UINT OffsetInBytes, UINT Stride )
	{
		//HITCOUNT(DRAW_STATES)

		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_STREAM_SOURCE ]++;
		HRESULT hr = S_OK;

		// If this dwStage is not cached, pass the value through and exit.
		// Otherwise, update the cache and if the return value is 'true', the 
		// command must be forwarded to the D3D Runtime.
		//ASSERT_RETFAIL( StreamNumber < m_dwVertexStreams );
		LPD3DCVB pBuffer = pStreamData;
		if ( StreamNumber >= m_dwVertexStreams || dx9_StateCacheSet( m_tStreams, (int)StreamNumber, pBuffer ) )
		{
			m_nActualStateChangesPerType[ STATE_SET_STREAM_SOURCE ]++;
			hr = dxC_GetDevice()->SetStreamSource( StreamNumber, pStreamData, OffsetInBytes, Stride );
			//m_tStreams[ StreamNumber ] = pStreamData;
			return hr;
		}

		m_nFilteredStateChanges++;
		return hr;
	}
	STDMETHOD(SetIndices)( LPD3DCIB pIndexData )
	{
		//HITCOUNT(DRAW_STATES)

		m_nTotalStateChanges++;
		m_nTotalStateChangesPerType[ STATE_SET_INDICES ]++;
		HRESULT hr = S_OK;

		if ( m_pIndices != pIndexData )
		{
			m_nActualStateChangesPerType[ STATE_SET_INDICES ]++;
			hr = dxC_GetDevice()->SetIndices( pIndexData );
			m_pIndices = pIndexData;
			return hr;
		}

		m_nFilteredStateChanges++;
		return hr;
	}

};

//--------------------------------------------------------------------------------------
// Create an extended ID3DXEffectStateManager instance
//--------------------------------------------------------------------------------------
CStateManagerInterface* CStateManagerInterface::Create( STATE_MANAGER_TYPE eType )
{
    CStateManagerInterface* pStateManager = NULL;

	DWORD dwStages = dx9_GetMaxTextureStages();
	DWORD dwStreams = dx9_GetMaxStreams();
	dwStreams = min( dwStreams, D3D_VERTEXSTREAM_COUNT );

	switch ( eType )
	{
	case STATE_MANAGER_TRACK:
		pStateManager = (CBaseStateManager*)MALLOCZ( NULL, sizeof(CBaseStateManager) );
		if ( ! pStateManager )
		{
			V( E_OUTOFMEMORY );
			return NULL;
		}

		// must call placement new (for parent constructors I have no control over)
		new ( pStateManager ) CBaseStateManager;
		((CBaseStateManager*)pStateManager)->Init();
		break;
	case STATE_MANAGER_OPTIMIZE:
		pStateManager = (CPureDeviceStateManager*)MALLOCZ( NULL, sizeof(CPureDeviceStateManager) );
		if ( ! pStateManager )
		{
			V( E_OUTOFMEMORY );
			return NULL;
		}

		// must call placement new (for parent constructors I have no control over)
		new ( pStateManager ) CPureDeviceStateManager;
		((CPureDeviceStateManager*)pStateManager)->Init( dwStages, dwStreams );
		break;
	default:
		ASSERT_MSG( "Failed to create State Manager - Invalid type" );
		return NULL;
	}

	// must be called
	pStateManager->AddRef();

    return pStateManager;
}