//----------------------------------------------------------------------------
// dx9_debug.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "camera_priv.h"
#include "dxC_occlusion.h"
#include "dxC_drawlist.h"
#include "dxC_model.h"
#include "dxC_meshlist.h"
#include "dxC_texture.h"
#include "dxC_target.h"
#include "filepaths.h"
#include "dxC_caps.h"
#include "dxC_primitive.h"
#include "e_main.h"
#include "dxC_light.h"
#include "dxC_state.h"
#include "dxC_state_defaults.h"
#include "e_texture_priv.h"
#include "appcommontimer.h"
#include "e_optionstate.h"	// CHB 2007.03.02
#include "dxC_obscurance.h"
#include "e_viewer.h"
#include "e_particle.h"
#include "dxC_profile.h"
#include "chb_timing.h"
#include "dxC_query.h"
#include "dxC_region.h"
#include "dxC_viewer.h"

using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

namespace
{
	FILE * gpDxDumpFile = NULL;

	struct DRAWLIST_DUMP 
	{
		char * pszBuf;
		int nAlloc;
		int nUsed;
	} gtDumpDrawlist = { NULL, 0, 0 };

};

DEBUG_MESH_RENDER_INFO gtDebugMeshRenderInfo;
CArrayIndexed<DEBUG_MIP_TEXTURE> gtDebugMipTextures;
BOOL gbDumpDrawList = FALSE;

#if ISVERSION(DEVELOPMENT)
SHADER_PROFILE_RENDER	gtShaderProfileRender = { INVALID_ID, INVALID_ID, 0, 0, NULL, NULL };
float					gfShaderProfileStatsCallbackElapsed = 0.f;
#endif

//static LINE_GRAPH<DWORD>	gtUploadGraph;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

