//----------------------------------------------------------------------------
// dx10_statemanager.cpp
// Handles DX10 renderstate
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_statemanager.h"
#include "dxC_effect.h"
#include "dxC_state.h"
#include "dxC_state_defaults.h"
#include "pakfile.h"
#include "dx10_effect.h"

#include "dxC_buffer.h"
#include <map>
#define HASH_ON_STATE
//#define DX10_CACHE_SETSTREAMSOURCE
#define MAX_FIXED_FUNC_TEXTURE_STAGES 8

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

CStateManagerInterface *				gpStateManager = NULL;
STATE_MANAGER_TYPE						geStateManagerType = STATE_MANAGER_INVALID;
int										gnStateStatsTotalCalls = 0;
int										gnStateStatsNonEffectCalls = 0;
ID3D10EffectShaderResourceVariable*		gpTextureStages[MAX_FIXED_FUNC_TEXTURE_STAGES];


CComPtr<ID3D10SamplerState>		gppSamplerStates[NUM_SAMPLER_DX10_ENGINE_TOUCHES] = {NULL};


std::map<UINT,ID3D10DepthStencilState*> gmDepthStencilStates;
UINT gnDSHash = 0;

PRESULT dx10_CreateDSState( D3D10_DEPTH_STENCIL_DESC& DSDesc )
{
	//Construct a hash for this particular state
	UINT nHash =  (UINT)DSDesc.DepthEnable;						//Need 1 bit for depth enable:			Next shift is 1
	nHash	  |=  (UINT)DSDesc.DepthWriteMask << 1;				//Need 1 bit for depth write enable:	Next shift is 2
	nHash	  |= ((UINT)DSDesc.DepthFunc) << 2;					//Need 3 bits for comparison mode:		Next shift is 5 

	gnDSHash = nHash;

	if( gmDepthStencilStates[ nHash ] )  //We've seen this before, no need to create
		return S_OK;

	return dxC_GetDevice()->CreateDepthStencilState( &DSDesc, &gmDepthStencilStates[ nHash ] );
}

std::map<UINT64,ID3D10BlendState*> gmBlendStates;
UINT64 gnBSHash = 0;

PRESULT dx10_CreateBlendState( D3D10_BLEND_DESC& blendDesc )
{
	//Construct a hash for this particular state
	UINT64 hash = (UINT64)blendDesc.BlendEnable[0];					//Need 1 bit for blend enable:			Next shift is 1
	hash	 |= (UINT64)blendDesc.BlendOp			<< 1;			//Need 3 bits for BlendOp:				Next shift is 4
	hash     |= (UINT64)blendDesc.BlendOpAlpha	<< 4;				//Need 3 bits for BlendOpAlpha:			Next shift is 7
	hash	 |= (UINT64)blendDesc.SrcBlend		<< 7;				//Need 5 bits for SrcBlend:				Next shift is 12
	hash	 |= (UINT64)blendDesc.SrcBlendAlpha	<< 12;				//Need 5 bits for SrcBlendAlpha:		Next shift is 17
	hash	 |= (UINT64)blendDesc.DestBlend		<< 17;				//Need 5 bits for DestBlend:			Next shift is 22
	hash	 |= (UINT64)blendDesc.DestBlendAlpha	<< 22;			//Need 5 bits for DestBlendAlpha:		Next shift is 27
	hash	 |= (UINT64)blendDesc.RenderTargetWriteMask[0] << 27;	//Need 4 bits for RenderTargetWriteMask:Next shift is 31
	hash	 |= (UINT64)blendDesc.RenderTargetWriteMask[1] << 31;	//Need 4 bits for RenderTargetWriteMask:Next shift is 35
	hash	 |= (UINT64)blendDesc.RenderTargetWriteMask[2] << 35;	//Need 4 bits for RenderTargetWriteMask:Next shift is 39

	gnBSHash = hash;

	if( gmBlendStates[hash] )
		return S_OK;

	return dxC_GetDevice()->CreateBlendState( &blendDesc, &gmBlendStates[hash] );
}

std::map<UINT64,ID3D10RasterizerState*> gmRSStates;
UINT64 gnRSHash = 0;

