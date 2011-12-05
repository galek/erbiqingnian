//----------------------------------------------------------------------------
// dx10_ShaderReflection.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"


static inline void sReflectConstantBuffersFromShader( ID3D10EffectShaderVariable* pShaderVar, ID3D10ShaderReflection** ppReflector, UINT* nCBCount )
{
	ASSERT_RETURN( ppReflector && nCBCount );
	D3D10_EFFECT_SHADER_DESC effectShadDesc;

	if( !EFFECT_INTERFACE_ISVALID( pShaderVar ) )
		return;

	pShaderVar->GetShaderDesc( 0, &effectShadDesc );

	if( FAILED( D3D10ReflectShader( effectShadDesc.pBytecode, effectShadDesc.BytecodeLength, ppReflector ) ) )
		return;
	
	ASSERT_RETURN( *ppReflector );

	D3D10_SHADER_DESC shadDesc;
	(*ppReflector)->GetDesc( &shadDesc );
	*nCBCount = shadDesc.ConstantBuffers;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static inline bool sReflectorTest( ID3D10ShaderReflection* pReflector, UINT nCBCount, const char* varName, UINT nVarIndex, DWORD* pDWFlags )
{
	ASSERT_RETFALSE( pDWFlags );
	D3D10_SHADER_VARIABLE_DESC varDesc;

	for(UINT i = 0; i < nCBCount; ++i )
	{
		pReflector->GetConstantBufferByIndex( i )->GetVariableByName( varName )->GetDesc( &varDesc );
		if( ( varDesc.uFlags & D3D10_SVF_USED ) )
		{
			SETBIT( pDWFlags, nVarIndex, 1 );
			return true;
		}
	}
	return false;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx10_EffectFillParams( const D3D_EFFECT* pEffect, EFFECT_TECHNIQUE & tTechnique )
{
	ASSERT_RETINVALIDARG( pEffect && pEffect->pD3DEffect );

	ASSERT_RETINVALIDARG( EFFECT_INTERFACE_ISVALID( tTechnique.hHandle ) );

	D3D10_TECHNIQUE_DESC techDesc;
	tTechnique.hHandle->GetDesc( &techDesc );

	for( UINT i = 0; i < techDesc.Passes; ++i )
	{
		D3DC_EFFECT_PASS_HANDLE hPass = tTechnique.hHandle->GetPassByIndex( i );

		ASSERT_CONTINUE( EFFECT_INTERFACE_ISVALID( hPass ) );

		D3D10_PASS_SHADER_DESC passShaderDesc;
		
		ID3D10ShaderReflection* pVSShaderReflection = NULL;
		ID3D10ShaderReflection* pGSShaderReflection = NULL;
		ID3D10ShaderReflection* pPSShaderReflection = NULL;
		

		UINT nVSCBCount = 0;
		UINT nGSCBCount = 0;
		UINT nPSCBCount = 0;


		hPass->GetVertexShaderDesc( &passShaderDesc );
		sReflectConstantBuffersFromShader( passShaderDesc.pShaderVariable, &pVSShaderReflection, &nVSCBCount );

		hPass->GetGeometryShaderDesc( &passShaderDesc );
		sReflectConstantBuffersFromShader( passShaderDesc.pShaderVariable, &pGSShaderReflection, &nGSCBCount );

		hPass->GetPixelShaderDesc( &passShaderDesc );
		sReflectConstantBuffersFromShader( passShaderDesc.pShaderVariable, &pPSShaderReflection, &nPSCBCount );


		if( pVSShaderReflection || pGSShaderReflection || pPSShaderReflection )
		{
			for ( int j = 0; j < NUM_EFFECT_PARAMS; j++ )
			{
#if 1 //KMNV HACK: Shader resources aren't treated the same way
				if( j >= EFFECT_PARAM_DIFFUSEMAP0 && j <= EFFECT_PARAM_SCREENFX_TEXTURERT )
				{
					SETBIT( tTechnique.dwParamFlags, j, 1 );
					continue;
				}
#endif
				if( TESTBIT_MACRO_ARRAY(tTechnique.dwParamFlags, j) )
					continue;

				if( sReflectorTest( pVSShaderReflection, nVSCBCount, gptEffectParamChart[ j ].pszName, j, tTechnique.dwParamFlags ) )
					continue;

				if( sReflectorTest( pGSShaderReflection, nGSCBCount, gptEffectParamChart[ j ].pszName, j, tTechnique.dwParamFlags ) )
					continue;

				if( sReflectorTest( pPSShaderReflection, nPSCBCount, gptEffectParamChart[ j ].pszName, j, tTechnique.dwParamFlags ) )
					continue;
			}
		}
		SAFE_RELEASE( pPSShaderReflection );
		SAFE_RELEASE( pGSShaderReflection );
		SAFE_RELEASE( pPSShaderReflection );

	}
	return S_OK;
}
