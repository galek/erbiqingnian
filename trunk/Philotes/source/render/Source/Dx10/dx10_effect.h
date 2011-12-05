//----------------------------------------------------------------------------
// dx10_effect.h
//	- dx10 effect methods
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

void dx10_InitPool();
PRESULT dx10_EffectFillParams( const D3D_EFFECT* pEffect, EFFECT_TECHNIQUE & tTechnique );
PRESULT dx10_PoolSetTexture( EFFECT_PARAM eParam, const LPD3DCBASESHADERRESOURCEVIEW pTexture );
PRESULT dx10_PoolSetFloat( EFFECT_PARAM eParam, float fValue );
PRESULT dx10_PoolSetFloatVector( EFFECT_PARAM eParam, float* pValue );
PRESULT dx10_PoolSetMatrix( EFFECT_PARAM eParam, const D3DXMATRIXA16* pMatrix );
BOOL dx10_IsParamGlobal( EFFECT_PARAM eParam );
D3DC_EFFECT_VARIABLE_HANDLE dx10_GetGlobalHandle( EFFECT_PARAM eParam );

void dx10_SetParamDirtyFlags( DWORD* pVariablesUsed );
void dx10_ClearParamDirtyFlags( DWORD* pVariablesUsed );
void dx10_DirtyParam( EFFECT_PARAM eParam );
BOOL dx10_IsParamDirty( EFFECT_PARAM eParam );