PRESULT dx10_CreateRSState( D3D10_RASTERIZER_DESC& RSDesc )
{
	UINT64 hash;
	hash  = (UINT64)RSDesc.CullMode;					//Need 2 bits for CullMode:		Next shift is 2
	hash |= (UINT64)RSDesc.FillMode << 2;				//Need 2 bits for FillMode:		Next shift is 4
	hash |= (UINT64)RSDesc.MultisampleEnable << 4;		//Need 1 bit for MSEnable:		Next shift is 5
	hash |= (UINT64)RSDesc.ScissorEnable << 5;			//Need 1 bit for ScissorEnable: Next shift is 6
	hash |= (UINT64)RSDesc.DepthBias << 6;				//Need 32 bits for DepthBias:	Next shift is 38
	hash |= (UINT64)RSDesc.SlopeScaledDepthBias << 38;	//Need 32 bits for DepthBias:	We're out of bits...\

	gnRSHash = hash;

	if( gmRSStates[hash] )
		return S_OK;
	
	return dxC_GetDevice()->CreateRasterizerState( &RSDesc, &gmRSStates[hash] );
}

// ----------------
//  Nuttapong Comment:
//  Need to handle alpha test and kill pixel
//	D3DRS_ALPHATESTENABLE,
//	D3DRS_ALPHAREF,
//	D3DRS_ALPHAFUNC,
//-----------------
ID3D10StateMapper::ID3D10StateMapper() 
{
	extern SPD3DCEFFECTPOOL gpEffectPool;
	gpEffectPool = NULL;
	dx10_InitPool();
#ifdef DX10_EMULATE_FIXED_FUNC
	//Now loaded on demand
	m_FixedFunc.pTextureArray = NULL;
	m_FixedFunc.pbUIOpaque = NULL;
#endif


	m_nIAType = VERTEX_DECL_INVALID;

	//matrixChanged = false;
	//worldHandle = dx10_GetGlobalEffect()->GetVariableByName( "World" )->AsMatrix();
	//viewHandle = dx10_GetGlobalEffect()->GetVariableByName( "View" )->AsMatrix();
	//projectionHandle = dx10_GetGlobalEffect()->GetVariableByName( "Projection" )->AsMatrix();
	//worldViewProjectionHandle = dx10_GetGlobalEffect()->GetVariableByName( "WorldViewProjection" )->AsMatrix();
	//worldViewHandle = dx10_GetGlobalEffect()->GetVariableByName( "WorldView" )->AsMatrix();
	//viewProjectionHandle = dx10_GetGlobalEffect()->GetVariableByName( "ViewProjection" )->AsMatrix();

	// Obtain all the handles

	m_AlphaTest.fAlphaTestRef = dx10_GetGlobalEffect()->GetVariableByName( "g_iAlphaTestRef" )->AsScalar();
	ASSERT(EFFECT_INTERFACE_ISVALID(m_AlphaTest.fAlphaTestRef ) );
	m_AlphaTest.bAlphaTestEnable = dx10_GetGlobalEffect()->GetVariableByName( "g_bAlphaTestEnable" )->AsScalar();
	ASSERT(EFFECT_INTERFACE_ISVALID(m_AlphaTest.bAlphaTestEnable ) );
	m_AlphaTest.iAlphaTestFunc = dx10_GetGlobalEffect()->GetVariableByName( "g_iAlphaTestFunc" )->AsScalar();
	ASSERT(EFFECT_INTERFACE_ISVALID(m_AlphaTest.iAlphaTestFunc ) );

	fvScreenSize = dx10_GetGlobalEffect()->GetVariableByName( "gvScreenSize" )->AsVector();
	char pszTextureName[32];

	for( UINT i = 0; i < MAX_FIXED_FUNC_TEXTURE_STAGES; ++i )
	{
		PStrPrintf( pszTextureName, 32, "TextureStage%d", i );
		gpTextureStages[i] = dx10_GetGlobalEffect()->GetVariableByName( pszTextureName )->AsShaderResource();
		ASSERT( EFFECT_INTERFACE_ISVALID(gpTextureStages[i] ) );
	}

	//Stream Source
	m_StreamSource.bStreamChanged = false;
	m_StreamSource.dwValidStreams = 0;

	//set screen size
	D3DXVECTOR4 vScreenSize;
	vScreenSize.x = (float)AppCommonGetWindowWidth();
	vScreenSize.y = (float)AppCommonGetWindowHeight();
	vScreenSize.z = 1.f / vScreenSize.x;
	vScreenSize.w = 1.f / vScreenSize.y;
	//vScreenSize.z = e_GetFarClipPlane();


	//Initialize defaults for state objects
	ZeroMemory( &DSDesc, sizeof(D3D10_DEPTH_STENCIL_DESC) );
	//DS Controllables
	DSDesc.DepthEnable = false;
	DSDesc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ZERO;
	DSDesc.DepthFunc = D3D10_COMPARISON_LESS_EQUAL;
	//DS static
	DSDesc.StencilEnable = false;
	V( dx10_CreateDSState( DSDesc ) );

	ZeroMemory( &BlendDesc, sizeof(D3D10_BLEND_DESC) );
	//Blend controllables 
	BlendDesc.BlendEnable[0] = false;
	BlendDesc.RenderTargetWriteMask[0] = 0x0000000f;
	BlendDesc.BlendOp = D3D10_BLEND_OP_ADD;
	BlendDesc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
	BlendDesc.SrcBlendAlpha = D3D10_BLEND_SRC_ALPHA;
	BlendDesc.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
	BlendDesc.DestBlendAlpha = D3D10_BLEND_DEST_ALPHA;
	BlendDesc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
	//Blend Static
	BlendDesc.AlphaToCoverageEnable = false;
	V( dx10_CreateBlendState( BlendDesc ) );

	ZeroMemory( &RSDesc, sizeof(D3D10_RASTERIZER_DESC) );
	//Rasterizer controllables
	RSDesc.CullMode = D3D10_CULL_BACK;
	RSDesc.FillMode = D3D10_FILL_SOLID;
	RSDesc.ScissorEnable = false;
	RSDesc.MultisampleEnable = false;
	//Rasterizer static
	RSDesc.AntialiasedLineEnable = false;
	RSDesc.DepthBias = 0;
	RSDesc.DepthBiasClamp = 0.0f;
	RSDesc.DepthClipEnable = true;
	RSDesc.FrontCounterClockwise = false;
	RSDesc.SlopeScaledDepthBias = 0.0f;
	V( dx10_CreateRSState( RSDesc ) );

	SetScreenSize( vScreenSize );
}

