//----------------------------------------------------------------------------
// dx9_profile.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_query.h"
#include "dxC_state.h"
#include "dxC_occlusion.h"
#include "appcommontimer.h"
#include "dxC_model.h"
#include "e_particles_priv.h"
#include "dx9_ui.h"

#include "dxC_profile.h"

#ifdef NVIDIA_PERFKIT_ENABLE
#include "nv_perfauth.h"
//#pragma comment lib "nv_perfauthST.lib"
#endif

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
BOOL gbTrackD3DResourceManagerStats = FALSE;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_GetStatesStatsString (
	WCHAR * pszString,
	int nSize )
{
	WCHAR * pszStats = dx9_GetStateStats( nSize );
	if ( pszStats && pszStats[ 0 ] )
		PStrCopy( pszString, pszStats, nSize );
	else
		PStrPrintf( pszString, nSize, L"State manager not enabled: state stats unavailable." );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dx9_TrackD3DResourceManagerStats()
{
	if ( ! gbTrackD3DResourceManagerStats )
		return;
	if ( ! gtStatQueries.pResourceManager )
		return;

	const int cnMinBytesDownloadedToReport = 0;
	const OUTPUT_PRIORITY tOP = OP_DEBUG;
	char szNumber[ 32 ];

	for ( int t = 0; t < gnStatsToDisplayCount; t++ )
	{
		DWORD dwBytesDownloaded = gtD3DStatResManagerDiff.stats[ gpnStatsToDisplay[ t ] ].ApproxBytesDownloaded;
		BOOL bThrashing = gtD3DStatResManagerDiff.stats[ gpnStatsToDisplay[ t ] ].bThrashing;

		PStrPrintf( szNumber, 32, "%d", dwBytesDownloaded );
		PStrGroupThousands( szNumber, 32 );
		if ( dwBytesDownloaded > cnMinBytesDownloadedToReport )
			DebugString( tOP, "D3D RM: %13s Upload: %15s bytes\n", gszStatsToDisplay[ t ], szNumber );

		if ( bThrashing )
			DebugString( tOP, "D3D RM: %13s thrashing!\n", gszStatsToDisplay[ t ] );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sPrintStats( WCHAR * pszOut, int nBufLen, const D3DRESOURCESTATS * pStats, const D3DRESOURCESTATS * pStatsDiff )
{
	WCHAR szBool[ 2 ][ 4 ] = { L"no", L"yes" };

	PStrPrintf( pszOut, nBufLen,
		L"    ApproxBytesDownloaded:%9d Thrashing: %-3s    LastEvictedPriority:%6d NumEvicts:%4d\n" \
		L"    NumSetToDevice:%4d NumSetToDevInVRAM:%4d NumCreatedInVRAM:%5d\n" \
		L"    TotalManagedBytes:%10d TotalManagedObjects:%5d TotalVdeoMemBytes:%10d TotalVdeoMemObjects:%5d\n",
		pStats->ApproxBytesDownloaded,
		szBool[ pStats->bThrashing ],
		pStats->LastPri,
		pStats->NumEvicts,
		pStatsDiff->NumUsed,
		pStatsDiff->NumUsedInVidMem,
		pStats->NumVidCreates,
		pStats->TotalBytes,
		pStats->TotalManaged,
		pStats->WorkingSetBytes,
		pStats->WorkingSet );
}

void e_GetStatsString( 
	WCHAR * pszString, 
	int nSize )
{
	if ( gtStatQueries.pVertexStats || gtStatQueries.pResourceManager )
	{
		WCHAR szCategoryData[ 4 ][ 1024 ];	ASSERT( gnStatsToDisplayCount == 4 );

		D3DRESOURCESTATS * pStats = NULL;
		D3DRESOURCESTATS * pStatsDiff = NULL;
		for ( int i = 0; i < gnStatsToDisplayCount; i++ )
		{
			pStats	   = &gtD3DStatResManagerRaw .stats[ gpnStatsToDisplay[ i ] ];
			pStatsDiff = &gtD3DStatResManagerDiff.stats[ gpnStatsToDisplay[ i ] ];

			sPrintStats( szCategoryData[ i ], 1024, pStats, pStatsDiff );
		}

		D3DRESOURCESTATS tAllStats;
		ZeroMemory( &tAllStats, sizeof(tAllStats) );

		// the stat types are indexed 1-7
		for ( int i = 1; i<=7; i++ )
		{
			tAllStats.ApproxBytesDownloaded += gtD3DStatResManagerRaw.stats[ i ].ApproxBytesDownloaded;
			tAllStats.bThrashing			|= gtD3DStatResManagerRaw.stats[ i ].bThrashing;
			tAllStats.LastPri				=  max( tAllStats.LastPri, gtD3DStatResManagerRaw.stats[ i ].LastPri );
			tAllStats.NumEvicts				+= gtD3DStatResManagerRaw.stats[ i ].NumEvicts;
			tAllStats.NumUsed				+= gtD3DStatResManagerDiff.stats[ i ].NumUsed;
			tAllStats.NumUsedInVidMem		+= gtD3DStatResManagerDiff.stats[ i ].NumUsedInVidMem;
			tAllStats.NumVidCreates			+= gtD3DStatResManagerRaw.stats[ i ].NumVidCreates;
			tAllStats.TotalBytes			+= gtD3DStatResManagerRaw.stats[ i ].TotalBytes;
			tAllStats.TotalManaged			+= gtD3DStatResManagerRaw.stats[ i ].TotalManaged;
			tAllStats.WorkingSet			+= gtD3DStatResManagerRaw.stats[ i ].WorkingSet;
			tAllStats.WorkingSetBytes		+= gtD3DStatResManagerRaw.stats[ i ].WorkingSetBytes;
		}
		pStats = &tAllStats;
		WCHAR szTotalData[ 1024 ];
		sPrintStats( szTotalData, 1024, &tAllStats, &tAllStats );

		WCHAR szTrisPerSec[ 32 ];
		PStrPrintf( szTrisPerSec, 32, L"%d", int( AppCommonGetDrawFrameRate() * gtD3DStatVertexStatsDiff.NumRenderedTriangles ) );
		PStrGroupThousands( szTrisPerSec, 32 );
		WCHAR szTris[ 32 ];
		PStrPrintf( szTris, 32, L"%d", gtD3DStatVertexStatsDiff.NumRenderedTriangles );
		PStrGroupThousands( szTris, 32 );
		PStrPrintf( pszString, nSize, 
			L"D3D Stats:\n" \
			L"VertStats:\n    NumRenderedTris: %10s EstTrisPerSecond: %11s\n" \
			L"Resource Manager - %S:\n%s" \
			L"Resource Manager - %S:\n%s" \
			L"Resource Manager - %S:\n%s" \
			L"Resource Manager - %S:\n%s" \
			L"Resource Manager - all:\n%s",
			szTris, 
			szTrisPerSec,
			gszStatsToDisplay[ 0 ], szCategoryData[ 0 ],
			gszStatsToDisplay[ 1 ], szCategoryData[ 1 ],
			gszStatsToDisplay[ 2 ], szCategoryData[ 2 ],
			gszStatsToDisplay[ 3 ], szCategoryData[ 3 ],
			szTotalData );
	} else
	{
		PStrPrintf( pszString, nSize, L"D3D Stats: data only available with D3D Debug runtime!\n" );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sGetHashMetricsString( const HashMetrics * pMetrics, const char * pszName, WCHAR * pszString, int nSize )
{
	float fPerBin = (float)pMetrics->nItems / pMetrics->nBins;
	PStrPrintf( pszString, nSize, L"%-12S%-8d%-8d%-8d%-8d%-8d%-8d%-8d%-8d%-6.1f\n",
		pszName,
		pMetrics->nBins,
		pMetrics->nItems,
		pMetrics->nGet,
		pMetrics->nGetFirst,
		pMetrics->nGetNext,
		pMetrics->nAdd,
		pMetrics->nMove,
		pMetrics->nRemove,
		fPerBin );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_GetHashMetricsString( 
	WCHAR * pszString, 
	int nSize )
{
	ASSERT_RETURN( pszString );

#if ISVERSION(DEVELOPMENT)


	// EXTERNS
	extern CHash<D3D_MODEL>						gtModels;
	extern CHash<MESH_DEFINITION_HASH>			gtMeshes;
	extern CHash<MODEL_DEFINITION_HASH>			gtModelDefinitions;
	extern CHash<D3D_TEXTURE>					gtTextures;
	extern CHash<D3D_LIGHT>						gtLights;
	extern CHash<PARTICLE_SYSTEM>				sgtParticleSystems;
	extern CHash<UI_D3DBUFFER>					gtUIEngineBuffers;
	extern CHash<UI_LOCALBUFFER>				gtUILocalBuffers;
	//extern CHash<SOUND_INSTANCE> 				sgtSounds;
	//extern CHash<SOUND_INSTANCE> 				sgtPlaySounds;
	//extern CHash<SOUND_INSTANCE> 				sgtVirtualSounds;
	//extern CHash<APPEARANCE>					sgpAppearances;
	//extern CHash<ATTACHMENT_HOLDER>				gtAttachmentHolders;

#define ADD_HASH_METRICS( hash, name )	{ hash.GetMetrics(), name },
#define ADD_HASH_METRICS_STUB( name )	{ 0, name },
	const int DISPLAY_NAME_LEN = 32;
	struct _HashMetricDisplay
	{
		HashMetrics tMetrics;
		char szName[DISPLAY_NAME_LEN];
	} tDisplay[] = 
	{
		ADD_HASH_METRICS( gtModels,					"Model" )
		ADD_HASH_METRICS( gtMeshes,					"Mesh" )
		ADD_HASH_METRICS( gtModelDefinitions,		"ModelDef" )
		ADD_HASH_METRICS( gtTextures, 				"Texture" )
		ADD_HASH_METRICS( gtLights, 				"Light" )
		ADD_HASH_METRICS( sgtParticleSystems,		"PartSys" )
		ADD_HASH_METRICS( gtUIEngineBuffers, 		"UIEngBuf" )
		ADD_HASH_METRICS( gtUILocalBuffers,			"UILocBuf" )
		//ADD_HASH_METRICS_STUB(					"Sound" )
		//ADD_HASH_METRICS_STUB(					"PlaySound" )
		//ADD_HASH_METRICS_STUB(					"VirtSound" )
		//ADD_HASH_METRICS_STUB(					"Appearance" )
		//ADD_HASH_METRICS_STUB(					"AttchHldr" )
	};
#undef ADD_HASH_METRICS
	int nDisplayCount = arrsize(tDisplay);

	// fill in stubs here?

	PStrPrintf( pszString, nSize, L"TABLE       BINS    ITEMS   GET     FIRST   NEXT    ADD     MOVE    REMOVE  ITEM/BIN\n\n" );

	const int DATA_LEN = 256;
	WCHAR szData[ DATA_LEN ];
	for ( int i = 0; i < nDisplayCount; i++ )
	{
		sGetHashMetricsString( &tDisplay[ i ].tMetrics, tDisplay[ i ].szName, szData, DATA_LEN );
		PStrCat( pszString, szData, nSize );
	}

#else

	pszString[ 0 ] = NULL;

#endif // ISVERSION(DEVELOPMENT)
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ProfileSetMarker( const WCHAR * pwszString, BOOL bForceNewFrame )
{
	if ( ! D3DPERF_GetStatus() )
		return;

	D3D_PROFILE_MARKER( pwszString );

	// forcing a new frame helps PIX recognize and capture calls made outside a normal frame context
	if ( bForceNewFrame )
	{
		LPDIRECT3DSWAPCHAIN9 pSwapChain = dxC_GetD3DSwapChain();
		ASSERT( pSwapChain );
		HRESULT hResult = pSwapChain->Present( NULL, NULL, NULL, NULL, 0 );
		// hResult may hold an error value like D3DERR_LOSTDEVICE; this is normal
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ToggleTrackResourceStats()
{
	gbTrackD3DResourceManagerStats = ! gbTrackD3DResourceManagerStats;
}
