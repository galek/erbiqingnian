//----------------------------------------------------------------------------
// dx10_statemanager.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX10_STATEMANAGER__
#define __DX10_STATEMANAGER__

#include "dxC_effect.h"
#include "dxC_primitive.h"
#include <d3d.h>
#define MAX_STREAM_SOURCE 32

#include <vector>
using namespace std;
class ID3D10StateMapper
{
private:

#ifdef DX10_EMULATE_FIXED_FUNC
	struct
	{
		ID3D10EffectShaderResourceVariable* pTextureArray;
		ID3D10EffectScalarVariable* pbUIOpaque;
	} m_FixedFunc;

	PRESULT SetFixedFunctionEffect();
#endif

public:
	//bool matrixChanged;
	D3DXMATRIXA16 worldMat;
	D3DXMATRIXA16 viewMat;
	D3DXMATRIXA16 projectionMat;

	//ID3D10EffectMatrixVariable* worldHandle;
	//ID3D10EffectMatrixVariable* viewHandle;
	//ID3D10EffectMatrixVariable* projectionHandle;
	//ID3D10EffectMatrixVariable* worldViewProjectionHandle;
	//ID3D10EffectMatrixVariable* worldViewHandle;
	//ID3D10EffectMatrixVariable* viewProjectionHandle;

	//Alpha test
	struct
	{
		ID3D10EffectScalarVariable* bAlphaTestEnable;
		ID3D10EffectScalarVariable* fAlphaTestRef;
		ID3D10EffectScalarVariable* iAlphaTestFunc;
	} m_AlphaTest;

	//ScreenSize
	ID3D10EffectVectorVariable* fvScreenSize;

	//States
	D3D10_DEPTH_STENCIL_DESC DSDesc;
	D3D10_BLEND_DESC BlendDesc;
	D3D10_RASTERIZER_DESC RSDesc;

	// Stream Source
	struct  
	{
		ID3D10Buffer* Buffers[MAX_STREAM_SOURCE];
		UINT Offsets[MAX_STREAM_SOURCE];
		UINT Strides[MAX_STREAM_SOURCE];
		bool bStreamChanged;
		DWORD dwValidStreams;
	} m_StreamSource;

	VERTEX_DECL_TYPE m_nIAType;

	ID3D10StateMapper();
	~ID3D10StateMapper();

	PRESULT SetScreenSize( D3DXVECTOR4 vScreenSize )
	{
		V_RETURN( fvScreenSize->SetFloatVector( (float*)&vScreenSize ) );
		return S_OK;
	}

	PRESULT ApplyBeforeRendering( D3D_EFFECT* pEffect  );

	PRESULT SetIndices( LPD3DCIB pIB, D3DFORMAT ibFormat );
	PRESULT SetInputLayoutType( VERTEX_DECL_TYPE eType );
	PRESULT SetStreamSource( UINT StreamNumber, LPD3DCVB pStreamData, UINT OffsetInBytes, UINT Stride);
	PRESULT SetFVF( DWORD dwFVF )
	{
		return SetInputLayoutType( (VERTEX_DECL_TYPE)dwFVF );
	}
	PRESULT SetTransform( D3DTRANSFORMSTATETYPE tState, const D3DXMATRIXA16 * pMatrix );
	PRESULT SetRenderState(D3DRS_RASTER tState, DWORD_PTR dwValue, bool force = false);
	PRESULT SetRenderState(D3DRS_BLEND tState, DWORD_PTR dwValue, bool force = false);
	PRESULT SetRenderState(DRDRS_DS tState, DWORD_PTR dwValue, bool force = false);
	PRESULT SetTexture( DWORD dwStage, LPD3DCBASESHADERRESOURCEVIEW pTexture );
#ifdef DX10_EMULATE_FIXED_FUNC
	PRESULT SetTexture( DWORD dwStage, LPD3DCBASESHADERRESOURCEVIEW pSRV );
	PRESULT SetTextureStageState( DWORD dwStage, D3DTEXTURESTAGESTATETYPE tType, DWORD dwValue );
	PRESULT UISetOpaque( BOOL bOpaque );
#endif
};

#ifdef DX10_EMULATE_FIXED_FUNC
PRESULT dx10_UISetOpaque( BOOL bOpaque );
#endif

#endif