ID3D10StateMapper::~ID3D10StateMapper()
{
#ifndef HASH_ON_STATE
	for( rast_map::iterator iter = gmRSStates.begin(); iter != gmRSStates.end(); iter++ )
	{
		ID3D10RasterizerState* pRS = (*iter).first;
		SAFE_RELEASE( pRS );
	}

	for( bs_map::iterator iter = gmBlendStates.begin(); iter != gmBlendStates.end(); iter++ )
	{
		ID3D10BlendState* pBS = (*iter).first;
		SAFE_RELEASE( pBS );
	}

	for( ds_map::iterator iter = gmDepthStencilStates.begin(); iter != gmDepthStencilStates.end(); iter++ )
	{
		ID3D10DepthStencilState* pDS = (*iter).first;
		SAFE_RELEASE( pDS );
	}
#endif
}

#ifdef DX10_EMULATE_FIXED_FUNC
PRESULT ID3D10StateMapper::SetFixedFunctionEffect()
{
	D3D_EFFECT* pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_FIXED_FUNC_EMULATOR ) );
	if( !pEffect || !EFFECT_INTERFACE_ISVALID( pEffect->pD3DEffect ) )
		return S_FALSE;  //Effect hasn't loaded yet

	m_FixedFunc.pTextureArray	= pEffect->pD3DEffect->GetVariableByName( "g_TextureStageArray" )->AsShaderResource();
	m_FixedFunc.pbUIOpaque		= pEffect->pD3DEffect->GetVariableByName( "bUIOpaque" )->AsScalar();

	return S_OK;
}
#endif

