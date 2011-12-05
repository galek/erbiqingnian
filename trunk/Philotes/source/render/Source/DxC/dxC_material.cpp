//----------------------------------------------------------------------------
// dx9_material.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_effect.h"
#include "dxC_texture.h"
#include "performance.h"
#include "definition_priv.h"
#include "filepaths.h"
#include "datatables.h"
#include "dxC_material.h"
#include "appcommon.h"
#include "definition.h"
#include "dxC_pixo.h"
#include "e_main.h"


using namespace FSSE;


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dxC_GetShaderTypeID( const char * pszShaderName )
{
	ASSERT_RETINVALID( pszShaderName );
	return ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_EFFECTS_SHADERS, pszShaderName);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const char * dxC_GetShaderTypeFXFile( int nShaderLineID, int nType )
{
	ASSERT_RETNULL( nShaderLineID != INVALID_ID );
	ASSERT_RETNULL( nType >= 0 && nType < NUM_SHADER_TYPES );
	const SHADER_TYPE_DEFINITION * pShaderTypeDef = (const SHADER_TYPE_DEFINITION *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS_SHADERS, nShaderLineID );
	ASSERT( pShaderTypeDef );

	const EFFECT_DEFINITION * pEffectDef = (const EFFECT_DEFINITION *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, pShaderTypeDef->nEffectFile[ nType ] );
	ASSERT_RETNULL( pEffectDef );
	return pEffectDef->pszFXFileName;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dxC_GetShaderTypeEffectDefinitionID( int nShaderLineID, int nShaderType )
{
	ASSERT_RETINVALID( nShaderLineID != INVALID_ID );
	ASSERT_RETINVALID( nShaderType   != INVALID_ID );
	const SHADER_TYPE_DEFINITION * pShaderTypeDef = (const SHADER_TYPE_DEFINITION *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS_SHADERS, nShaderLineID );
	ASSERT_RETINVALID( pShaderTypeDef );
	return pShaderTypeDef->nEffectFile[ nShaderType ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dxC_GetShaderTypeEffectID( int nShaderLineID, int nShaderType )
{
	ASSERT_RETINVALID( nShaderLineID != INVALID_ID );
	ASSERT_RETINVALID( nShaderType   != INVALID_ID );
	const SHADER_TYPE_DEFINITION * pShaderTypeDef = (const SHADER_TYPE_DEFINITION *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS_SHADERS, nShaderLineID);
	ASSERT_RETINVALID( pShaderTypeDef );
	return pShaderTypeDef->nEffectID[ nShaderType ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT sRestoreShaderTypeEffect( int nShaderLineID, int nShaderType )
{
	ASSERT_RETINVALIDARG( nShaderLineID != INVALID_ID );
	ASSERT_RETINVALIDARG( nShaderType   != INVALID_ID );
	SHADER_TYPE_DEFINITION * pShaderTypeDef = (SHADER_TYPE_DEFINITION *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_SHADERS, nShaderLineID );
	ASSERT_RETFAIL( pShaderTypeDef );

	if ( pShaderTypeDef->nEffectFile[ nShaderType ] == INVALID_ID )
		pShaderTypeDef->nEffectID[ nShaderType ] = INVALID_ID;
	else
	{
		V_RETURN( dx9_EffectNew( &pShaderTypeDef->nEffectID[ nShaderType ], pShaderTypeDef->nEffectFile[ nShaderType ] ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void WINAPI sFillSpecularLookupTexture( D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData )
{
	// x == specular mask value [0..1]
	// y == dot3 result			[0..1]
	float * pfGloss = (float *)pData;
	float fGloss = ( ( pfGloss[ 1 ] - pfGloss[ 0 ] ) * pTexCoord->x + pfGloss[ 0 ] );
	float fDot   = pTexCoord->y;
	pOut->x = 
	pOut->y = 
	pOut->z = 
	pOut->w = pow( fDot, fGloss );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT dx9_RestoreSpecularLookupTexture( D3D_TEXTURE * pTexture )
{
	ASSERT_RETINVALIDARG( pTexture );
	MATERIAL * pMaterial = (MATERIAL *) DefinitionGetById( DEFINITION_GROUP_MATERIAL, pTexture->nUserID );
	ASSERT_RETFAIL( pMaterial );
	float fGlossMin = e_GetMinGlossiness() * SPECULAR_MAX_POWER;
	float pfGloss[ 2 ];
	pfGloss[ 0 ] = pMaterial->fGlossiness[ 0 ] * SPECULAR_MAX_POWER;
	pfGloss[ 1 ] = pMaterial->fGlossiness[ 1 ] * SPECULAR_MAX_POWER;
	pfGloss[ 0 ] = max( pfGloss[ 0 ], fGlossMin );
	pfGloss[ 1 ] = max( pfGloss[ 1 ], fGlossMin );
	V_RETURN( dxC_FillTexture( pTexture->pD3DTexture, sFillSpecularLookupTexture, pfGloss ) ); 
	V( dxC_TexturePreload( pTexture ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sCreateSpecularLookupTexture( MATERIAL * pMaterial, int nShaderType )
{
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	if ( pMaterial->nSpecularLUTId != INVALID_ID )
	{
		return S_FALSE;
	}

	EFFECT_DEFINITION * pEffectDef = e_MaterialGetEffectDef( pMaterial->tHeader.nId, nShaderType );
	if ( ! pEffectDef || ! TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_SPECULAR_LUT ) )
	{
		return S_FALSE;
	}

	char szTimer[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrCopy( szTimer, "Generating specular lookup for ", DEFAULT_FILE_WITH_PATH_SIZE );
	PStrCat( szTimer, pMaterial->tHeader.pszName, DEFAULT_FILE_WITH_PATH_SIZE );
	TIMER_START( szTimer );
	ASSERT_RETFAIL( pMaterial );
	int		  nDim	  = 256;
	DXC_FORMAT tFormat = D3DFMT_L8;
	// using lower resolution on u dimension (spec mask value) -- still need the resolution on the v (dp3 result)
	V_RETURN( dx9_TextureNewEmpty( &pMaterial->nSpecularLUTId, nDim/4, nDim, 1, tFormat, INVALID_ID, INVALID_ID, D3DC_USAGE_2DTEX, dx9_RestoreSpecularLookupTexture, pMaterial->tHeader.nId ) );
	e_TextureAddRef( pMaterial->nSpecularLUTId );
	ASSERT( pMaterial->nSpecularLUTId != INVALID_ID );

	unsigned int nMS = (unsigned int)TIMER_END();
	//trace( "Specular LUT generation took: %5d ms\n", nMS );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_MaterialUpdate ( void * pData )
{
	MATERIAL * pMaterial = (MATERIAL *) pData;
	ASSERT_RETURN( pMaterial );

	e_MaterialRelease( pMaterial->tHeader.nId );
	V( e_MaterialRestore( pData ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sSetMaterialTextureOverride( MATERIAL* pMaterial, TEXTURE_TYPE eType, const char* pOverrideStr )
{

	if ( pMaterial->nOverrideTextureID[ eType ] != INVALID_ID )
		return;

	char szFilepath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrPrintf( szFilepath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_MATERIAL, pOverrideStr );

	e_TextureReleaseRef( pMaterial->nOverrideTextureID[ eType ] );
	V( e_TextureNewFromFile( &pMaterial->nOverrideTextureID[ eType ], szFilepath, TEXTURE_GROUP_NONE, eType ) );
	e_TextureAddRef( pMaterial->nOverrideTextureID[ eType ] );

	if ( pMaterial->nOverrideTextureID[ eType ] == INVALID_ID && e_TextureSlotIsUsed( eType ) )
	{
		ASSERTV_MSG( "Failed to load %s override texture:\n\n%s\n\nIn material:\n\n%s", szFilepath, pOverrideStr, pMaterial->tHeader.pszName );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_MaterialRelease( int nMaterialID )
{
	MATERIAL * pMaterial = (MATERIAL *) DefinitionGetById( DEFINITION_GROUP_MATERIAL, nMaterialID );
	ASSERT_RETURN( pMaterial )

	// release material's resources (like textures)
	e_CubeTextureReleaseRef( pMaterial->nCubeMapTextureID );
	pMaterial->nCubeMapTextureID = INVALID_ID;
	e_TextureReleaseRef( pMaterial->nSphereMapTextureID );
	pMaterial->nSphereMapTextureID = INVALID_ID;
	e_TextureReleaseRef( pMaterial->nSpecularLUTId );
	pMaterial->nSpecularLUTId = INVALID_ID;
	for ( int i = 0; i < NUM_TEXTURE_TYPES; i++ )
	{
		e_TextureReleaseRef( pMaterial->nOverrideTextureID[ i ] );
		pMaterial->nOverrideTextureID[ i ] = INVALID_ID;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sAddAmtLCM( MATERIAL * pMaterial, int nSampler )
{
	if ( ! FEQ( pMaterial->fScrollAmt[nSampler], 0.f, 0.0001f ) )
	{
		float fAmt = fabsf( pMaterial->fScrollAmt[nSampler] );
		pMaterial->fScrollAmtLCM = LCM( pMaterial->fScrollAmtLCM, LCM( fAmt, 1.f ) / fAmt );
	}
}

PRESULT e_MaterialComputeScrollAmtLCM( MATERIAL * pMaterial )
{
	ASSERT_RETINVALIDARG( pMaterial );

	// compute scrolling LCM
	if ( pMaterial->dwFlags & MATERIAL_FLAG_SCROLLING_ENABLED )
	{
		pMaterial->fScrollAmtLCM = 1.f;

		sAddAmtLCM( pMaterial, SAMPLER_DIFFUSE );
		sAddAmtLCM( pMaterial, SAMPLER_NORMAL );
		sAddAmtLCM( pMaterial, SAMPLER_SPECULAR );
		sAddAmtLCM( pMaterial, SAMPLER_SELFILLUM );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_MaterialRestore ( void * pData )
{
	MATERIAL * pMaterial = (MATERIAL *) pData;
	ASSERT_RETINVALIDARG( pMaterial )

	pMaterial->nShaderLineId = dxC_GetShaderTypeID( pMaterial->pszShaderName );

	int nShaderType = dx9_GetCurrentShaderType( NULL );
	if ( nShaderType != LEVEL_LOCATION_UNKNOWN )
	{
		// level location must be set to restore the effect
		V_RETURN( sRestoreShaderTypeEffect( pMaterial->nShaderLineId, nShaderType ) );
	}

	pMaterial->tScrollLastUpdateTime = e_GetCurrentTimeRespectingPauses();
	V( e_MaterialComputeScrollAmtLCM( pMaterial ) );

	if ( pMaterial->dwFlags & MATERIAL_FLAG_ENVMAP_ENABLED &&
		! ( pMaterial->dwFlags & MATERIAL_FLAG_CUBE_ENVMAP || pMaterial->dwFlags & MATERIAL_FLAG_SPHERE_ENVMAP ) )
	{
		// if they specify to use an env map but neither cube or sphere are specified, assume cube
		pMaterial->dwFlags |= MATERIAL_FLAG_CUBE_ENVMAP;
	}

	// load envmap texture if present
	// CHB 2006.10.16 - Check to see if envmap will be used.
	// CML 2007.10.06 - The "env map is used" check has been moved to apply to cube maps only, not including sphere maps.
	if ( pMaterial->szEnvMapFileName[ 0 ] && ! dxC_IsPixomaticActive() )
	{
		if ( pMaterial->dwFlags & MATERIAL_FLAG_SPHERE_ENVMAP && pMaterial->nSphereMapTextureID == INVALID_ID )
		{
			char szFilepath[ DEFAULT_FILE_WITH_PATH_SIZE ];
			PStrPrintf( szFilepath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_MATERIAL, pMaterial->szEnvMapFileName );

			e_TextureReleaseRef( pMaterial->nSphereMapTextureID ); 
			V( e_TextureNewFromFile(
				&pMaterial->nSphereMapTextureID,
				szFilepath,
				TEXTURE_GROUP_NONE,
				TEXTURE_ENVMAP ) );
			e_TextureAddRef( pMaterial->nSphereMapTextureID );

			if ( pMaterial->nSphereMapTextureID == INVALID_ID )
			{
				ASSERTV_MSG( "Failed to load sphere map texture:\n\n%s\n\nIn material:\n\n%s", szFilepath, pMaterial->tHeader.pszName );
			}
		}
		else if ( pMaterial->dwFlags & MATERIAL_FLAG_CUBE_ENVMAP && pMaterial->nCubeMapTextureID == INVALID_ID && e_TextureSlotIsUsed(TEXTURE_ENVMAP) )
		{
			//ErrorDialog( "Material cube maps not yet fully implemented!  Go bug Chris!\n\n%s", pMaterial->tHeader.pszName );

			char szFilepath[ DEFAULT_FILE_WITH_PATH_SIZE ];
			PStrPrintf( szFilepath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_MATERIAL, pMaterial->szEnvMapFileName );

			// load cube map
			e_CubeTextureReleaseRef( pMaterial->nCubeMapTextureID );
			V( e_CubeTextureNewFromFile( &pMaterial->nCubeMapTextureID, szFilepath ) );
			//D3D_CUBETEXTURE * pTexture = dx9_CubeTextureGet( pMaterial->nCubeMapTextureID );
			//if ( pTexture && pTexture->pD3DTexture )
			//	pMaterial->nCubeMapMIPLevels = pTexture->pD3DTexture->GetLevelCount();
			e_CubeTextureAddRef( pMaterial->nCubeMapTextureID );
			pMaterial->nCubeMapMIPLevels = -1;

			if ( pMaterial->nCubeMapTextureID == INVALID_ID )
			{
				ASSERTV_MSG( "Failed to load cube map texture:\n\n%s\n\nIn material:\n\n%s", szFilepath, pMaterial->tHeader.pszName );
			}
		}
	}

	// diffuse override
	if( PStrLen(pMaterial->szDiffuseOverrideFileName) )
		sSetMaterialTextureOverride( pMaterial, TEXTURE_DIFFUSE, pMaterial->szDiffuseOverrideFileName );

	// normal override
	if( PStrLen(pMaterial->szNormalOverrideFileName) )
		sSetMaterialTextureOverride( pMaterial, TEXTURE_NORMAL, pMaterial->szNormalOverrideFileName );

	// self-illumination override
	if( PStrLen(pMaterial->szSelfIllumOverrideFileName) )
		sSetMaterialTextureOverride( pMaterial, TEXTURE_SELFILLUM, pMaterial->szSelfIllumOverrideFileName );

	// specular override
	if( PStrLen(pMaterial->szSpecularOverrideFileName) )
		sSetMaterialTextureOverride( pMaterial, TEXTURE_SPECULAR, pMaterial->szSpecularOverrideFileName );

	// particle system hack (havok- or DX10-related?)
	if( pMaterial->szParticleSystemDef[ 0 ] )
		pMaterial->nParticleSystemDefId = DefinitionGetIdByName( DEFINITION_GROUP_PARTICLE, pMaterial->szParticleSystemDef );
	else 
		pMaterial->nParticleSystemDefId = INVALID_ID;

	// generate specular lookup texture
	V( sCreateSpecularLookupTexture( pMaterial, nShaderType ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_MaterialGetCubeMap( struct MATERIAL * pMaterial, int & nTextureID, int & nMIPLevels )
{
	nTextureID = INVALID_ID;
	nMIPLevels = -1;
	ASSERT_RETINVALIDARG( pMaterial );

	if ( e_CubeTextureIsLoaded( pMaterial->nCubeMapTextureID, TRUE ) )
	{
		if ( pMaterial->nCubeMapMIPLevels <= 0 )
		{
			D3D_CUBETEXTURE * pTexture = dx9_CubeTextureGet( pMaterial->nCubeMapTextureID );
			ASSERT_RETFAIL( pTexture && pTexture->pD3DTexture );
			pMaterial->nCubeMapMIPLevels = dxC_GetNumMipLevels( pTexture->pD3DTexture );
		}

		nTextureID = pMaterial->nCubeMapTextureID;
		nMIPLevels = pMaterial->nCubeMapMIPLevels;
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_MaterialNew ( int* pnMaterialID, char * pszMaterialName, BOOL bForceSyncLoad /*= FALSE*/ )
{
	ASSERT_RETINVALIDARG( pnMaterialID );
	*pnMaterialID = DefinitionGetIdByName( DEFINITION_GROUP_MATERIAL, pszMaterialName, -1, bForceSyncLoad );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL MaterialDefinitionPostXMLLoad(
	void * pDef,
	BOOL bForceSyncLoad )
{
	ASSERT_RETFALSE( pDef );

	V( e_MaterialRestore( pDef ) );

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_MaterialsRefresh( BOOL bForceSyncLoad /*= FALSE*/ )
{
	// this catches the non-material effects that need restoring as well
	V( e_ShadersReloadAll( bForceSyncLoad ) );

	int nID = DefinitionGetFirstId( DEFINITION_GROUP_MATERIAL );
	while ( nID != INVALID_ID )
	{
		e_MaterialRelease( nID );
		V( e_MaterialRestore( nID ) );
		nID = DefinitionGetNextId( DEFINITION_GROUP_MATERIAL, nID );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
EFFECT_DEFINITION * e_MaterialGetEffectDef( int nMaterial, int nShaderType )
{
	MATERIAL * pMaterial = (MATERIAL *) DefinitionGetById( DEFINITION_GROUP_MATERIAL, nMaterial );
	if ( ! pMaterial )
		return NULL;
	//e_MaterialRestore ( pMaterial );
	if ( nShaderType == INVALID_ID )
		nShaderType = SHADER_TYPE_DEFAULT;  // default to indoor for purposes of effect definition

	int nEffectDef = dxC_GetShaderTypeEffectDefinitionID( pMaterial->nShaderLineId, nShaderType );
	if ( nEffectDef == INVALID_ID )
	{
		V( dx9_LoadShaderType( pMaterial->nShaderLineId, nShaderType ) );
		nEffectDef = dxC_GetShaderTypeEffectDefinitionID( pMaterial->nShaderLineId, nShaderType );
	}

	//D3D_EFFECT * pEffect = dx9_EffectGet( dxC_GetShaderTypeEffectID( pMaterial->nShaderLineId, nShaderType ) );
	//if ( ! pEffect )
	//{
	//	dx9_RestoreShaderTypeEffect( pMaterial->nShaderLineId, nShaderType );
	//	pEffect = dx9_EffectGet( dxC_GetShaderTypeEffectID( pMaterial->nShaderLineId, nShaderType ) );
	//	ASSERT_RETNULL( pEffect );
	//}
	if ( nEffectDef == INVALID_ID )
		return NULL;
	return (EFFECT_DEFINITION *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, nEffectDef );
}
