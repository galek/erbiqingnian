//----------------------------------------------------------------------------
// dx10_state.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX10_STATE__
#define __DX10_STATE__

#include "dxC_statemanager.h"

#ifdef DX10_SAMPLERS_DEFINED_IN_FX_FILE
inline dx10_SetSamplerState( DWORD dwSampler, D3D10_FILTER Filter, D3D10_TEXTURE_ADDRESS_MODE AddressU, D3D10_TEXTURE_ADDRESS_MODE AddressV, D3D10_TEXTURE_ADDRESS_MODE AddressW, UINT nMaxAnisotropy, D3D10_COMPARISON_FUNC ComparisonFunc, float fMinLOD, float fMaxLOD, float fMipLODBias, float fBorderColor ){}
#else
PRESULT dx10_SetSamplerState( DWORD dwSampler, D3D10_FILTER Filter, D3D10_TEXTURE_ADDRESS_MODE AddressU, D3D10_TEXTURE_ADDRESS_MODE AddressV, D3D10_TEXTURE_ADDRESS_MODE AddressW, UINT nMaxAnisotropy, D3D10_COMPARISON_FUNC ComparisonFunc = D3D10_COMPARISON_ALWAYS, float fMinLOD = 0.0f, float fMaxLOD = D3D10_FLOAT32_MAX, float fMipLODBias = 0.0f );

#endif



#endif //__DX10_STATE__