PRESULT ID3D10StateMapper::SetRenderState(D3DRS_RASTER tState, DWORD_PTR dwValue, bool force)
{
	switch (tState) {
		case D3DRS_FILLMODE:
			RSDesc.FillMode = (D3D10_FILL_MODE)dwValue;
			break;
		case D3DRS_CULLMODE:
			RSDesc.CullMode = (D3D10_CULL_MODE)dwValue;
			break;
		case D3DRS_SCISSORTESTENABLE:
			RSDesc.ScissorEnable = (BOOL)dwValue;
			break;
		case D3DRS_DEPTHBIAS:
			RSDesc.DepthBias = (int)dwValue;
			break;
		case D3DRS_SLOPESCALEDEPTHBIAS:
			RSDesc.SlopeScaledDepthBias = (*(float*)&dwValue);
			break;
		case D3DRS_MULTISAMPLEENABLE:
			RSDesc.MultisampleEnable = (BOOL)dwValue;
			break;
		default:
			return E_NOTIMPL;
	}
	

	return dx10_CreateRSState( RSDesc );
}

PRESULT ID3D10StateMapper::SetRenderState(D3DRS_BLEND tState, DWORD_PTR dwValue, bool force )
{
	switch (tState)
	{
		case D3DRS_ALPHABLENDENABLE:
			BlendDesc.BlendEnable[0] = (dwValue!=0);
			break;
		case D3DRS_SRCBLEND:
			BlendDesc.SrcBlend = (D3D10_BLEND)dwValue;
			break;
		case D3DRS_DESTBLEND:
			BlendDesc.DestBlend = (D3D10_BLEND)dwValue;
			break;
		case D3DRS_BLENDOP:
			BlendDesc.BlendOp = (D3D10_BLEND_OP)dwValue;
			break;
		case D3DRS_SRCBLENDALPHA:
			BlendDesc.SrcBlendAlpha = (D3D10_BLEND)dwValue;
			break;
		case D3DRS_DESTBLENDALPHA:
			BlendDesc.DestBlendAlpha = (D3D10_BLEND)dwValue;
			break;	
		case D3DRS_BLENDOPALPHA:
			BlendDesc.BlendOpAlpha = (D3D10_BLEND_OP)dwValue;
			break;
		case D3DRS_COLORWRITEENABLE:
			BlendDesc.RenderTargetWriteMask[0] = (UINT8)dwValue;
			break;
		case D3DRS_COLORWRITEENABLE1:
			BlendDesc.RenderTargetWriteMask[1] = (UINT8)dwValue;
			break;
		case D3DRS_COLORWRITEENABLE2:
			BlendDesc.RenderTargetWriteMask[2] = (UINT8)dwValue;
			break;
/*
		case D3DRS_COLORWRITEENABLE3:
			BlendDesc.RenderTargetWriteMask[3] = (UINT8)dwValue;
			break;
*/
		case D3DRS_ALPHATESTENABLE:
			{
				V_RETURN( m_AlphaTest.bAlphaTestEnable->SetBool( dwValue ) );
			}
			break;
		case D3DRS_ALPHAREF:
			{
				V_RETURN( m_AlphaTest.fAlphaTestRef->SetInt( dwValue ) );
			}
			break;
		case D3DRS_ALPHAFUNC:
			{
				V_RETURN( m_AlphaTest.iAlphaTestFunc->SetInt( dwValue ) );
			}
		default:
			return E_NOTIMPL;
/*
		case D3DRS_BLENDFACTOR: {
			float bf = *((float *)&dwValue);
			float blendFactor[4] = {bf, bf, bf, bf};
			fBlendFactor->SetFloatVector(blendFactor);
								
			break;}
		case D3DRS_MULTISAMPLEMASK:
			iSampleMask->SetInt(*((int *)&dwValue));
			break;
			*/
	}

	return dx10_CreateBlendState( BlendDesc );
}

