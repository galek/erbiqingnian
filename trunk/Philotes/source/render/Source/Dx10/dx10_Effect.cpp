//----------------------------------------------------------------------------
// dx10_effect.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

D3DC_EFFECT_VARIABLE_HANDLE hPoolParams[NUM_EFFECT_PARAMS];
DWORD						dwGlobalParamFlags[ DWORD_FLAG_SIZE(NUM_EFFECT_PARAMS) ] = {0};
DWORD						dwGlobalParamDirtyFlags[ DWORD_FLAG_SIZE(NUM_EFFECT_PARAMS) ] = {0};

D3D10_SHADER_MACRO g_pFX10Defines[] = {
	{ "ENGINE_TARGET_DX10", "1" },
	{ "FXC10_PATH", "1" },
	{ "PRIME", "1" },
	{ NULL, NULL },
};

void dx10_InitPool()
{
	//We need to load our statemanager as the pool before any effects
	TCHAR szEffectPoolName[DEFAULT_FILE_WITH_PATH_SIZE];
	dxC_GetFullShaderPath( szEffectPoolName, "StateManager.fxo", EFFECT_FOLDER_HELLGATE, EFFECT_SUBFOLDER_NOTDEFINED );

	DECLARE_LOAD_SPEC( spec, szEffectPoolName );
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_ALWAYS_WARN_IF_MISSING;
	PakFileLoadNow(spec);

	ASSERTV_RETURN( spec.buffer && spec.bytesread, "Failed to load state manager file: \n%s", szEffectPoolName );

	LPD3DCBLOB test = NULL;
	extern SPD3DCEFFECTPOOL gpEffectPool;
	V( D3DX10CreateEffectPoolFromMemory(
		spec.buffer, 
		spec.bytesread,
		szEffectPoolName,
		g_pFX10Defines, 
		NULL, 
		NULL,
		NULL,
		NULL,
		dxC_GetDevice(),
		NULL, //KMNV TODO THREADPUMP
		&gpEffectPool.p,
		&test,
		NULL ) );

	FREE( spec.pool, spec.buffer );

	ASSERT_RETURN( gpEffectPool );

	for ( int i = 0; i < NUM_EFFECT_PARAMS; i++ )
	{
		D3DC_EFFECT_VARIABLE_HANDLE varHandle =  dx10_GetGlobalEffect()->GetVariableByName( gptEffectParamChart[ i ].pszName );
		if( EFFECT_INTERFACE_ISVALID( varHandle ) )
		{
			hPoolParams[i] = varHandle;
			SETBIT( dwGlobalParamFlags, i, 1 );
		}
	}
}


PRESULT dx10_PoolSetTexture( EFFECT_PARAM eParam, const LPD3DCBASESHADERRESOURCEVIEW pTexture )
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = hPoolParams[eParam];

	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		return hParam->AsShaderResource()->SetResource( pTexture );
	}
	return E_FAIL;
}

PRESULT dx10_PoolSetFloat( EFFECT_PARAM eParam, float fValue )
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = hPoolParams[eParam];

	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		return hParam->AsScalar()->SetFloat( fValue );
	}
	return E_FAIL;
}


D3DC_EFFECT_VARIABLE_HANDLE dx10_GetGlobalHandle( EFFECT_PARAM eParam )
{
	return hPoolParams[ eParam ];
}

BOOL dx10_IsParamGlobal( EFFECT_PARAM eParam )
{
	return TESTBIT_MACRO_ARRAY( dwGlobalParamFlags, eParam );
}

BOOL dx10_IsParamDirty( EFFECT_PARAM eParam )
{
	return TESTBIT_MACRO_ARRAY( dwGlobalParamDirtyFlags, eParam );
}

void dx10_DirtyParam( EFFECT_PARAM eParam )
{
	SETBIT( dwGlobalParamDirtyFlags, eParam, 1 );
}

void dx10_SetParamDirtyFlags( DWORD* pVariablesUsed )
{
	for( int i =0; i < DWORD_FLAG_SIZE(NUM_EFFECT_PARAMS); ++i )
	{
		dwGlobalParamDirtyFlags[i] |= pVariablesUsed[i];
	}
}

void dx10_ClearParamDirtyFlags( DWORD* pVariablesUsed )
{
	for( int i =0; i < DWORD_FLAG_SIZE(NUM_EFFECT_PARAMS); ++i )
	{
		dwGlobalParamDirtyFlags[i] = dwGlobalParamDirtyFlags[i] & (~pVariablesUsed[i]);
	}
}


PRESULT dx10_PoolSetFloatVector( EFFECT_PARAM eParam, float* pValue )
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = hPoolParams[eParam];

	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		return hParam->AsVector()->SetFloatVector( pValue );
	}
	return E_FAIL;
}