static void sDxDumpLogger( const char* s )
{
#ifdef DXDUMP_ENABLE
	fprintf( gpDxDumpFile, "%s\n", s );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dx9_DXDumpAll()
{
#ifdef DXDUMP_ENABLE
	if ( !gpDxDumpFile )
	{
		char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrPrintf( szPath, DEFAULT_FILE_WITH_PATH_SIZE, "%sdxDumpLog.txt", LogGetRootPath() );
		TCHAR szFullname[DEFAULT_FILE_WITH_PATH_SIZE];
		FileGetFullFileName(szFullname, szPath, DEFAULT_FILE_WITH_PATH_SIZE);
		gpDxDumpFile = fopen( szFullname, "wt" );
	}
	dxDumpAll( dxC_GetDevice(), sDxDumpLogger );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dx9_DXDestroy()
{
#ifdef DXDUMP_ENABLE
	if ( gpDxDumpFile )
	{
		fclose( gpDxDumpFile );
		gpDxDumpFile = NULL;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_CreateDebugMipTextures ()
{
	D3DCOLOR nColors[ DEBUG_MIP_TEXTURE_COUNT ] = { 
		0xffff0000, // red
		0xff00ff00, // green
		0xff0000ff, // blue
		0xffffff00, // yellow
		0xff7f00ff, // purple
		0xff00ff7f, // cyan
		0xff7f0000, // dark red
	};

	RECT rColor;
	rColor.top    = 0;
	rColor.left   = 0;
	rColor.bottom = MAX_MIP_TEXTURE_SIZE - 1;
	rColor.right  = MAX_MIP_TEXTURE_SIZE - 1;

	// ugly but necessary, since I can't find a D3D or D3DX function to scale a surface
	int nDataCount = MAX_MIP_TEXTURE_SIZE * MAX_MIP_TEXTURE_SIZE;
	UINT * pColorData[ DEBUG_MIP_TEXTURE_COUNT ];
	for ( int i = 0; i < DEBUG_MIP_TEXTURE_COUNT; i++ )
	{
		pColorData[ i ] = (UINT*)MALLOC( NULL, sizeof(UINT) * nDataCount );
		for ( int x = 0; x < nDataCount; x++ )
			pColorData[ i ][ x ] = nColors[ i ];
	}

	int nMaxSize = dxC_CapsGetMaxSquareTextureResolution();
	for ( int nSize = MIN_MIP_TEXTURE_SIZE, nLevels = 1; nSize <= MAX_MIP_TEXTURE_SIZE; nSize *= 2, nLevels++ )
	{
		if ( nSize > nMaxSize )
			break;

		int nID;
		DEBUG_MIP_TEXTURE * pDbgMipTex;
		pDbgMipTex = gtDebugMipTextures.Add( &nID );
		pDbgMipTex->nResolution = nSize;

		D3D_TEXTURE * pTexture;
		V_CONTINUE( dxC_TextureAdd( INVALID_ID, &pTexture, INVALID_ID, INVALID_ID ) );
		ASSERT_CONTINUE( pTexture );
		pDbgMipTex->nTextureID = pTexture->nId;

		V_CONTINUE( dxC_Create2DTexture( nSize, nSize, nLevels, D3DC_USAGE_2DTEX, D3DFMT_DXT1, &pTexture->pD3DTexture ) );

		for ( int nLevel = 0; nLevel < nLevels; nLevel++ )
		{
			V( dxC_UpdateTexture( pTexture->pD3DTexture, NULL, pColorData[ nLevel ], &rColor, nLevel, D3DFMT_A8R8G8B8, sizeof(UINT) * MAX_MIP_TEXTURE_SIZE ) );
		}
	}

	for ( int i = 0; i < DEBUG_MIP_TEXTURE_COUNT; i++ )
		if ( pColorData[ i ] )
			FREE( NULL, pColorData[ i ] );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ReleaseDebugMipTextures ()
{
	for ( int nDbgMipTex = gtDebugMipTextures.GetFirst();
		nDbgMipTex != INVALID_ID;
		nDbgMipTex = gtDebugMipTextures.GetNextId( nDbgMipTex ) )
	{
		DEBUG_MIP_TEXTURE * pDbgMipTex = gtDebugMipTextures.Get( nDbgMipTex );
		ASSERT_CONTINUE( pDbgMipTex );
		e_TextureReleaseRef( pDbgMipTex->nTextureID );
		pDbgMipTex->nTextureID = INVALID_ID;
	}

	gtDebugMipTextures.Clear();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_GetDebugMipTextureOverride( D3D_TEXTURE * pTexture, LPD3DCBASETEXTURE * ppTexture )
{
	ASSERT_RETINVALIDARG( pTexture && (LPD3DCTEXTURE2D)pTexture->pD3DTexture );
	ASSERT_RETINVALIDARG( ppTexture && *ppTexture );

	D3DRESOURCETYPE rType;
	dxC_GetResourceType( (*ppTexture), &rType );
	if ( rType == D3DRTYPE_TEXTURE )
	{
		LPD3DCTEXTURE2D pTex = (LPD3DCTEXTURE2D)*ppTexture;
		D3DC_TEXTURE2D_DESC tDesc;
		V_RETURN( dxC_Get2DTextureDesc( pTex, 0, &tDesc ) );
		UINT uSize = max( tDesc.Width, tDesc.Height );

		DEBUG_MIP_TEXTURE * pDbgMipTex;
		int nMipTexID;
		for ( nMipTexID = gtDebugMipTextures.GetFirst();
			nMipTexID != INVALID_ID;
			nMipTexID = gtDebugMipTextures.GetNextId( nMipTexID ) )
		{
			pDbgMipTex = gtDebugMipTextures.Get( nMipTexID );
			ASSERT_CONTINUE( pDbgMipTex );
			if ( pDbgMipTex->nResolution == (int)uSize )
				break;
		}
		ASSERT_RETFAIL( nMipTexID != INVALID_ID );
		D3D_TEXTURE* pMipTex = dxC_TextureGet( pDbgMipTex->nTextureID );
		ASSERT_RETFAIL(pMipTex);

		if ( e_GetRenderFlag( RENDER_FLAG_SET_LOD_ENABLED ) )
		{
			DX9_BLOCK( DWORD dwLOD = pTexture->pD3DTexture->GetLOD(); )
			DX10_BLOCK( DWORD dwLOD = dxC_GetNumMipLevels( pTexture->pD3DTexture ); )
			if ( dwLOD < ( DEBUG_MIP_TEXTURE_COUNT - 1 ) )
			{
				DX9_BLOCK( DWORD mLod = pMipTex->pD3DTexture->GetLOD(); )
				DX10_BLOCK( DWORD mLod = dxC_GetNumMipLevels( pMipTex->pD3DTexture ); )
				if ( mLod != dwLOD )
				{
					dxC_SetMaxLOD( pMipTex, dwLOD );
					//( *ppTexture )->SetLOD( dwLOD );
				}
			}
		}

		*ppTexture = (LPD3DCBASETEXTURE)pMipTex->pD3DTexture;
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_DebugDumpDrawLists()
{
	gbDumpDrawList = TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_DumpDrawListCommand( int nDrawList, D3D_DRAWLIST_COMMAND * pCommand )
{
	const int cnAllocSize = 4096;
	if ( !gtDumpDrawlist.pszBuf || gtDumpDrawlist.nUsed == 0 )
	{
		if ( !gtDumpDrawlist.pszBuf )
		{
			gtDumpDrawlist.pszBuf = (char*)MALLOC( NULL, cnAllocSize );
			gtDumpDrawlist.nAlloc = cnAllocSize;
			ASSERT_RETVAL( gtDumpDrawlist.pszBuf, E_OUTOFMEMORY );
		}
		PStrPrintf( gtDumpDrawlist.pszBuf, cnAllocSize, "DrawList,Command,ID,(name),nData,pData,pData2,Flags,MinX,MinY,MinZ,MaxX,MaxY,MaxZ,SortValue\n" );
		gtDumpDrawlist.nUsed  = PStrLen( gtDumpDrawlist.pszBuf );
	}

	char szDrawLists[ NUM_DRAWLISTS ][64] =
	{
		"DRAWLIST_PRIMARY",
		"DRAWLIST_SCREEN",
		"DRAWLIST_INTERFACE",
		"DRAWLIST_SCRATCH",
		"DRAWLIST_OPTIMIZED",
		"DRAWLIST_MESH",
	};

	char szCommand[ NUM_DRAWLIST_COMMANDS+1 ][64] = {
		"Invalid ID",
		"Draw",
		"Draw bounding box",
		"Draw shadow",
		"Draw depth",
		"Draw layer",
		"Set clip",
		"Reset clip",
		"Set scissor",
		"Reset scissor",
		"Set viewport",
		"Reset viewport",
		"Reset full viewport",
		"Set render/depth targets",
		"Clear color",
		"Clear color/depth",
		"Clear depth",
		"Copy backbuffer",
		"Copy rendertarget",
		"Set state",
		"Reset state",
		"Callback",
		"Draw behind",
		"Fence",
	};

	char szStates[ NUM_DLSTATES+1 ][64] = {
		"Invalid",
		"Override shadertype",
		"Override envdef",
		"View matrix",
		"Proj matrix",
		"Proj2 matrix",
		"Eye location",
		"'Do draw' flags",
		"'Don't draw' flags",
		"Use lights",
		"Allow invisible models",
		"No backup shaders",
		"No depth test",
		"Not force loading material",
		"Viewport"
	};

	DWORD dwPData = *(DWORD*)&pCommand->pData;
	DWORD dwPData2 = *(DWORD*)&pCommand->pData2;

	char szLine[ cnAllocSize ];
	char szID[ 16 ];
	char * pszID;
	if ( pCommand->nCommand == DLCOM_SET_STATE )
	{
		pszID = szStates[ pCommand->nID + 1 ];
	}
	else
	{
		PIntToStr(szID, 16, pCommand->nID);
		pszID = szID;
	}

	char * pszName = NULL;
	if ( pCommand->nCommand == DLCOM_DRAW ||
		pCommand->nCommand == DLCOM_DRAW_BOUNDING_BOX ||
		pCommand->nCommand == DLCOM_DRAW_DEPTH ||
		pCommand->nCommand == DLCOM_DRAW_SHADOW )
	{		
		if ( pCommand->nID != INVALID_ID )
		{
			D3D_MODEL* pModel = dx9_ModelGet( pCommand->nID );
			if ( pModel )
			{
				int nModelDef = e_ModelGetDefinition( pCommand->nID );
				D3D_MODEL_DEFINITION* pModelDef = dxC_ModelDefinitionGet( nModelDef, 
					e_ModelGetDisplayLOD( pCommand->nID ) );

				if ( pModelDef )
				{
					pszName = pModelDef->tHeader.pszName;
				}
			}
		}
	}
	PStrPrintf( szLine, cnAllocSize, "%s,%s,%s,%s,%d,%08x,%08x,%08x,%f,%f,%f,%f,%f,%f,%d\n",
		szDrawLists[nDrawList],
		szCommand[pCommand->nCommand+1],
		pszID,
		pszName ? pszName : "",
		pCommand->nData,
		*(DWORD*)&dwPData,
		*(DWORD*)&dwPData2,
		pCommand->qwFlags,
		pCommand->bBox.vMin.fX,
		pCommand->bBox.vMin.fY,
		pCommand->bBox.vMin.fZ,
		pCommand->bBox.vMax.fX,
		pCommand->bBox.vMax.fY,
		pCommand->bBox.vMax.fZ,
		pCommand->nSortValue );

	int nLen = PStrLen( szLine );
	if ( gtDumpDrawlist.nUsed + nLen >= gtDumpDrawlist.nAlloc )
	{
		gtDumpDrawlist.pszBuf = (char*)REALLOC( NULL, gtDumpDrawlist.pszBuf, gtDumpDrawlist.nAlloc + cnAllocSize );
		gtDumpDrawlist.nAlloc += cnAllocSize;
		ASSERT_RETVAL( gtDumpDrawlist.pszBuf, E_OUTOFMEMORY );
	}
	PStrCat( gtDumpDrawlist.pszBuf, szLine, gtDumpDrawlist.nAlloc );
	gtDumpDrawlist.nUsed += nLen;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_DumpQuery( const char * pszEvent, const int nModelID, const int nInt, const void * pPtr )
{
	const int cnAllocSize = 4096;
	if ( !gtDumpDrawlist.pszBuf || gtDumpDrawlist.nUsed == 0 )
	{
		if ( !gtDumpDrawlist.pszBuf )
		{
			gtDumpDrawlist.pszBuf = (char*)MALLOC( NULL, cnAllocSize );
			gtDumpDrawlist.nAlloc = cnAllocSize;
			ASSERT_RETVAL( gtDumpDrawlist.pszBuf, E_OUTOFMEMORY );
		}
		PStrPrintf( gtDumpDrawlist.pszBuf, cnAllocSize, "Event,ModelID,nData,pData,Rendering,Waiting\n" );
		gtDumpDrawlist.nUsed  = PStrLen( gtDumpDrawlist.pszBuf );
	}

	char szLine[ cnAllocSize ];

	PStrPrintf( szLine, cnAllocSize, "%s,%d,%d,0x%08p,%d,%d\n",
		pszEvent,
		nModelID,
		nInt,
		pPtr,
		gnOcclusionQueriesRendering,
		gnOcclusionQueriesWaiting );

	int nLen = PStrLen( szLine );
	if ( gtDumpDrawlist.nUsed + nLen >= gtDumpDrawlist.nAlloc )
	{
		gtDumpDrawlist.pszBuf = (char*)REALLOC( NULL, gtDumpDrawlist.pszBuf, gtDumpDrawlist.nAlloc + cnAllocSize );
		gtDumpDrawlist.nAlloc += cnAllocSize;
		ASSERT_RETVAL( gtDumpDrawlist.pszBuf, E_OUTOFMEMORY );
	}
	PStrCat( gtDumpDrawlist.pszBuf, szLine, gtDumpDrawlist.nAlloc );
	gtDumpDrawlist.nUsed += nLen;

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_DumpDrawListBuffer()
{
	OS_PATH_CHAR szLog[ MAX_PATH ];
	PStrPrintf( szLog, _countof(szLog), OS_PATH_TEXT("%s%s"), LogGetRootPath(), OS_PATH_TEXT("datadump.csv") );
	BOOL bResult = FileSave( szLog, gtDumpDrawlist.pszBuf, gtDumpDrawlist.nUsed );
	ASSERT_RETFAIL( bResult && "Failed to save draw list command dump file!" );	

	FREE( NULL, gtDumpDrawlist.pszBuf );
	gtDumpDrawlist.pszBuf = NULL;
	gtDumpDrawlist.nUsed = 0;
	gtDumpDrawlist.nAlloc = 0;
	gbDumpDrawList = FALSE;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_DebugInit()
{
	ArrayInit(gtDebugMipTextures, NULL, DEBUG_MIP_TEXTURE_COUNT);
	ArrayInit(gtRender2DAABBs, NULL, 8);

	//gtUploadGraph.Init();
	//E_RECT tRect;
	//tRect.Set( 100, 100, 500, 400 );
	//gtUploadGraph.SetRect( tRect );

	//V( gtUploadGraph.AddLine( 0, L"Test", 0xffff0000 ) );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL dx9_IsDebugRuntime()
{
	static int nDebugRuntime = -1;

	if ( nDebugRuntime < 0 )
	{
		HKEY key = NULL;
		if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Direct3D"), 0, KEY_QUERY_VALUE, &key ) == ERROR_SUCCESS )
		{
			DWORD dwValue = 0;
			DWORD dwBufLen = sizeof( DWORD );
			if( RegQueryValueEx( key, _T("LoadDebugRuntime"), NULL, NULL, (LPBYTE)&dwValue, &dwBufLen ) == ERROR_SUCCESS )
				nDebugRuntime = (int)dwValue;
			RegCloseKey(key);
		}
	}	

	return ( nDebugRuntime == 1 );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_DebugDestroy()
{
	gtDebugMipTextures.Destroy();
	gtRender2DAABBs.Destroy();
	dxC_DebugTextureGetShaderResource() = NULL;

	//gtUploadGraph.Destroy();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowLightSlotInfo( int nSlot )
{
	const int cnSize = 512;
	WCHAR szInfo[ cnSize ];

	VECTOR vFocus;
	e_GetViewFocusPosition( &vFocus );

	LIGHT_SLOT_CONTAINER * pContainer = &gtLightSlots[ nSlot ];
	for ( int i = 0; i < pContainer->nSlotsActive; i++ )
	{
		if ( pContainer->pSlots[ i ].eState != LSS_ACTIVE )
			continue;
		D3D_LIGHT * pLight = dx9_LightGet( pContainer->pSlots[ i ].nLightID );
		if ( !pLight )
			continue;

		//VECTOR vMid = ( pLight->vPosition + vCenter ) * 0.5f;
		float fDist = VectorDistanceSquared( pLight->vPosition, vFocus );
		fDist = sqrtf( fDist );
		//float fStrength = pLight->vFalloff.x - fDist * pLight->vFalloff.y;
		//fStrength = min( fStrength, 1.0f );

		LIGHT_DEFINITION * pLightDef = NULL;
		if ( pLight->nLightDef != INVALID_ID )
			pLightDef = (LIGHT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_LIGHT, pLight->nLightDef );

		PStrPrintf( szInfo, cnSize, L"ID: %d DefID: %d\nName: (%S)\nPos: (%1.1f, %1.1f, %1.1f)\nRad: %3.1f\nDst: %3.1f\nPri: %1.1f",
			pLight->nId,
			pLight->nLightDef,
			pLightDef ? pLightDef->tHeader.pszName : "",
			pLight->vPosition.x,
			pLight->vPosition.y,
			pLight->vPosition.z,
			pLight->fFalloffEnd,
			fDist,
			pLight->fPriority );
		V( e_UIAddLabel( szInfo, &pLight->vPosition, TRUE, 1.0f, 0xffffffff, UIALIGN_TOP, UIALIGN_TOPLEFT, TRUE ) );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int dxC_DebugTrackMesh(
	const D3D_MODEL * pModel,
	const D3D_MODEL_DEFINITION * pModelDef )
{
#if ISVERSION(DEVELOPMENT)

	ASSERT_RETFALSE( pModel );
	ASSERT_RETFALSE( pModelDef );

	if ( ! e_GetRenderFlag( RENDER_FLAG_ISOLATE_MESH ) )
		return INVALID_ID;

	int nShowModel = e_GetRenderFlag( RENDER_FLAG_MODEL_RENDER_INFO );
	if ( nShowModel != pModel->nId )
		return INVALID_ID;

	int nMesh = DebugKeyGetValue();
	if ( DebugKeyGetValue() < 0 )
		DebugKeySetValue( 0 );
	if ( DebugKeyGetValue() >= pModelDef->nMeshCount )
		DebugKeySetValue( pModelDef->nMeshCount - 1 );

	return DebugKeyGetValue();

#endif // DEVELOPMENT
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dx9_DebugTrackMeshRenderInfo(
	int nModelID,
	int nMeshIndex,
	int nMaterial,
	struct EFFECT_TECHNIQUE_REF & tFXRef,
	float fDistToEyeSqr )
{
#if ISVERSION(DEVELOPMENT)

	int nShowModel = e_GetRenderFlag( RENDER_FLAG_MODEL_RENDER_INFO );
	if ( nShowModel != nModelID )
		return;

	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return;

	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
		e_ModelGetDisplayLOD( nModelID ) );
	if ( ! pModelDef )
		return;

	int nMesh = DebugKeyGetValue();
	if ( DebugKeyGetValue() < 0 )
		DebugKeySetValue( 0 );
	if ( DebugKeyGetValue() >= pModelDef->nMeshCount )
		DebugKeySetValue( pModelDef->nMeshCount - 1 );

	if ( nMeshIndex != DebugKeyGetValue() )
		return;

	gtDebugMeshRenderInfo.nModelID			= nModelID;
	gtDebugMeshRenderInfo.nMeshIndex		= nMeshIndex;
	gtDebugMeshRenderInfo.nMaterial			= nMaterial;
	gtDebugMeshRenderInfo.tFXRef			= tFXRef;
	gtDebugMeshRenderInfo.fDistToEyeSqr		= fDistToEyeSqr;

#endif // DEVELOPMENT
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowMeshRenderInfo()
{
	const int cnSize = 1536;
	WCHAR szInfo[ cnSize ];

	int nModel = e_GetRenderFlag( RENDER_FLAG_MODEL_RENDER_INFO );
	if ( nModel == INVALID_ID )
		return;

	if ( ! e_ModelExists( nModel ) )
	{
		DebugKeyHandlerEnd();
		return;
	}

	D3D_MODEL * pModel = dx9_ModelGet( nModel );
	if ( ! pModel )
	{
		DebugKeyHandlerEnd();
		return;
	}

	int nLOD = e_ModelGetDisplayLOD( nModel );

	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, nLOD );
	if ( ! pModelDef )
	{
		DebugKeyHandlerEnd();
		return;
	}

	D3D_MESH_DEFINITION * pMesh = dx9_ModelDefinitionGetMesh( pModelDef, gtDebugMeshRenderInfo.nMeshIndex );
	if ( ! pMesh )
	{
		DebugKeyHandlerEnd();
		return;
	}
	VECTOR vPos = ( pModel->tRenderAABBWorld.vMin + pModel->tRenderAABBWorld.vMax ) * 0.5f;

	MATERIAL * pMaterial = (MATERIAL*)DefinitionGetById( DEFINITION_GROUP_MATERIAL, gtDebugMeshRenderInfo.nMaterial );
	D3D_EFFECT * pEffect = dxC_EffectGet( gtDebugMeshRenderInfo.tFXRef.nEffectID );
	float fDist = gtDebugMeshRenderInfo.fDistToEyeSqr > 0.f ? sqrtf( gtDebugMeshRenderInfo.fDistToEyeSqr ) : 0.f;
	const int cnFXInfoLen = 1024;
	WCHAR szFXInfo[ cnFXInfoLen ];
	V( dx9_GetTechniqueRefFeaturesString( szFXInfo, cnFXInfoLen, &gtDebugMeshRenderInfo.tFXRef ) );

	float fWorldSize = 0.f;
	V( dxC_ModelGetWorldSizeAvg( pModel, fWorldSize, pModelDef ) );
	float fScreensize = e_GetWorldDistanceBiasedScreenSizeByVertFOV( fDist, fWorldSize );

	PStrPrintf( szInfo, cnSize, L"\nModel: %4d  Mesh: %3d  ModelDef: %4d\n"
								L"Name: (%S)\n"
								L"  Diff [%4d] %S\n"
								L"  Norm [%4d] %S\n"
								L"  Ilum [%4d] %S\n"
								L"  Dif2 [%4d] %S\n"
								L"  Spec [%4d] %S\n"
								L"  Envm [%4d] %S\n"
								L"  Lght [%4d] %S\n"
								L"Tris:      %d\n"
								L"Mtrl:      %4d (%S)\n"
								L"DistToEye: %f\n"
								L"DistWght:  %f\n"
								L"Effect:    %4d (%S)\n"
								L"  Tech index:   %d\n"
								L"%s",
		gtDebugMeshRenderInfo.nModelID,
		gtDebugMeshRenderInfo.nMeshIndex,
		pModelDef->tHeader.nId,
		pModelDef->tHeader.pszName,
		e_ModelGetTexture( nModel, gtDebugMeshRenderInfo.nMeshIndex, TEXTURE_DIFFUSE, nLOD ),	// CHB 2006.11.28
		pMesh->pszTextures[ TEXTURE_DIFFUSE ],
		e_ModelGetTexture( nModel, gtDebugMeshRenderInfo.nMeshIndex, TEXTURE_NORMAL, nLOD ),	// CHB 2006.11.28
		pMesh->pszTextures[ TEXTURE_NORMAL ],
		e_ModelGetTexture( nModel, gtDebugMeshRenderInfo.nMeshIndex, TEXTURE_SELFILLUM, nLOD ),	// CHB 2006.11.28
		pMesh->pszTextures[ TEXTURE_SELFILLUM ],
		e_ModelGetTexture( nModel, gtDebugMeshRenderInfo.nMeshIndex, TEXTURE_DIFFUSE2, nLOD ),	// CHB 2006.11.28
		pMesh->pszTextures[ TEXTURE_DIFFUSE2 ],
		e_ModelGetTexture( nModel, gtDebugMeshRenderInfo.nMeshIndex, TEXTURE_SPECULAR, nLOD ),	// CHB 2006.11.28
		pMesh->pszTextures[ TEXTURE_SPECULAR ],
		e_ModelGetTexture( nModel, gtDebugMeshRenderInfo.nMeshIndex, TEXTURE_ENVMAP, nLOD ),	// CHB 2006.11.28
		pMesh->pszTextures[ TEXTURE_ENVMAP ],
		e_ModelGetTexture( nModel, gtDebugMeshRenderInfo.nMeshIndex, TEXTURE_LIGHTMAP, nLOD ),	// CHB 2006.11.28
		pMesh->pszTextures[ TEXTURE_LIGHTMAP ],
		pMesh->wTriangleCount,
		gtDebugMeshRenderInfo.nMaterial,
		pMaterial ? pMaterial->tHeader.pszName : "(none)",
		fDist,
		fScreensize,
		gtDebugMeshRenderInfo.tFXRef.nEffectID,
		pEffect ? pEffect->pszFXFileName : "(none)",
		gtDebugMeshRenderInfo.tFXRef.nIndex,
		szFXInfo
		 );
	VECTOR vScrPos = VECTOR( -0.95f, 0.95f, 0.f );
	V( e_UIAddLabel( szInfo, &vScrPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPLEFT, UIALIGN_TOPLEFT, TRUE ) );

	e_PrimitiveAddBox( 0,						pModel->tRenderOBBWorld, 0xff3f3f7f );
	e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP, pModel->tRenderOBBWorld, 0x007f7f7f );

	if ( AppIsHellgate() && dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) )
	{
		VECTOR vRagPos;
		BOOL bRagPos = e_ModelGetRagdollPosition( pModel->nId, vRagPos );
		if ( bRagPos )
		{
			BOUNDING_BOX tBBox;
			tBBox.vMin = vRagPos;
			tBBox.vMax = vRagPos;
			BoundingBoxExpandBySize( &tBBox, 0.03f );
			e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP, &tBBox, 0x007f7f7f );

			tBBox.vMin = pModel->vPosition;
			tBBox.vMax = pModel->vPosition;
			BoundingBoxExpandBySize( &tBBox, 0.03f );
			e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP, &tBBox, 0x007f7f7f );

			e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &pModel->vPosition, &vRagPos, 0x007f7f7f );
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowTextureInfo()
{
	int nTexture = e_GetRenderFlag( RENDER_FLAG_TEXTURE_INFO );
	if ( nTexture == INVALID_ID )
		return;

	if ( ! e_TextureIsValidID( nTexture ) )
	{
		DebugKeyHandlerEnd();
		return;
	}

	D3D_TEXTURE * pTexture = dxC_TextureGet( nTexture );
	if ( ! pTexture || ! pTexture->pD3DTexture )
	{
		DebugKeyHandlerEnd();
		return;
	}

	int nLevelCount = dxC_GetNumMipLevels( pTexture->pD3DTexture );

	int nLevel = DebugKeyGetValue();
	if ( DebugKeyGetValue() < 0 )
		DebugKeySetValue( 0 );
	if ( DebugKeyGetValue() >= nLevelCount )
		DebugKeySetValue( nLevelCount - 1 );
	nLevel = DebugKeyGetValue();


	// test
	dxC_SetMaxLOD( pTexture, nLevel );
	


	D3DC_TEXTURE2D_DESC tDesc;
	V( dxC_Get2DTextureDesc( pTexture, nLevel, &tDesc ) );

	TEXTURE_DEFINITION * pTextureDef = NULL;
	if ( pTexture->nDefinitionId != INVALID_ID )
		pTextureDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );

	const int cnSize = 1024;
	WCHAR szInfo[ cnSize ];

	char szFormat[32];
	dxC_TextureFormatGetDisplayStr( tDesc.Format, szFormat, 32 );

	PStrPrintf( szInfo, cnSize, L"Texture: %4d\n"
								L"Name   : (%S)\n"
								L"Level  : %2d/%d\n"
								L"Width  : %4d\n"
								L"Height : %4d\n"
								L"Format : %S\n"
								L"Usage  : %d\n"
								L"Pool   : %d",
		nTexture,
		pTextureDef ? pTextureDef->tHeader.pszName : "(none)",
		nLevel,
		nLevelCount - 1,
		tDesc.Width,
		tDesc.Height,
		szFormat,
		tDesc.Usage
#if defined(ENGINE_TARGET_DX9)
		,tDesc.Pool
#endif
		);

	VECTOR vPos = VECTOR( 0.f, -0.5f, 0.f );
	V( e_UIAddLabel( szInfo, &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOP, UIALIGN_LEFT, TRUE ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowModelLightInfo( int nModelID )
{
	if ( nModelID == INVALID_ID )
		return;

	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return;

	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
												e_ModelGetDisplayLOD( nModelID ) );
	if ( ! pModelDef )
		return;

	BOOL bSlots = FALSE;
	int nMaxEffectLights = MAX_POINT_LIGHTS_PER_EFFECT;

	int nMesh;
	for ( nMesh = 0; nMesh < pModelDef->nMeshCount; nMesh++ )
	{
		D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ nMesh ] );

		if ( ! dx9_MeshIsVisible( pModel, pMesh ) )
			continue;

		EFFECT_DEFINITION * pEffectDef = e_MaterialGetEffectDef( pMesh->nMaterialId, dx9_GetCurrentShaderType( NULL, NULL, pModel ) );
		if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_USE_GLOBAL_LIGHTS ) )
			bSlots = TRUE;
		else
		{
			//int nEffectLights = tTechnique.tFeatures.nInts[ TF_INT_MODEL_POINTLIGHTS ];;
			//if ( nMaxEffectLights < nEffectLights )
			//	nMaxEffectLights = nEffectLights;
		}
	}
	if ( nMesh < pModelDef->nMeshCount )
		bSlots = TRUE;

	VECTOR vCenter = ( pModel->tRenderAABBWorld.vMin + pModel->tRenderAABBWorld.vMax ) * 0.5f;
	const int cnSize = 512;
	WCHAR szInfo[ cnSize ];

	VECTOR vViewDir;
	//VECTOR vFocus;
	//e_GetViewFocusPosition( &vFocus );
	VECTOR vViewPos;
	e_GetViewPosition( &vViewPos );
	e_GetViewDirection( &vViewDir );
	VectorNormalize( vViewDir );

	DWORD dwHidden = 0xff00ff00;
	DWORD dwOnTop  = 0x000000ff;

	if ( bSlots )
	{
		{
			// show light slot info
			LIGHT_SLOT_CONTAINER * pContainer = &gtLightSlots[ LS_LIGHT_DIFFUSE_POINT ];
			for ( int i = 0; i < pContainer->nSlotsActive; i++ )
			{
				if ( pContainer->pSlots[ i ].eState != LSS_ACTIVE )
					continue;
				D3D_LIGHT * pLight = dx9_LightGet( pContainer->pSlots[ i ].nLightID );
				if ( !pLight )
					continue;

				V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &pLight->vPosition, &vCenter, dwOnTop ) );
				V( e_PrimitiveAddLine( 0, &pLight->vPosition, &vCenter, dwHidden ) );
			}

			sDebugShowLightSlotInfo( LS_LIGHT_DIFFUSE_POINT );
		}

		{
			// specular slot info
			DWORD dwSpecHidden = 0xffff0000;
			DWORD dwSpecOnTop  = 0x0000ff00;

			LIGHT_SLOT_CONTAINER * pContainer = &gtLightSlots[ LS_LIGHT_SPECULAR_ONLY_POINT ];
			for ( int i = 0; i < pContainer->nSlotsActive; i++ )
			{
				if ( pContainer->pSlots[ i ].eState != LSS_ACTIVE )
					continue;
				D3D_LIGHT * pLight = dx9_LightGet( pContainer->pSlots[ i ].nLightID );
				if ( !pLight )
					continue;

				DWORD dwTempOnTop  = dwSpecOnTop;
				DWORD dwTempHidden = dwSpecHidden;
				//if ( i >= nMaxEffectLights )
				//{
				//	// "unselected" coloring
				//	dwTempOnTop = 0x004f4f4f;
				//	dwTempHidden = 0xff4f4f4f;
				//}

				V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &pLight->vPosition, &vCenter, dwTempOnTop ) );
				V( e_PrimitiveAddLine( 0, &pLight->vPosition, &vCenter, dwTempHidden ) );

				VECTOR vMid = ( pLight->vPosition + vCenter ) * 0.5f;
				float fDist = VectorDistanceSquared( pLight->vPosition, /*vFocus*/ vViewPos );
				fDist = sqrtf( fDist );

				float fStrength = e_LightSpecularCalculateWeight(
					pLight,
					//vFocus,
					vViewPos,
					vViewDir );
				fStrength = max( fStrength, 0.f );

				LIGHT_DEFINITION * pLightDef = NULL;
				if ( pLight->nLightDef != INVALID_ID )
					pLightDef = (LIGHT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_LIGHT, pLight->nLightDef );

				PStrPrintf( szInfo, cnSize, L"ID: %d DefID: %d\nName: (%S)\nPos: (%1.1f, %1.1f, %1.1f)\nRad: %-3.1f\nDst: %-3.1f\nStr: %-3.1f\nPri: %-3.1f",
					pLight->nId,
					pLight->nLightDef,
					pLightDef ? pLightDef->tHeader.pszName : "",
					pLight->vPosition.x,
					pLight->vPosition.y,
					pLight->vPosition.z,
					pLight->fFalloffEnd,
					fDist,
					fStrength,
					pLight->fPriority );
				V( e_UIAddLabel( szInfo, &vMid, TRUE, 1.0f, 0xffffffff, UIALIGN_TOP, UIALIGN_TOPLEFT, TRUE ) );
			}
		}
	}
	else
	{
		BOUNDING_SPHERE tBS( pModel->tRenderAABBWorld );

		DWORD dwMustHaveFlags = LIGHT_FLAG_DEFINITION_APPLIED;
		DWORD dwDontWantFlags = LIGHT_FLAG_NODRAW | LIGHT_FLAG_DEAD | LIGHT_FLAG_NO_APPLY | LIGHT_FLAG_SPECULAR_ONLY;
		if ( ! e_OptionState_GetActive()->get_bDynamicLights() )
			dwMustHaveFlags |= LIGHT_FLAG_STATIC;

		// show actor light info
		int pnLights[ MAX_LIGHTS_PER_EFFECT ];
		int nLights;
		V( dx9_MakePointLightList(
			pnLights,
			nLights,
			pModel->vPosition,
			dwMustHaveFlags,
			dwDontWantFlags,
			MAX_LIGHTS_PER_EFFECT ) );

		V( e_LightSelectAndSortPointList(
			tBS,
			pnLights,
			nLights,
			pnLights,
			nLights,
			MAX_LIGHTS_PER_EFFECT,
			0,
			0 ) );

		for ( int i = 0; i < nLights; i++ )
		{
			D3D_LIGHT * pLight = dx9_LightGet( pnLights[ i ] );
			if ( !pLight )
				continue;

			DWORD dwTempOnTop  = dwOnTop;
			DWORD dwTempHidden = dwHidden;
			if ( i >= nMaxEffectLights )
			{
				// "unselected" coloring
				dwTempOnTop = 0x004f4f4f;
				dwTempHidden = 0xff4f4f4f;
			}

			V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &pLight->vPosition, &vCenter, dwTempOnTop ) );
			V( e_PrimitiveAddLine( 0, &pLight->vPosition, &vCenter, dwTempHidden ) );

			VECTOR vMid = ( pLight->vPosition + vCenter ) * 0.5f;
			float fDist = VectorDistanceSquared( pLight->vPosition, vCenter );
			fDist = sqrtf( fDist );
			float fStrength = pLight->vFalloff.x - fDist * pLight->vFalloff.y;
			fStrength = SATURATE( fStrength );

			LIGHT_DEFINITION * pLightDef = NULL;
			if ( pLight->nLightDef != INVALID_ID )
				pLightDef = (LIGHT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_LIGHT, pLight->nLightDef );

			PStrPrintf( szInfo, cnSize, L"ID: %d DefID: %d\nName: (%S)\nPos: (%1.1f, %1.1f, %1.1f)\nRad: %-3.1f\nDst: %-3.1f\nStr: %-3.1f\nPri: %-3.1f",
				pLight->nId,
				pLight->nLightDef,
				pLightDef ? pLightDef->tHeader.pszName : "",
				pLight->vPosition.x,
				pLight->vPosition.y,
				pLight->vPosition.z,
				pLight->fFalloffEnd,
				fDist,
				fStrength,
				pLight->fPriority );
			V( e_UIAddLabel( szInfo, &vMid, TRUE, 1.0f, 0xffffffff, UIALIGN_TOP, UIALIGN_TOPLEFT, TRUE ) );
		}
	}
	PStrPrintf( szInfo, cnSize, L"%d\nSelected", pModel->nId );
	V( e_UIAddLabel( szInfo, &vCenter, TRUE, 1.5f, 0xffffffff, UIALIGN_TOP, UIALIGN_TOP, TRUE ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowEnvironmentInfo()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_ENVIRONMENT_INFO ) )
		return;

	const int cnSize = 512;
	WCHAR szInfo[ cnSize ];

	ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	if ( ! pEnvDef )
		return;

	PStrPrintf( szInfo, cnSize, L"Environment: %4d  Name: (%S)\n"
								L"Skybox     : %4d  Name: (%S)\n"
								L"Env map    : %4d  Name: (%S)\n"
								L"App SH     : %4d  Name: (%S)\n"
								L"Bg SH      : %4d  Name: (%S)\n"
								L"Location   : %d\n"
								L"Fog Start  : %d\n"
								L"Silhouette : %d\n"
								L"Clip       : %d\n"
								L"Flags      : 0x%08x\n"
								L"DefFlags   : 0x%08x",
		pEnvDef->tHeader.nId,
		pEnvDef->tHeader.pszName,
		pEnvDef->nSkyboxDefID,
		pEnvDef->szSkyBoxFileName,
		pEnvDef->nEnvMapTextureID,
		pEnvDef->szEnvMapFileName,
		BOOL(pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_HAS_APP_SH_COEFS),
		pEnvDef->szAppearanceLightingEnvMapFileName,
		BOOL(pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_HAS_BG_SH_COEFS),
		pEnvDef->szBackgroundLightingEnvMapFileName,
		pEnvDef->nLocation,
		pEnvDef->nFogStartDistance,
		pEnvDef->nSilhouetteDistance,
		pEnvDef->nClipDistance,
		pEnvDef->dwFlags,
		pEnvDef->dwDefFlags	);

	VECTOR vPos = VECTOR( 0.f, 0.5f, 0.f );
	V( e_UIAddLabel( szInfo, &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOP, UIALIGN_TOPLEFT, TRUE ) );


	// Disabled because it doesn't work correctly
	if ( 0 )
	{
		VECTOR vOrigin, vViewPos;
		V( e_GetViewFocusPosition( &vOrigin ) );
		V( e_GetViewPosition( &vViewPos ) );
		float fMaxRad = 0.125f;
		float fConeAngle = PI / 4;
		float fConeLen = 0.2f; // % of full scale
		float fScale = 0.4f * sqrtf( VectorDistanceSquared( vOrigin, vViewPos ) );

		WCHAR wszDirs[ NUM_ENV_DIR_LIGHTS ][16] = 
		{
			L"\n\n  PDif",
			L"\n\n  PSpc",
			L"\n\n  SDif",
		};

		for ( int nDir = 0; nDir < NUM_ENV_DIR_LIGHTS; nDir++ )
		{
			VECTOR vDir;
			e_GetDirLightVector( pEnvDef, (ENV_DIR_LIGHT_TYPE)nDir, &vDir );
			VECTOR4 vColor;
			e_GetDirLightColor( pEnvDef, (ENV_DIR_LIGHT_TYPE)nDir, &vColor );
			DWORD dwColor = ARGB_MAKE_FROM_VECTOR4( vColor );
			float fIntensity = e_GetDirLightIntensity( pEnvDef, (ENV_DIR_LIGHT_TYPE)nDir );

			VECTOR vEnd = vDir * fScale + vOrigin;
			V( e_PrimitiveAddCylinder( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_RENDER_ON_TOP, 
				&vOrigin,
				&vEnd,
				fIntensity * fMaxRad * fScale,
				dwColor ) );
			VECTOR vTip = vEnd + ( vDir * fConeLen * fMaxRad * fScale );
			V( e_PrimitiveAddCone( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_RENDER_ON_TOP,
				&vTip,
				&vEnd,  
				fConeAngle,
				dwColor ) );

			V( e_UIAddLabel( wszDirs[ nDir ], &vEnd, TRUE, 2.5f, 0xffffffff, UIALIGN_TOPLEFT ) );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowViewerInfo()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_VIEWER_INFO ) )
		return;

	const int cnBuf = 4096;
	WCHAR wszStats[ cnBuf ];
	V( e_ViewersGetStats( wszStats, cnBuf ) );

	VECTOR vPos = VECTOR( 0.f, 0.f, 0.f );
	V( e_UIAddLabel( wszStats, &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_CENTER, UIALIGN_TOPLEFT, TRUE ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowRenderTargetList()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_RENDERTARGET_LIST ) )
		return;

	const int cnSize = 2048;
	WCHAR szInfo[ cnSize ];

	dxC_DebugRenderTargetsGetList( szInfo, cnSize );

	VECTOR vPos = VECTOR( 0.98f, -0.75f, 0.f );
	V( e_UIAddLabel( szInfo, &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_BOTTOMRIGHT, UIALIGN_TOPLEFT, TRUE ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowRenderTargetInfo()
{
	int nRT = e_GetRenderFlag( RENDER_FLAG_RENDERTARGET_INFO );
	if ( nRT == INVALID_ID )
		return;

	dxC_DebugTextureGetShaderResource() = dxC_RTShaderResourceGet( (RENDER_TARGET_INDEX)nRT );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowAxes()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_AXES ) )
		return;

	VECTOR vOrigin, vPos;
	V( e_GetViewFocusPosition( &vOrigin ) );
	V( e_GetViewPosition( &vPos ) );

	MATRIX mWorld;
	MatrixIdentity( &mWorld );

	V( e_DebugRenderAxes( mWorld, 0.15f * sqrtf( VectorDistanceSquared( vOrigin, vPos ) ), &vOrigin ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowCells()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_SHOW_CELLS ) )
		return;

	V( dxC_CellDebugRenderAll() );
	V( dxC_PortalDebugRenderAll() );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowCameraInfo()
{
	if ( e_GetRenderFlag( RENDER_FLAG_SHOW_CAMERA_INFO ) == INVALID_ID )
		return;

	Viewer * pViewer = NULL;
	V( dxC_ViewerGet( e_GetRenderFlag( RENDER_FLAG_SHOW_CAMERA_INFO ), pViewer ) );
	if ( ! pViewer )
		return;

	VECTOR vPos = pViewer->tCameraInfo.vPosition;
	VECTOR vFoc = pViewer->tCameraInfo.vLookAt;
	VECTOR vDir = vFoc - vPos;
	VectorNormalize( vDir );
	float fVFOVDeg = RAD_TO_DEG( pViewer->tCameraInfo.fVerticalFOV );

	int nCell = pViewer->nCameraCell;
	int nRoom = pViewer->nCameraRoom;

	const int cnBufLen = 1024;
	WCHAR wszBuf[cnBufLen];

	PStrPrintf( wszBuf, cnBufLen, L"Camera:\n"
		L"%-10s ( %3.2f, %3.2f, %3.2f )\n"
		L"%-10s ( %3.2f, %3.2f, %3.2f )\n"
		L"%-10s ( %3.2f, %3.2f, %3.2f )\n"
		L"%-10s %3.1f\n"
		L"%-10s %d\n"
		L"%-10s %d\n"
		,
		L"Position",	vPos.x, vPos.y, vPos.z,
		L"Focus",		vFoc.x, vFoc.y, vFoc.z,
		L"Direction",	vDir.x, vDir.y, vDir.z,
		L"V FOV Deg",	fVFOVDeg,
		L"Cell",		nCell,
		L"Room",		nRoom
		);

	VECTOR vUIPos( 0.95f, 0.95f, 0.f );
	V( e_UIAddLabel( wszBuf, &vUIPos, FALSE, 0.5f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_LEFT ) );

	return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowPerf()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_SHOW_PERF ) )
		return;

	VECTOR vPos;
	const int cnBufLen = 2048;
	WCHAR wszBuf[cnBufLen];

	float fDrawFrameRate = AppCommonGetDrawFrameRate();
	DWORD dwSlowestFrameMs = AppCommonGetDrawLongestFrameMs();
	DWORD dwAvgFrameMs = (DWORD)( MSECS_PER_SEC / fDrawFrameRate );
	DWORD dwCPUAvgMs, dwCPUMaxMs;
	V( dxC_CPUTimerGetCounts( &dwCPUAvgMs, &dwCPUMaxMs ) );
	int nSyncTimeMS = static_cast<int>(CHB_TimingGetInstantaneous(CHB_TIMING_SYNC) * 1000 + 0.5f);
	DWORD dwGPUAvgMs, dwGPUMaxMs, dwGPUStaleFrames;
	V( dxC_GPUTimerGetCounts( &dwGPUAvgMs, &dwGPUMaxMs, dwGPUStaleFrames ) );
	float fCPUFPS = dwCPUAvgMs == 0 ? 0.f : (float)MSECS_PER_SEC / dwCPUAvgMs;
	float fGPUFPS = dwGPUAvgMs == 0 ? 0.f : (float)MSECS_PER_SEC / dwGPUAvgMs;
	DWORD dwSumTime = dwCPUAvgMs + dwGPUAvgMs;
	DWORD dwMaxTime = MAX( dwCPUAvgMs, dwGPUAvgMs );
	// Both parallelism and efficiency are 100% when CPU time + GPU time is equal to twice the frame time.
	// Parallelism drops as the max of CPU and GPU is less than the frame time.
	float fHalfFrame = 0.5f * dwAvgFrameMs;
	int nParallelism = (int)( ( ( dwMaxTime - fHalfFrame ) / ( dwAvgFrameMs - fHalfFrame ) ) * 100 );
	nParallelism = MIN( 100, MAX( 0, nParallelism ) );
	// Efficiency drops as the sum of CPU and GPU is less than twice the frame time.
	int nEfficiency = (int)( ( float( dwSumTime - dwAvgFrameMs ) / dwAvgFrameMs ) * 100 );
	nEfficiency = MIN( 100, MAX( 0, nEfficiency ) );

	int nParticlesDrawn   = e_ParticlesGetDrawnTotal();
	float fParticleWorldArea  = e_ParticlesGetTotalWorldArea();
	float fParticleScreenAreaMeasured = e_ParticlesGetTotalScreenArea( PARTICLE_DRAW_SECTION_INVALID, FALSE );
	float fParticleScreenAreaPredicted = e_ParticlesGetTotalScreenArea( PARTICLE_DRAW_SECTION_INVALID, TRUE );

	UINT nActualSetVSF, nTotalSetVSF;
	UINT nActualSetPSF, nTotalSetPSF;
	UINT nActualSetVS, nTotalSetVS;
	UINT nActualSetPS, nTotalSetPS;
	UINT nActualSetVD, nTotalSetVD;
	V( dxC_StateGetStats( STATE_SET_VERTEX_SHADER_CONSTANT_F,	nActualSetVSF,	nTotalSetVSF ) );
	V( dxC_StateGetStats( STATE_SET_PIXEL_SHADER_CONSTANT_F,	nActualSetPSF,	nTotalSetPSF ) );
	V( dxC_StateGetStats( STATE_SET_VERTEX_SHADER,				nActualSetVS,	nTotalSetVS ) );
	V( dxC_StateGetStats( STATE_SET_PIXEL_SHADER,				nActualSetPS,	nTotalSetPS ) );
	V( dxC_StateGetStats( STATE_SET_VERTEX_DECLARATION,			nActualSetVD,	nTotalSetVD ) );

	ENGINE_MEMORY tMemManaged;
	int nPool = DXC_9_10( D3DPOOL_MANAGED, 0 );
	V( dxC_EngineMemoryGetPool( nPool, tMemManaged ) );

	struct
	{
		int nHits;
		int nBytes;
	} tUploadStats[] = 
	{
		{ CHB_StatsGetHits(CHB_STATS_TEXTURE_BYTES_UPLOADED),			CHB_StatsGetTotal(CHB_STATS_TEXTURE_BYTES_UPLOADED)			},
		{ CHB_StatsGetHits(CHB_STATS_VERTEX_BUFFER_BYTES_UPLOADED),		CHB_StatsGetTotal(CHB_STATS_VERTEX_BUFFER_BYTES_UPLOADED)	},
		{ CHB_StatsGetHits(CHB_STATS_INDEX_BUFFER_BYTES_UPLOADED),		CHB_StatsGetTotal(CHB_STATS_INDEX_BUFFER_BYTES_UPLOADED)	},
	};


	// groups:
	//	memory
	//	states
	//	timing
	//	particles
	//	batches

	// Timing
	PStrPrintf( wszBuf, cnBufLen, L"FPS[ %3.1f ]  MS[ avg: %4d  slowest: %4d ]\n"
		L"CPU[ %4dms %6.1ffps max:%4dms ]  PRESENT[ %4dms ]\n"
		L"GPU[ %4dms %6.1ffps max:%4dms ]  STALE[ %2d ]\n"
		L"Parallelism: %3d%%\n"
		L"Efficiency:  %3d%%",
		fDrawFrameRate,
		dwAvgFrameMs,
		dwSlowestFrameMs,
		dwCPUAvgMs,
		fCPUFPS,
		dwCPUMaxMs,
		nSyncTimeMS,
		dwGPUAvgMs,
		fGPUFPS,
		dwGPUMaxMs,
		dwGPUStaleFrames,
		nParallelism,
		nEfficiency );
	vPos = VECTOR( -0.95f, 0.95f );
	V( e_UIAddLabel( wszBuf, &vPos, FALSE, 0.5f, 0xffffffff, UIALIGN_TOPLEFT, UIALIGN_LEFT ) );


	// Batches
	e_GetBatchString(wszBuf, cnBufLen);
	vPos = VECTOR( -0.95f, 0.0f );
	V( e_UIAddLabel( wszBuf, &vPos, FALSE, 0.5f, 0xffffffff, UIALIGN_LEFT, UIALIGN_LEFT ) );


	// Memory
	PStrPrintf( wszBuf, cnBufLen, L"MANAGED:  Count    Bytes\n"
		L"%8s: %8d %10d\n"
		L"%8s: %8d %10d\n"
		L"%8s: %8d %10d\n"
		L"%8s: %8s %10d\n"
		L"\nUPLOADS:  Count    Bytes\n"
		//L"%8s: %8d %10d\n"
		L"%8s: %8d %10d\n"
		L"%8s: %8d %10d"
		,
		L"TEXTURES",	tMemManaged.dwTextures,		tMemManaged.dwTextureBytes,
		L"VBUFFERS",	tMemManaged.dwVBuffers,		tMemManaged.dwVBufferBytes,
		L"IBUFFERS",	tMemManaged.dwIBuffers,		tMemManaged.dwIBufferBytes,
		L"TOTAL",		L"",						tMemManaged.Total(),
		//L"TEXTURES",	tUploadStats[0].nHits,		tUploadStats[0].nBytes,
		L"VBUFFERS",	tUploadStats[1].nHits,		tUploadStats[1].nBytes,
		L"IBUFFERS",	tUploadStats[2].nHits,		tUploadStats[2].nBytes
		);
	vPos = VECTOR( 0.95f, 0.95f );
	V( e_UIAddLabel( wszBuf, &vPos, FALSE, 0.5f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_LEFT ) );


	// States
	PStrPrintf( wszBuf, cnBufLen, L"STATE: Actual Total\n"
		L"%6s %6d %6d\n"
		L"%6s %6d %6d\n"
		L"%6s %6d %6d\n"
		L"%6s %6d %6d\n"
		L"%6s %6d %6d",
		L"VS",		nActualSetVS,		nTotalSetVS,
		L"PS",		nActualSetPS,		nTotalSetPS,
		L"VSF",		nActualSetVSF,		nTotalSetVSF,
		L"PSF",		nActualSetPSF,		nTotalSetPSF,
		L"VDECL",	nActualSetVD,		nTotalSetVD );
	vPos = VECTOR( 0.95f, -0.65f );
	V( e_UIAddLabel( wszBuf, &vPos, FALSE, 0.5f, 0xffffffff, UIALIGN_BOTTOMRIGHT, UIALIGN_LEFT ) );


	// Particles
	PStrPrintf( wszBuf, cnBufLen, L"PARTICLES:\n"
		L"COUNT: Drawn[ %6d ]\n"
		L"AREA:  World[ %6.1f ]   Screen[ %6.1f / 6.1f ]",
		nParticlesDrawn,
		fParticleWorldArea,		
		fParticleScreenAreaMeasured, fParticleScreenAreaPredicted );
	vPos = VECTOR( 0.95f, 0.0f );
	V( e_UIAddLabel( wszBuf, &vPos, FALSE, 0.5f, 0xffffffff, UIALIGN_RIGHT, UIALIGN_LEFT ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowUploadInfo()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_UPLOAD_INFO ) )
		return;

	const int cnBufLen = 2048;
	WCHAR wszBuf[ cnBufLen ];

	RESOURCE_UPLOAD_MANAGER & tManager = e_UploadManagerGet();

	// Memory
	PStrPrintf( wszBuf, cnBufLen, L"UPLOADS:     Avg    Max\n"
		L"%8s: %8d/%-8d\n"
		L"%8s: %8d/%-8d\n"
		L"%8s: %8d/%-8d\n"
		L"%8s: %8d/%-8d\n"
		L"%8s: %8d"
		L"%8s: %8d",
		L"TEXTURE",	tManager.GetAvgRecentForType( RESOURCE_UPLOAD_MANAGER::TEXTURE ),		tManager.GetMaxRecentForType( RESOURCE_UPLOAD_MANAGER::TEXTURE ),
		L"VBUFFER",	tManager.GetAvgRecentForType( RESOURCE_UPLOAD_MANAGER::VBUFFER ),		tManager.GetMaxRecentForType( RESOURCE_UPLOAD_MANAGER::VBUFFER ),
		L"IBUFFER",	tManager.GetAvgRecentForType( RESOURCE_UPLOAD_MANAGER::IBUFFER ),		tManager.GetMaxRecentForType( RESOURCE_UPLOAD_MANAGER::IBUFFER ),
		L"ALL",		tManager.GetAvgRecentAll(),												tManager.GetMaxRecentAll(),
		L"LIMIT",	tManager.GetLimit(),
		L"TOTAL",	tManager.GetTotal()
		 );
	VECTOR vPos = VECTOR( 0.f, 0.95f );
	V( e_UIAddLabel( wszBuf, &vPos, FALSE, 0.5f, 0xffffffff, UIALIGN_TOP, UIALIGN_LEFT ) );


	//V( gtUploadGraph.UpdateLine( 0, tManager.tMetrics[TEXTURE].dwFrameBytes. ) );
	//V( gtUploadGraph.Render() );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugShowWardrobeInfo()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_WARDROBE_INFO ) )
		return;

#if ISVERSION(DEVELOPMENT)
	const int cnBufLen = 2048;
	WCHAR wszBuf[ cnBufLen ];

	DWORD dwTextureCount = 0;
	DWORD dwTextureSize  = 0;
	DWORD dwSysmemCount  = 0;
	DWORD dwSysmemSize   = 0;
	DWORD dwCanvasCount  = 0;
	DWORD dwCanvasSize   = 0;

#define AVERAGE_FRAMES	30

	static RunningAverage<DWORD, AVERAGE_FRAMES>	tAvgTotalCount;
	static RunningAverage<DWORD, AVERAGE_FRAMES>	tAvgTotalSize;
	static RunningAverage<DWORD, AVERAGE_FRAMES>	tAvgSysmemCount;
	static RunningAverage<DWORD, AVERAGE_FRAMES>	tAvgSysmemSize;
	static RunningAverage<DWORD, AVERAGE_FRAMES>	tAvgCanvasCount;
	static RunningAverage<DWORD, AVERAGE_FRAMES>	tAvgCanvasSize;

	for ( D3D_TEXTURE * pTexture = dxC_TextureGetFirst();
		pTexture;
		pTexture = dxC_TextureGetNext( pTexture ) )
	{
		if ( pTexture->nGroup != TEXTURE_GROUP_WARDROBE )
			continue;

		if ( pTexture->ptSysmemTexture )
		{
			dwSysmemCount++;
			DWORD dwSize = pTexture->ptSysmemTexture->GetSize();
			dwSysmemSize += dwSize;
			dwTextureSize += dwSize;
		}

		if ( pTexture->pD3DTexture )
		{
			dwCanvasCount++;
			DWORD dwSize = 0;
			V( dxC_TextureGetSizeInMemory( pTexture, &dwSize ) );
			dwCanvasSize += dwSize;
			dwTextureSize += dwSize;
		}

		dwTextureCount++;
	}

	tAvgCanvasCount.Push( dwCanvasCount );
	tAvgCanvasSize.Push( dwCanvasSize );
	tAvgSysmemCount.Push( dwSysmemCount );
	tAvgSysmemSize.Push( dwSysmemSize );
	tAvgTotalCount.Push( dwTextureCount );
	tAvgTotalSize.Push( dwTextureSize );

	// Memory
	PStrPrintf( wszBuf, cnBufLen, 
		L"%16s: %8s %8s\n"
		L"%16s: %8d %8d\n"
		L"%16s: %8d %8d\n"
		L"%16s: %8d %8d\n"
		L"%16s: %8d %8d\n"
		L"%16s: %8d %8d\n"
		L"%16s: %8d %8d\n"
		,
		L"WARDROBE INFO",	L"AVG",									L"MAX",
		L"CANVAS COUNT",	tAvgCanvasCount.Avg(),					tAvgCanvasCount.Max(),
		L"CANVAS MB",		tAvgCanvasSize.Avg() / BYTES_PER_MB,	tAvgCanvasSize.Max() / BYTES_PER_MB,
		L"SYSMEM COUNT",	tAvgSysmemCount.Avg(),					tAvgSysmemCount.Max(),
		L"SYSMEM MB",		tAvgSysmemSize.Avg() / BYTES_PER_MB,	tAvgSysmemSize.Max() / BYTES_PER_MB,
		L"TOTAL COUNT",		tAvgTotalCount.Avg(),					tAvgTotalCount.Max(),
		L"TOTAL MB",		tAvgTotalSize.Avg() / BYTES_PER_MB,		tAvgTotalSize.Max() / BYTES_PER_MB
		);
	VECTOR vPos = VECTOR( 0.95f, 0.95f );
	V( e_UIAddLabel( wszBuf, &vPos, FALSE, 0.5f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_LEFT ) );
#undef AVERAGE_FRAMES

#endif	// DEVELOPMENT
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_DebugShowInfo()
{
	if ( e_GetRenderFlag( RENDER_FLAG_SHOW_MODEL_IDS ) )
	{
		for ( D3D_MODEL * pModel = dx9_ModelGetFirst();
			pModel;
			pModel = dx9_ModelGetNext( pModel ) )
		{
			if ( ! dx9_ModelIsVisible( pModel ) )
				continue;
			VECTOR vCenter = ( pModel->tRenderAABBWorld.vMin + pModel->tRenderAABBWorld.vMax ) * 0.5f;
			WCHAR szID[ 16 ];
			PStrPrintf( szID, 16, L"%d", pModel->nId );
			V( e_UIAddLabel( szID, &vCenter, TRUE, 1.5f, 0xffffffff, UIALIGN_CENTER, UIALIGN_CENTER, TRUE ) );
		}
	}

	if ( e_GetRenderFlag( RENDER_FLAG_SHOW_CAMERA_FOCUS ) )
	{
		const CAMERA_INFO * pCameraInfo = CameraGetInfo();
		if ( pCameraInfo )			
		{
			BOUNDING_BOX tBox;
			float fHalfSize = 0.25f;
			VECTOR vCorner( fHalfSize, fHalfSize, fHalfSize );
			tBox.vMin = pCameraInfo->vLookAt - vCorner;
			tBox.vMax = pCameraInfo->vLookAt + vCorner;
			
			V( e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL | PRIM_FLAG_WIRE_BORDER, 
				&tBox, 0x08ff0000 ) );
		}
	}	

	sDebugShowModelLightInfo( e_GetRenderFlag( RENDER_FLAG_DEBUG_LIGHT_INFO ) );
	sDebugShowMeshRenderInfo();
	sDebugShowRenderTargetList();
	sDebugShowRenderTargetInfo();
	sDebugShowTextureInfo();
	sDebugShowEnvironmentInfo();
	sDebugShowViewerInfo();
	sDebugShowAxes();
	sDebugShowPerf();
	sDebugShowCells();
	sDebugShowCameraInfo();
	sDebugShowUploadInfo();
	sDebugShowWardrobeInfo();


	//V( dxC_DebugRenderNormals( e_GetRenderFlag( RENDER_FLAG_MODEL_RENDER_INFO ) ) );



	// CHB 2007.03.04 - To match conditional in e_adclient.h
#if ISVERSION(DEVELOPMENT)
	if ( e_GetRenderFlag( RENDER_FLAG_ADOBJECT_INFO ) )
	{
		V( e_AdInstanceDebugRenderAll() );
	}
#endif

	// ui align test
	//VECTOR vPos = VECTOR( 0.f, 0.f, 0.f );
	//V( e_UIAddLabel( L"cccc", &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_CENTER, FALSE ) );
	//V( e_UIAddLabel( L"llll", &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_LEFT, FALSE ) );
	//V( e_UIAddLabel( L"rrrr", &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_RIGHT, FALSE ) );
	//V( e_UIAddLabel( L"tttt", &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOP, FALSE ) );
	//V( e_UIAddLabel( L"bbbb", &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_BOTTOM, FALSE ) );
	//V( e_UIAddLabel( L"blblblbl\n", &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_BOTTOMLEFT, FALSE ) );
	//V( e_UIAddLabel( L"\nbrbrbrbr", &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_BOTTOMRIGHT, FALSE ) );
	//V( e_UIAddLabel( L"tltltltl\n", &vPos, FALSE, 2.0f, 0xffffffff, UIALIGN_TOPLEFT, FALSE ) );
	//V( e_UIAddLabel( L"\ntrtrtrtr", &vPos, FALSE, 2.0f, 0xffffffff, UIALIGN_TOPRIGHT, FALSE ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_DebugSetShaderProfileRender( const SHADER_PROFILE_RENDER * pSettings )
{
#if ISVERSION(DEVELOPMENT) 

	if ( ! pSettings )
	{
		gtShaderProfileRender.nEffectID			= INVALID_ID;
		return S_OK;
	}

	gtShaderProfileRender = *pSettings;
	gfShaderProfileStatsCallbackElapsed = 0.f;

	// make sure we're in non-vsync mode
	if (e_OptionState_GetActive()->get_bFlipWaitVerticalRetrace())
	{
		CComPtr<OptionState> pState;
		V_RETURN(e_OptionState_CloneActive(&pState));
		pState->set_bFlipWaitVerticalRetrace(false);
		e_OptionState_CommitToActive(pState);
	}
#endif // DEVELOPMENT

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL dxC_DebugShaderProfileRender()
{
#if ISVERSION(DEVELOPMENT)

	if ( gtShaderProfileRender.nEffectID == INVALID_ID )
		return FALSE;
	D3D_EFFECT * pEffect = dxC_EffectGet( gtShaderProfileRender.nEffectID );
	if ( ! dxC_EffectIsLoaded( pEffect ) )
		return FALSE;
	if ( ! IS_VALID_INDEX( gtShaderProfileRender.nTechniqueIndex, pEffect->nTechniques ) )
		return FALSE;

	if ( gtShaderProfileRender.hWnd && gtShaderProfileRender.pfnCallback )
	{
		gfShaderProfileStatsCallbackElapsed += e_GetElapsedTime();

		if ( gfShaderProfileStatsCallbackElapsed > 1.f )
		{
			SHADER_PROFILE_STATS tStats;

			tStats.hWnd = gtShaderProfileRender.hWnd;
			tStats.fFramesPerSecond = AppCommonGetDrawFrameRate();
			tStats.nPixelsPerSecond = 0;

			gtShaderProfileRender.pfnCallback( &tStats );
			gfShaderProfileStatsCallbackElapsed = FMODF( gfShaderProfileStatsCallbackElapsed, 1.f );
		}
	}


	V( dx9_ResetSamplerStates() );
#ifdef ENGINE_TARGET_DX9
	V( dx9_RestoreD3DStates() ); 
#endif
	DX10_BLOCK( dx10_RestoreD3DStates(); )

	// reset device states
	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	dxC_SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	dxC_SetRenderState( D3DRS_ALPHAREF, 1L );
	dxC_SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
	dxC_SetRenderState( D3DRS_ZENABLE, FALSE );
	dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );
	V( dxC_ResetViewport() );
	dxC_SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	dxC_SetRenderState( D3DRS_STENCILENABLE, FALSE );
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );

	V_GOTO( failed, dxC_BeginScene() );

	DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );
	V_GOTO( failed, dxC_SetRenderTargetWithDepthStencil( SWAP_RT_FINAL_BACKBUFFER, eDT ) );
	V_GOTO( failed, dxC_ClearBackBufferPrimaryZ( D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000 ) );

	EFFECT_TECHNIQUE & tTechnique = pEffect->ptTechniques[ pEffect->pnTechniquesIndex[ gtShaderProfileRender.nTechniqueIndex ] ];

	// render just this shader

	for ( int nScreen = 0; nScreen < gtShaderProfileRender.nOverdraw; nScreen++ )
	{
		UINT nPasses;
		V_GOTO( failed, dxC_SetTechnique( pEffect, &tTechnique, &nPasses ) );

		for ( UINT nPass = 0; nPass < nPasses; nPass++ )
		{
			V_GOTO( failed, dxC_EffectBeginPass( pEffect, nPass ) );
			V_GOTO( failed, dx9_SetStaticBranchParamsFromFlags( pEffect, tTechnique, gtShaderProfileRender.dwBranches ) );
			for ( int i = 0; i < NUM_SAMPLER_TYPES; i++ )
			{
				D3D_TEXTURE * pTexture = dxC_TextureGet( e_GetUtilityTexture( TEXTURE_RGBA_FF ) );
				if ( pTexture && pTexture->pD3DTexture )
				{
					V_GOTO( failed, dxC_SetTexture( i, pTexture->GetShaderResourceView() ) );
				}
			}
			// override the vertex shader
			V_GOTO( failed, dx9_EffectSetDebugVertexShader( DEBUG_VS_PROFILE ) );

			V_GOTO( failed, dxC_DrawFullscreenQuad( FULLSCREEN_QUAD_UV, pEffect, tTechnique ) );
		}
	}

	V_GOTO( failed, dxC_EndScene() );

	DX9_BLOCK
	(
		V_GOTO( failed, dxC_GetD3DSwapChain()->Present( NULL, NULL, NULL, NULL, 0 ) );
	)
	DX10_BLOCK
	(
		V_GOTO( failed, dxC_GetD3DSwapChain()->Present(0, 0) );
	)

	return TRUE;

failed:
	// don't bother checking the output
	dxC_EndScene();

#endif // DEVELOPMENT

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class T>
void LINE_GRAPH_LINE<T>::Init( int nNumElements )
{
	tData.tBuffer = NULL;
	tData.Init( nNumElements );
	tLifePeak = (element_type)0;
	tLifeSum = (element_type)0;
}

//----------------------------------------------------------------------------

template <class T>
void LINE_GRAPH_LINE<T>::Destroy()
{
	tData.Destroy();
}

//----------------------------------------------------------------------------

template <class T>
void LINE_GRAPH_LINE<T>::SetName( const WCHAR * pszName )
{
	ASSERT_RETURN( pszName );
	PStrCopy( wszName, pszName, cnNameLen );
}

//----------------------------------------------------------------------------

template <class T>
void LINE_GRAPH_LINE<T>::Reset()
{
	tData.Zero();
}

//----------------------------------------------------------------------------

template <class T>
void LINE_GRAPH_LINE<T>::Update( const element_type & tValue )
{
	tData.Push( tValue );
	tLifePeak = MAX( tLifePeak, tValue );
	tLifeSum += tValue;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class T>
void LINE_GRAPH<T>::Init()
{
	tLines = (line_map*)MALLOC_NEW( NULL, line_map );
	ASSERT( tLines );
}

//----------------------------------------------------------------------------

template <class T>
void LINE_GRAPH<T>::Destroy()
{
	if ( tLines )
	{
		FREE_DELETE( NULL, tLines, line_map );
		tLines = NULL;
	}
}

//----------------------------------------------------------------------------

template <class T>
void LINE_GRAPH<T>::ResetCounts()
{
	ASSERT_RETURN( tLines );

	for ( line_map_itr i = tLines->begin();
		i != tLines->end();
		++i )
	{
		i->second.Reset();
	}
}

//----------------------------------------------------------------------------

template <class T>
void LINE_GRAPH<T>::SetRect( const E_RECT & tRect )
{
	tRenderRect = tRect;
}

//----------------------------------------------------------------------------

template <class T>
PRESULT LINE_GRAPH<T>::AddLine( int nID, const WCHAR * pszName, DWORD dwColor )
{
	ASSERT_RETFAIL( tLines );
	ASSERT_RETINVALIDARG( nID >= 0 );

	line_type * ptLine = NULL;

	DWORD dwID = static_cast<DWORD>(nID);

	ASSERT_RETFAIL(tLines->find(dwID) == tLines->end());

	ptLine = &(*tLines)[ dwID ];

	int nElements = tRenderRect.right - tRenderRect.left;
	ptLine->Init( nElements );
	ptLine->SetName( pszName );
	ptLine->SetColor( dwColor );
	ptLine->Reset();

	return S_OK;
}

//----------------------------------------------------------------------------

template <class T>
LINE_GRAPH_LINE<T> * LINE_GRAPH<T>::GetLine( int nID )
{
	ASSERT_RETNULL( tLines );
	ASSERT_RETNULL( nID >= 0 );

	DWORD dwID = static_cast<DWORD>(nID);

	line_map_itr i = tLines->find( dwID );
	ASSERT_RETNULL( i != tLines->end() );

	return &i->second;
}


//----------------------------------------------------------------------------

template <class T>
PRESULT LINE_GRAPH<T>::UpdateLine( int nID, const T & tValue )
{
	line_type * pLine = GetLine( nID );
	ASSERT_RETFAIL( pLine );

	pLine->Update( tValue );

	return S_OK;
}

//----------------------------------------------------------------------------

static void sScreenToPos( VECTOR2 & vPos, int x, int y )
{
	//vPos.x = x / (float)nRTW;
	//vPos.y = y / (float)nRTH;

	vPos.x = x;
	vPos.y = y;
}

template <class T>
PRESULT LINE_GRAPH<T>::Render()
{
	//int nRTW, nRTH;
	//V_RETURN( dxC_GetRenderTargetDimensions( nRTW, nRTH ) );

	VECTOR2 vUL, vLR;
	sScreenToPos( vUL, tRenderRect.left, tRenderRect.top );
	sScreenToPos( vLR, tRenderRect.right, tRenderRect.bottom );

	e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL | PRIM_FLAG_WIRE_BORDER, &vUL, &vLR, 0x7f3f3f3f );

	float fMax = 0.f;
	for ( line_map_itr i = tLines->begin();
		i != tLines->end();
		++i )
	{
		line_type & tLine = i->second;
		int nMax = (int)tLine.Graph().Max();
		nMax = MAX( 0, nMax );
		nMax = NEXTPOWEROFTWO( (DWORD)nMax );
		fMax = MAX( fMax, (float)nMax );
	}	

	VECTOR2 vE1, vE2;
	VECTOR2 vScale;
	vScale.x = 1.f / (vLR.x - vUL.x);
	vScale.y = (vLR.y - vUL.y) / fMax;

	for ( line_map_itr i = tLines->begin();
		i != tLines->end();
		++i )
	{
		line_type & tLine = i->second;
		float fScaleX = vScale.x * tLine.Graph().N;
		for ( int n = 1; n < tLine.Graph().N; ++n )
		{
			// make a line between n-1 and n
			vE1.x = fScaleX * (n-1) + vUL.x;
			vE2.x = fScaleX * (n  ) + vUL.x;
			vE1.y = vLR.y - ( vScale.y * (n-1) );
			vE2.y = vLR.y - ( vScale.y * (n  ) );

			e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &vE1, &vE2, tLine.GetColor() );
		}
	}

	return S_OK;
}