PRESULT ID3D10StateMapper::SetRenderState(DRDRS_DS tState, DWORD_PTR dwValue, bool force)
{
	switch (tState) {
		case D3DRS_ZENABLE:
			DSDesc.DepthEnable = (dwValue!=0);
			break;
		case D3DRS_ZWRITEENABLE:
			DSDesc.DepthWriteMask = (dwValue!=0) ? D3D10_DEPTH_WRITE_MASK_ALL : D3D10_DEPTH_WRITE_MASK_ZERO;
			break;
		case D3DRS_ZFUNC:
			DSDesc.DepthFunc = (D3D10_COMPARISON_FUNC)dwValue;
			break;
		default:
			return E_NOTIMPL; 
	};
	/*
		case D3DRS_STENCILENABLE:
			bStencilEnable->SetBool(*((bool* )&dwValue));			
			break;
		case D3DRS_STENCILMASK:
			iStencilReadMask->SetInt(*((int *)&dwValue));
			break;
		case D3DRS_STENCILWRITEMASK:
			iStencilWriteMask->SetInt(*((int *)&dwValue));
			break;
		case D3DRS_STENCILFAIL:
			iFrontFaceStencilFailOp->SetInt(*((int *)&dwValue));
			break;
		case D3DRS_STENCILZFAIL:
			iFrontFaceStencilDepthFailOp->SetInt(*((int *)&dwValue));
			break;
		case D3DRS_STENCILPASS:
			iFrontFaceStencilPassOp->SetInt(*((int *)&dwValue));
			break;
		case D3DRS_STENCILFUNC:
			iFrontFaceStencilFunc->SetInt(*((int *)&dwValue));
			break;

		case D3DRS_CCW_STENCILFAIL:
			iBackFaceStencilFailOp->SetInt(*((int *)&dwValue));
			break;
		case D3DRS_CCW_STENCILZFAIL:
			iBackFaceStencilDepthFailOp->SetInt(*((int *)&dwValue));
			break;
		case D3DRS_CCW_STENCILPASS:
			iBackFaceStencilPassOp->SetInt(*((int *)&dwValue));
			break;
		case D3DRS_CCW_STENCILFUNC:
			iBackFaceStencilFunc->SetInt(*((int *)&dwValue));
			break;
		case D3DRS_STENCILREF:
			iStencilRef->SetInt(*((int *)&dwValue));
			break;
	}
	*/

	return dx10_CreateDSState( DSDesc );
}

PRESULT ID3D10StateMapper::ApplyBeforeRendering( D3D_EFFECT* pEffect  )
{
	ASSERT_RETINVALIDARG( pEffect );
	ASSERT_RETFAIL( pEffect->ptCurrentPass && EFFECT_INTERFACE_ISVALID( pEffect->ptCurrentPass->hHandle ) );

	if( m_StreamSource.bStreamChanged && m_nIAType != VERTEX_DECL_INVALID )
	{
		// Call this function before each draw call to make sure the state changes get committed to the device
#ifdef DX10_CACHE_SETSTREAMSOURCE
		UINT nLast = 0;
		for( int i = 0; i < MAX_STREAM_SOURCE; ++i )
		{
			DWORD dwStream = 1 << i;
			if( !( m_StreamSource.dwValidStreams & dwStream ) )  //This slot is empty
			{
				if( nLast != i )  //If i == nLast we've got two invalid slots in a row
					dxC_GetDevice()->IASetVertexBuffers( nLast, i - nLast, &m_StreamSource.Buffers[nLast], &m_StreamSource.Strides[nLast], &m_StreamSource.Offsets[nLast] );

				nLast = i + 1;  //Next slot might be valid
			}
		}
		
#endif
		if( !pEffect->ptCurrentPass->pptIAObjects[ m_nIAType ])
		{
			V_RETURN( dx10_CreateIAObject( m_nIAType, pEffect->ptCurrentPass->hHandle, &pEffect->ptCurrentPass->pptIAObjects[ m_nIAType ] ) );
		}
		dxC_GetDevice()->IASetInputLayout( pEffect->ptCurrentPass->pptIAObjects[ m_nIAType ] );
	}
	m_StreamSource.bStreamChanged = false;

	// All state is set prior to calling apply, allowing effects to clobber state
	{
		//Set our depth stencil state
		dxC_GetDevice()->OMSetDepthStencilState( gmDepthStencilStates[gnDSHash], 0xffffffff );

		//Set our blend state
		const float blendFactor[4] = {1.0f, 1.0f, 1.0f, 1.0f };
		dxC_GetDevice()->OMSetBlendState( gmBlendStates[gnBSHash], blendFactor, 0xffffffff );

		//Set our rasterizer state
		dxC_GetDevice()->RSSetState( gmRSStates[gnRSHash] );
	}

	V_RETURN( pEffect->ptCurrentPass->hHandle->Apply( 0 ) );

	dxC_GetDevice()->PSSetSamplers( 0, NUM_SAMPLER_DX10_ENGINE_TOUCHES, (ID3D10SamplerState**)gppSamplerStates ); // Need to be set after apply to force

	dx10_ClearParamDirtyFlags( pEffect->ptCurrentTechnique->dwParamFlags );

	return S_OK;
}

