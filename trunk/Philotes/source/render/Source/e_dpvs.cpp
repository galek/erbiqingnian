//----------------------------------------------------------------------------
// e_dpvs.cpp
//
// - Implementation for dPVS interface code
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "camera_priv.h"
#include "e_settings.h"
#include "e_main.h"
#include "performance.h"
#include "perfhier.h"
#include "e_primitive.h"
#include "e_model.h"
#include "e_renderlist.h"
#include "e_region.h"

#include "e_dpvs.h"
#include "e_dpvs_priv.h"

#ifdef ENABLE_DPVS
using namespace DPVS;
#endif // ENABLE_DPVS

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

#ifdef ENABLE_DPVS

DPVSCommander*		gpDPVSCommander			= NULL;
DPVSServices*		gpDPVSServices			= NULL;
//Cell*				gpDPVSCell				= NULL;
//Camera*			gpDPVSCamera			= NULL;
DPVSDebugCommander*	gpDPVSDebugCommander	= NULL;

#endif // ENABLE_DPVS

PFN_QUERY_BY_FILE gpfn_BGFileOccludesVisibility = NULL;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

void e_DPVS_Trace( const char * szStr, ... )
{
	va_list args;
	va_start(args, szStr);

	const int cnBufMax = 256;
	char buf[cnBufMax];
	PStrVprintf(buf, cnBufMax, szStr, args);
	int nLen = PStrLen(buf, cnBufMax);

	if (buf[nLen-1] != '\n')
		trace( "dpvs: %s\n", buf );
	else
		trace( "dpvs: %s", buf );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_Init()
{
#ifdef ENABLE_DPVS
	DPVSTRACE( "Init" );

	gpDPVSServices	= MALLOC_NEW( NULL, DPVSServices );
	ASSERT( gpDPVSServices );

	Library::init( Library::COLUMN_MAJOR, gpDPVSServices );

	gpDPVSCommander	= MALLOC_NEW( NULL, DPVSCommander );
	ASSERT( gpDPVSCommander );

	// debug
	// creating single global cell and single global camera
	//gpDPVSCell		= Cell::create();
	//gpDPVSCamera	= Camera::create();
	//ASSERT( gpDPVSCell );
	//ASSERT( gpDPVSCamera );
	//gpDPVSCamera->setCell( gpDPVSCell );

#if ISVERSION(DEBUG_VERSION)
	gpDPVSDebugCommander	= MALLOC_NEW( NULL, DPVSDebugCommander );
	ASSERT( gpDPVSDebugCommander );
#endif

	//Library::textCommand( gpDPVSCommander, "test" );
#endif // ENABLE_DPVS
	return S_OK;
}

PRESULT e_DPVS_Destroy()
{
#ifdef ENABLE_DPVS
	DPVSTRACE( "Destroy" );

	//DPVS_SAFE_RELEASE( gpDPVSCamera );
	//DPVS_SAFE_RELEASE( gpDPVSCell );

#if ISVERSION(DEVELOPMENT)
	//Library::textCommand( gpDPVSCommander, "wili" );			// outputs various stats, apparently
	//e_DPVS_QueryVisibility( NULL );
	V( e_DPVS_DebugDumpData() );
#endif

	FREE_DELETE( NULL, gpDPVSCommander, DPVSCommander );
#if ISVERSION(DEBUG_VERSION)
	FREE_DELETE( NULL, gpDPVSDebugCommander, DPVSDebugCommander );
#endif
	Library::exit();
	FREE_DELETE( NULL, gpDPVSServices, DPVSServices );			// must delete after Library::exit()
#endif // ENABLE_DPVS
	return S_OK;
}

PRESULT e_DPVS_DebugDumpData()
{
	// disabled
	return S_OK;

#ifdef ENABLE_DPVS
	//Library::textCommand( gpDPVSDebugCommander, "wili" );			// outputs various stats, apparently

	//Cell * pCell = Cell::create();
	//Camera * pDPVSCamera = Camera::create();
	//ASSERT_RETFAIL( pDPVSCamera );
	//pDPVSCamera->setCell( pCell );
	//pDPVSCamera->setParameters( 640, 480, 0 );
	//int nWidth  = pDPVSCamera->getWidth();
	//int nHeight = pDPVSCamera->getHeight();
	//// Set projection transform
	//Frustum frustum;
	//float fAspect = float(nWidth) / float(nHeight);
	//frustum.type = Frustum::PERSPECTIVE;
	//float fW, fH;
	//fH = tanf( PI/3.f * 0.5f ) * 0.1f;
	//fW = fH * fAspect;
	//frustum.left   = -fW;
	//frustum.right  = +fW;
	//frustum.top    = +fH;
	//frustum.bottom = -fH;
	//frustum.zNear  = 0.1f;
	//frustum.zFar   = 100.0f;
	//pDPVSCamera->setFrustum(frustum);


	//pDPVSCamera->resolveVisibility( gpDPVSDebugCommander, 1 );

	//pDPVSCamera->release();
	//pCell->release();

	Library::Statistic eStats[] =
	{
		Library::STAT_LIVEMODELS,
		Library::STAT_LIVEINSTANCES,
		Library::STAT_LIVECAMERAS,
		Library::STAT_LIVECELLS,
		Library::STAT_LIVEPHYSICALPORTALS,
		Library::STAT_LIVEVIRTUALPORTALS,
		Library::STAT_LIVEREGIONSOFINFLUENCE,
		Library::STAT_LIVENODES,
		Library::STAT_MEMORYCURRENTALLOCATIONS,
	};
	int nStats = arrsize(eStats);

	for ( int i = 0; i < nStats; ++i )
		trace( "dPVS: %32s  %d\n", Library::getStatisticName( eStats[i] ), (int)Library::getStatistic( eStats[i] ) );
#endif // ENABLE_DPVS

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void e_DPVS_ClearStats()
{
#ifdef ENABLE_DPVS
	if ( ! e_GetRenderFlag( RENDER_FLAG_DPVS_ENABLED ) )
		return;

	Library::resetStatistics();

	//const CAMERA_INFO * pCameraInfo = CameraGetInfo();
	//e_DPVS_CameraUpdate( pCameraInfo );

	//e_DPVS_QueryVisibility();
#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_CameraCreate(
	DPVS::Camera *& pDPVSCamera,
	BOOL bOcclusionCulling,
	int nRenderWidth,
	int nRenderHeight
	)
{
#ifdef ENABLE_DPVS
	ASSERT_RETINVALIDARG( pDPVSCamera == NULL );

	if ( ! e_GetRenderFlag( RENDER_FLAG_DPVS_ENABLED ) )
		return S_FALSE;

	if ( ! e_GetRenderFlag( RENDER_FLAG_DPVS_OCCLUSION ) )
		bOcclusionCulling = FALSE;

	BOOL bFrustumCulling = !! e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS );

	pDPVSCamera = Camera::create();
	ASSERT_RETFAIL( pDPVSCamera );
	//pDPVSCamera->autoRelease();

	unsigned int nFlags = 0;
	if ( bOcclusionCulling )		nFlags |= Camera::OCCLUSION_CULLING;
	if ( bFrustumCulling )			nFlags |= Camera::VIEWFRUSTUM_CULLING;

	int nDownsampleDPVSScreen = 1;
	float fScreenScale = 1.f / (nDownsampleDPVSScreen + 1);

	// Set rasterization/culling parameters
	pDPVSCamera->setParameters(
		nRenderWidth,
		nRenderHeight,
		nFlags,
		fScreenScale,
		fScreenScale );

	// Set Pixel Center
#ifdef ENGINE_TARGET_DX10
	// set pixel center to match D3D10/OpenGL default
	pDPVSCamera->setPixelCenter( 0.5f, 0.5f );
#else
	// set pixel center to match D3D9 default
	pDPVSCamera->setPixelCenter( 0.f, 0.f );
#endif

#endif // ENABLE_DPVS

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sCameraAdjustNearPortal(
	LP_INTERNAL_CELL & pUpdatedCell,
	int nCell,
	const CAMERA_INFO * pCameraInfo )
{
	ASSERT_RETINVALIDARG( pCameraInfo );

	// Figure out if the camera is near a portal
	const int MAX_PORTAL_IDS = 128;
	int nPortalIDs[ MAX_PORTAL_IDS ];
	int nPortals;
	V_RETURN( e_CellFindSourcePhysicalPortals( nCell, nPortalIDs, nPortals, MAX_PORTAL_IDS ) );

	for ( int i = 0; i < nPortals; ++i )
	{
		CULLING_PHYSICAL_PORTAL * pPortal = e_PhysicalPortalGet( nPortalIDs[i] );
		ASSERT_CONTINUE( pPortal );

		// Is this portal in the effective radius of the camera?
		if ( VectorDistanceSquared( pCameraInfo->vPosition, pPortal->vCenter ) > pPortal->fRadiusSq )
			continue;

		// Test against the portal plane
		float fDot = PlaneDotCoord( &pPortal->tPlane, &pCameraInfo->vPosition );
		if ( fDot > 0.f )
			continue;

		// Set to the portal's target cell
		CULLING_CELL * pTargetCell = e_CellGet( pPortal->nTargetCellID );
		ASSERT_CONTINUE( pTargetCell );
		ASSERT_CONTINUE( pTargetCell->pInternalCell );
		pUpdatedCell = pTargetCell->pInternalCell;
		break;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_CameraUpdate(
	DPVS::Camera * pDPVSCamera,
	int nCell,
	const CAMERA_INFO * pCameraInfo,
	float fNear,
	float fFar )
{
#ifdef ENABLE_DPVS
	if ( ! e_GetRenderFlag( RENDER_FLAG_DPVS_ENABLED ) )
		return S_FALSE;

	ASSERT_RETINVALIDARG( pCameraInfo );
	ASSERT_RETINVALIDARG( pDPVSCamera );
	ASSERT_RETINVALIDARG( nCell != INVALID_ID );

	CULLING_CELL * pCameraCell = e_CellGet( nCell );
	ASSERT_RETINVALIDARG( pCameraCell );

	LP_INTERNAL_CELL pUpdatedCell = pCameraCell->pInternalCell;
	V( sCameraAdjustNearPortal( pUpdatedCell, nCell, pCameraInfo ) );

	int nWidth  = pDPVSCamera->getWidth();
	int nHeight = pDPVSCamera->getHeight();

	// Set projection transform
	Frustum frustum;
	float fAspect = float(nWidth) / float(nHeight);

	switch ( pCameraInfo->eProjType )
	{
	case PROJ_ORTHO:
		frustum.type = Frustum::ORTHOGRAPHIC;
		break;
	case PROJ_PERSPECTIVE:
		frustum.type = Frustum::PERSPECTIVE;
		break;
	}

	float fW, fH;
	fH = tanf( pCameraInfo->fVerticalFOV * 0.5f ) * fNear;
	fW = fH * fAspect;

	frustum.left   = -fW;
	frustum.right  = +fW;
	frustum.top    = +fH;
	frustum.bottom = -fH;
	frustum.zNear  = fNear;
	frustum.zFar   = fFar;

	pDPVSCamera->setFrustum(frustum);

	VECTOR vRight;
	VECTOR vUp( 0, 0, 1 );
	VECTOR vForward = pCameraInfo->vLookAt - pCameraInfo->vPosition;
	VectorNormalize( vForward );
	VectorCross( vRight, vUp, vForward );
	VectorNormalize( vRight );
	VectorCross( vUp, vForward, vRight );
	VectorNormalize( vUp );

	MATRIX mCam;
	MatrixIdentity( &mCam );
	*(VECTOR*)&mCam._11 = vRight;
	*(VECTOR*)&mCam._21 = vUp;
	*(VECTOR*)&mCam._31 = vForward;
	*(VECTOR*)&mCam._41 = pCameraInfo->vPosition;



	// Set camera view transform
	//MATRIX mView;
	//e_GetWorldViewProjMatricies( NULL, &mView, NULL );


	// DEBUG
	//mCam = mView;

	// need to convert from LH Zup to LH Yup
	//MATRIX mCoordConvert =
	//{
	//	1.f, 0.f, 0.f, 0.f,
	//	0.f, 0.f, 1.f, 0.f,
	//	0.f, 1.f, 0.f, 0.f,
	//	0.f, 0.f, 0.f, 1.f, 
	//};
	//MatrixMultiply( &matView, &matView, &mCoordConvert );

	//MatrixInverse( &mView, &mView );

	//MATRIX mCell;
	//pCameraCell->pInternalCell->getCellToWorldMatrix( (Matrix4x4&)matWorld );
	//pCameraCell->pInternalCell->getWorldToCellMatrix( (Matrix4x4&)mCell );
	//MatrixMultiply( &mCam, &mCell, &mCam );
	//MatrixMultiply( &mCam, &mCam, &mCell );

	//MatrixInverse( &matView, &matView );

	// need to convert from LH Zup to LH Yup
	//SWAP( *(VECTOR*)&mCam._31, *(VECTOR*)&mCam._21 );


	// ensure that the fourth column is proper identity (avoid rounding error)
	mCam.m[0][3] = 0.f;
	mCam.m[1][3] = 0.f;
	mCam.m[2][3] = 0.f;
	mCam.m[3][3] = 1.f;

	pDPVSCamera->setCell( pUpdatedCell );
	pDPVSCamera->setCameraToCellMatrix( (const Matrix4x4&)mCam );

	//pDPVSCamera->setCameraToCellMatrix( (const Matrix4x4&)matView );

#endif // ENABLE_DPVS

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_DPVS_QueryVisibility( DPVS::Camera * pDPVSCamera )
{
#ifdef ENABLE_DPVS
	if ( ! e_GetRenderFlag( RENDER_FLAG_DPVS_ENABLED ) )
		return;

	PERF(CULL_DPVS);

	e_DPVS_ClearStats();

	//TIMER_STARTEX2( "DPVS Query", 3 );
	pDPVSCamera->resolveVisibility( gpDPVSCommander, MAX_PORTAL_DEPTH );
	//TIMER_END2();

#ifdef DPVS_DEBUG_OCCLUDER
	extern Vector3 *  gpDPVSDebugVerts;
	extern Vector3i * gpDPVSDebugTris;
	extern int gnDPVSDebugTris;

	if ( ! gpDPVSDebugTris || ! gpDPVSDebugVerts )
		return;

	for ( int t = 0; t < gnDPVSDebugTris; t++ )
	{
		e_PrimitiveAddTri( PRIM_FLAG_RENDER_ON_TOP, 
			(const VECTOR*)&gpDPVSDebugVerts[ gpDPVSDebugTris[t].i ],
			(const VECTOR*)&gpDPVSDebugVerts[ gpDPVSDebugTris[t].j ],
			(const VECTOR*)&gpDPVSDebugVerts[ gpDPVSDebugTris[t].k ],
			0x00ff0000 );
	}
#endif

#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD e_DPVS_DebugSetDrawFlags( DWORD dwFlags )
{
#ifdef ENABLE_DPVS
	DWORD dwCurFlags = Library::getFlags( Library::LINEDRAW );
	if ( dwCurFlags & dwFlags )
		Library::clearFlags( Library::LINEDRAW, dwFlags );
	else
		Library::setFlags( Library::LINEDRAW, dwFlags );
	return Library::getFlags( Library::LINEDRAW );
#else
	return 0;
#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void e_DPVS_GetStats( WCHAR * pwszStr, int nBufLen )
{
#ifdef ENABLE_DPVS
	const char * szVersion			= Library::getInfoString( Library::VERSION );
	const char * szBuildTime		= Library::getInfoString( Library::BUILD_TIME );
	const char * szOptimizations	= Library::getInfoString( Library::CPU_OPTIMIZATIONS );
	const char * szStatus			= Library::getInfoString( Library::FUNCTIONALITY );


	// 2nd stat is available to perform ratios
	const Library::Statistic cnInvalid = (Library::Statistic)-1;
	Library::Statistic pnStats[][2] =
	{
		{ Library::STAT_DATABASEOBSVISIBLE,			cnInvalid								},
		{ Library::STAT_WRITEQUEUEWRITESPERFORMED,	cnInvalid								},
		{ Library::STAT_DATABASEOBSTATUSCHANGES,	Library::STAT_DATABASEOBSVISIBLE		},
		{ Library::STAT_LIVEMODELS,					cnInvalid								},
		{ Library::STAT_LIVEINSTANCES,				cnInvalid								},
		{ Library::STAT_LIVECAMERAS,				cnInvalid								},
		{ Library::STAT_LIVECELLS,					cnInvalid								},
		{ Library::STAT_LIVEPHYSICALPORTALS,		cnInvalid								},
		{ Library::STAT_LIVEVIRTUALPORTALS,			cnInvalid								},
	};
	const int nStats = arrsize(pnStats);

	PStrPrintf( pwszStr, nBufLen, L"\n\ndPVS v%S [ %S ] %S  CPU Opts: %S\n", szVersion, szBuildTime, szStatus, szOptimizations );

	const int cnStatSize = 128;
	WCHAR wszStat[ cnStatSize ];
	for ( int i = 0; i < nStats; i++ )
	{
		if ( pnStats[ i ][ 1 ] == cnInvalid )
		{
			const char * szName		= Library::getStatisticName( pnStats[ i ][ 0 ] );
			float fValue			= Library::getStatistic( pnStats[ i ][ 0 ] );
			PStrPrintf( wszStat, cnStatSize, L"  %80S: %f\n", szName, fValue );
		} else
		{
			const char * szName1	= Library::getStatisticName( pnStats[ i ][ 0 ] );
			float fValue1			= Library::getStatistic( pnStats[ i ][ 0 ] );
			const char * szName2	= Library::getStatisticName( pnStats[ i ][ 1 ] );
			float fValue2			= Library::getStatistic( pnStats[ i ][ 1 ] );
			WCHAR wszRatio[ 81 ];
			PStrPrintf( wszRatio, 81, L"%S per %S", szName1, szName2 );
			PStrPrintf( wszStat, cnStatSize, L"  %80s: %f\n", wszRatio, fValue1 / fValue2 );
		}
		PStrCat( pwszStr, wszStat, nBufLen );
	}
#else
	pwszStr[0] = L'\0';
#endif // ENABLE_DPVS
}