PRESULT ID3D10StateMapper::SetIndices( LPD3DCIB pIB, D3DFORMAT ibFormat )
{
	if( pIB )  //KMNV: DX10 doesn't seem to like setting a NULL index buffer...
		dxC_GetDevice()->IASetIndexBuffer( pIB, ibFormat, 0 ); 

	return S_OK;
}

PRESULT ID3D10StateMapper::SetInputLayoutType( VERTEX_DECL_TYPE eType )
{
	m_nIAType = eType;
	return S_OK;
}

PRESULT ID3D10StateMapper::SetStreamSource( UINT StreamNumber, LPD3DCVB pStreamData, UINT OffsetInBytes, UINT Stride)
{
	m_StreamSource.bStreamChanged = true;

#ifdef DX10_CACHE_SETSTREAMSOURCE
	ASSERT_RETFAIL( StreamNumber <= MAX_STREAM_SOURCE );
	DWORD dwStream = 1 << StreamNumber;

	if( pStreamData == NULL )
		m_StreamSource.dwValidStreams &= ~dwStream;
	else
	{ 
		m_StreamSource.dwValidStreams |= dwStream;
		m_StreamSource.Buffers[StreamNumber] = pStreamData;
		m_StreamSource.Offsets[StreamNumber] = OffsetInBytes;
		m_StreamSource.Strides[StreamNumber] = Stride;
	}
#else
	dxC_GetDevice()->IASetVertexBuffers( StreamNumber, 1, &pStreamData, &Stride, &OffsetInBytes );
#endif
	return S_OK;
}

PRESULT ID3D10StateMapper::SetTransform( D3DTRANSFORMSTATETYPE tState, const D3DXMATRIXA16 * pMatrix )
{
	switch (tState) {
		case D3DTS_WORLD: MemoryCopy(&worldMat, sizeof(D3DXMATRIXA16), pMatrix, sizeof(D3DXMATRIXA16));
			dx10_PoolSetMatrix( EFFECT_PARAM_WORLDMATRIX, pMatrix );
			break;
		case D3DTS_VIEW: MemoryCopy(&viewMat, sizeof(D3DXMATRIXA16), pMatrix, sizeof(D3DXMATRIXA16));
			dx10_PoolSetMatrix( EFFECT_PARAM_VIEWMATRIX, pMatrix );
			break;
		case D3DTS_PROJECTION: MemoryCopy(&projectionMat, sizeof(D3DXMATRIXA16), pMatrix, sizeof(D3DXMATRIXA16));
			dx10_PoolSetMatrix( EFFECT_PARAM_PROJECTIONMATRIX, pMatrix );
			break;
		default:
			return S_FALSE;
	}

	const D3DXMATRIXA16 worldViewMat = worldMat * viewMat;
	const D3DXMATRIXA16 worldViewProjMat = worldViewMat * projectionMat;
	const D3DXMATRIXA16 viewProjMat = viewMat * projectionMat;

	V( dx10_PoolSetMatrix( EFFECT_PARAM_WORLDVIEW, &worldViewMat ) );
	V( dx10_PoolSetMatrix( EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX, &worldViewProjMat ) );
	V( dx10_PoolSetMatrix( EFFECT_PARAM_VIEWPROJECTIONMATRIX, &viewProjMat ) );

	return S_OK;
}

#ifdef DX10_EMULATE_FIXED_FUNC
PRESULT ID3D10StateMapper::UISetOpaque( BOOL bOpaque )
{
	if( !m_FixedFunc.pTextureArray && S_FALSE == SetFixedFunctionEffect() )  //Effect isn't loaded yet
		return S_FALSE;

	ASSERT_RETFAIL( m_FixedFunc.pbUIOpaque );

	V_RETURN( m_FixedFunc.pbUIOpaque->SetBool( bOpaque ) );
	return S_OK;
}
#endif


PRESULT ID3D10StateMapper::SetTexture( DWORD dwStage, LPD3DCBASESHADERRESOURCEVIEW pSRV )
{
	ASSERT_RETFAIL( dwStage < MAX_FIXED_FUNC_TEXTURE_STAGES );

	V_RETURN( gpTextureStages[dwStage]->SetResource( pSRV ) );

	return S_OK;
}

#ifdef DX10_EMULATE_FIXED_FUNC
PRESULT ID3D10StateMapper::SetTextureStageState( DWORD dwStage, D3DTEXTURESTAGESTATETYPE tType, DWORD dwValue )
{
	return S_OK;
}
#endif

PRESULT dxC_SetRenderState(D3DRS_RASTER tState, DWORD_PTR dwValue, bool force)
{
	//NUTTAPONG DONE: Call into statemanger depending on state flag
	if( gpStateManager )
		return gpStateManager->SetRenderState(tState, dwValue, force);
	return S_OK;
}
	
PRESULT dxC_SetRenderState(D3DRS_BLEND tState, DWORD_PTR dwValue, bool force)
{
	//NUTTAPONG DONE: Call into statemanger depending on state flag
	if( gpStateManager )
		return gpStateManager->SetRenderState(tState, dwValue, force);
	return S_OK;
}
	
PRESULT dxC_SetRenderState(DRDRS_DS tState, DWORD_PTR dwValue, bool force)
{
	//NUTTAPONG DONE: Call into statemanger depending on state flag
	if( gpStateManager )
		return gpStateManager->SetRenderState(tState, dwValue, force);
	return S_OK;
}

CStateManagerInterface* CStateManagerInterface::Create( STATE_MANAGER_TYPE eType )
{
	CStateManagerInterface* pStateManager = NULL;
	
	pStateManager = new CStateManagerInterface;
	return pStateManager;
}

#ifndef DX10_SAMPLERS_DEFINED_IN_FX_FILE
UINT gSamplerCurrentSampler[NUM_SAMPLER_DX10_ENGINE_TOUCHES] = {0};

PRESULT dx10_SetSamplerState( DWORD dwSampler, D3D10_FILTER Filter, D3D10_TEXTURE_ADDRESS_MODE AddressU, D3D10_TEXTURE_ADDRESS_MODE AddressV, D3D10_TEXTURE_ADDRESS_MODE AddressW, UINT nMaxAnisotropy, D3D10_COMPARISON_FUNC ComparisonFunc, float fMinLOD, float fMaxLOD, float fMipLODBias )
{
	if( dwSampler >= NUM_SAMPLER_DX10_ENGINE_TOUCHES )
		return S_OK;

	UINT nHash = (UINT)Filter;
	nHash |= (UINT)AddressU	  << 16;
	nHash |= (UINT)AddressV	  << 19;
	nHash |= (UINT)AddressW	  << 22;
	nHash |= nMaxAnisotropy	  << 25;  //Need 4 bits for nMaxAnisotropy

	if( nHash == gSamplerCurrentSampler[dwSampler] )  //This is a hack, it assumes
		return S_OK;

	gSamplerCurrentSampler[dwSampler] = nHash;
	gppSamplerStates[ dwSampler ] = NULL;

	D3D10_SAMPLER_DESC sampleDesc;
	memset( &sampleDesc, 0, sizeof(D3D10_SAMPLER_DESC) );
	sampleDesc.Filter = Filter;
	sampleDesc.AddressU = AddressU;
	sampleDesc.AddressV = AddressV;
	sampleDesc.AddressW = AddressW;
	sampleDesc.ComparisonFunc = ComparisonFunc;
	sampleDesc.MaxAnisotropy = nMaxAnisotropy;
	sampleDesc.MaxLOD = fMaxLOD;
	sampleDesc.MinLOD = fMinLOD;
	sampleDesc.MipLODBias = fMipLODBias;
	//sampleDesc.BorderColor = fBorderColor;

	V_RETURN( dxC_GetDevice()->CreateSamplerState( &sampleDesc, &gppSamplerStates[ dwSampler ] ) );

	return S_OK;
}

#endif


#ifdef DX10_EMULATE_FIXED_FUNC
PRESULT dx10_UISetOpaque( BOOL bOpaque )
{
	ASSERT_RETFAIL( gpStateManager );
	return gpStateManager->UISetOpaque( bOpaque );
}
#endif
