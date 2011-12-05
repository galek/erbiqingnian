//----------------------------------------------------------------------------
// c_particles.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"

//#if !ISVERSION(SERVER_VERSION)

#include "c_sound.h"
#include "c_sound_util.h"
#include "c_footsteps.h"
#include "units.h"
#include "c_particles.h"
#include "e_particles_priv.h"
#include "c_camera.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "c_appearance.h"
#include "room.h"
#include "level.h"
#include "filepaths.h"
#include "config.h"
#include "holodeck.h"
#include "e_common.h"
#include "e_definition.h"
#include "e_particle.h"
#include "e_texture.h"
#include "e_light.h"
#include "e_model.h"
#include "e_profile.h"
#include "e_texture.h"
#include "e_environment.h"
#include "e_settings.h"
#include "pakfile.h"
#include "appcommon.h"
#include "e_havokfx.h"
#include "e_frustum.h"
#include "e_main.h"
#include "e_effect_priv.h"
#include "e_region.h"
#include "unit_priv.h"
#ifdef INTEL_CHANGES
#include "jobs.h"
#include "memory.h"
#ifdef THREAD_PROFILER
#include "libittnotify.h"

// events to trigger a visual flag in Thread Profiler
static __itt_event sgtThreadProfilerEventParticles;
static __itt_event sgtThreadProfilerEventAsyncParticles;
#endif

#include "e_optionstate.h"	// CHB 2007.09.12

#if USE_MEMORY_MANAGER
#include "memorymanager_i.h"
#include "memoryallocator_wrapper.h"
#include "memoryallocator_heap.h"
using namespace FSCommon;
#endif

int UnitsGetNearbyUnitsForParticles(
	int nMaxUnits, 
	const VECTOR & vCenter, 
	float fRadius, 
	VECTOR * pvPositions, 
	VECTOR * pvHeightsRadiiSpeeds, 
	VECTOR * pvDirections );

// all the changes necessary to support async particles
BOOL * sgbParticleSystemAsyncVerticesBufferAvailable = NULL;
int sgnParticleSystemAsyncCount = 0;
int sgnParticleSystemAsyncMax = 0;

PARTICLE_QUAD_VERTEX * sgpParticleSystemAsyncVerticesBuffer[ PARTICLE_SYSTEM_ASYNC_MAX ];
PARTICLE_QUAD_VERTEX * sgpParticleSystemAsyncMainThreadVerticesBuffer[ PARTICLE_SYSTEM_ASYNC_MAX ];

struct ASYNC_PARTICLE_SYSTEM
{
	int				nId;  // need this and the next one for CHash support
	ASYNC_PARTICLE_SYSTEM *pNext;
	PARTICLE_SYSTEM tParticleSystem;
	HANDLE			hCallbackFinished;
	
#if USE_MEMORY_MANAGER

#if DEBUG_MEMORY_ALLOCATIONS
#define useDebugHeaderAndFooter true
#else
#define useDebugHeaderAndFooter false
#endif	

	CMemoryAllocatorHEAP<true, useDebugHeaderAndFooter, 80>*	pMemoryPoolHEAP;
	CMemoryAllocatorWRAPPER* pMemoryPool;
#else
	MEMORYPOOL *	pMemoryPool;
#endif	
	
	
	int				nAsyncVerticesBufferIndex; // which vertices buffer is used?

	// this is put together in good twin during update and draw, then copied here before job dispatch
	// these are inputs to the callback from update
	float			fAsyncUpdateTime;
	// these are inputs to the callback from draw
	VECTOR			vAsyncEyeDirection;
	PLANE			pAsyncPlanes[ NUM_FRUSTUM_PLANES ];

	struct PARTICLE_TO_DRAW * pParticleSortList;
	int				nParticlesToSortAllocated;
	int				nParticleCount;

	// NOTE: these will be changed in the main thread.  They cannot be used in the async thread
	VECTOR			vMainThreadEyeDirection;
	PLANE			pMainThreadAsyncPlanes[ NUM_FRUSTUM_PLANES ];
	BOOL			bMainThreadEnabled;

	// these are outputs from the callback
	int				nVerticesInBuffer;
	int				nIndexBufferCurr;
};
static CHash<ASYNC_PARTICLE_SYSTEM> sgtAsyncParticleSystems;
static BOOL sgbAsyncParticleSystemsValid;

// prototype the callbacks
static void sAsyncParticlesJob( JOB_DATA & );
static void sAsyncParticlesComplete( JOB_DATA & );
static
void sCreateAsyncParticleSystemJob(
	PARTICLE_SYSTEM * pParticleSystem,
	ASYNC_PARTICLE_SYSTEM * pAsyncParticleSystem );

#endif

#define PARTIClE_CULL_FOR_SPEED_ENABLED
#define PARTICLE_FILL_TEXTURE_ALPHA_FACTOR 0.6f

static float sgfMaxPredictedParticleScreens = 8.0f;

static int   sgnParticlesUpdateSync     = 0;
static int   sgnParticlesUpdateAsync[ NUM_PARTICLE_DRAW_SECTIONS ];
static int   sgnParticleSystemGUID       = 0;
static VECTOR sgvWindDirection ( 0, 1, 0 );
static float sgfWindMin = 1.0f;
static float sgfWindMax = 5.0f;
static float sgfWindCurrent = 3.0f;
static float sgfWindTimeCurrent = 0.0f;
static float sgfWindFrequency = 2.0f;

extern int gnLastBeginOcclusionModelID;
extern int gnLastEndOcclusionModelID;

static struct PARTICLE_TO_DRAW * sgpParticleSortList = NULL;
static int sgnParticleSortListAllocated = 0;

#define PARTICLE_PERF(x)
#define PARTICLE_HITCOUNT(x)

CHash<PARTICLE_SYSTEM> sgtParticleSystems;

static PARTICLE_SYSTEM ** sgpDrawList = NULL;
static int sgnDrawListCount = 0;
static int sgnDrawListAllocated = 0;
static RAND sgParticlesRand;
static DWORD sgdwGlobalFlags = 0;
static BOOL sgbUpdateParticleCounts = FALSE;
static int sgnCullPriority = MAX_PARTICLE_SYSTEM_CULL_PRIORITY;

static VECTOR sgvGravity = VECTOR( 0.0f, 0.0f, -9.8f );
static VECTOR sgvUpVector = VECTOR( 0.0f, 0.0f, 1.0f );

#define PARTICLE_SYSTEM_FADE_TIME 4.0f
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define ROPE_PATH_FILE_NAME_LENGTH 40
struct ROPE_PATH_FILE
{
	float fTotalLength;
	int nPointCount;
	SAFE_PTR(VECTOR*, pvPoints);
	SAFE_PTR(float*, pfLengths);
	SAFE_PTR(BYTE*, pbFileStart);
};

struct ROPE_PATH_HASH
{
	int nId;
	char pszFilename[ ROPE_PATH_FILE_NAME_LENGTH ];
	ROPE_PATH_HASH * pNext;
	ROPE_PATH_FILE * pFile;
};

static CHash< ROPE_PATH_HASH > sgtRopePathHash;
static int sgnRopePathHashIndex = 0;

#define ROPE_PATH_FILE_EXTENSION			"rp"
#define ROPE_PATH_FILE_VERSION				(1 + GLOBAL_FILE_VERSION)
#define ROPE_PATH_FILE_MAGIC_NUMBER			0xabab1974
#define VERTEX_COUNT_TOKEN					"SHAPE_VERTEXCOUNT"

static KNOT * c_sRopeAddKnot( 
	GAME * pGame, 
	PARTICLE_SYSTEM * pParticleSystem,
	float fSegmentLength,
	float fPathTime,
	BOOL bAddEnd );

static float c_sGetValueFromPath( 
	float fPathTime, 
	const CInterpolationPath<CFloatPair> & tPath);


#define PARTICLE_COUNT_MAX					3000.0f	//max particles allowed
#define PARTICLE_COUNT_ACTIVATE_AT_FPS		20.0f	//when fps goes below this, the delta is modified by particle count
#define PARTICLE_TIMEDELTA_ACCELERATE_MAX	2.0f	//speed up particles by delta
#define PARTICLE_FPS_MAX					30.0f	//start effecting particle spawn rate below this fps
#define PARTICLE_FPS_MIN					10.0f	//reach MAX particle drop at this FPS
#define PARTICLE_FRAME_COUNT				20		//stores X number of FPS and averages them for a smoother delta line
static float sParticleReleaseDelta( 1.0f );
static BOOL bForceFPS(FALSE);
static float fForceFPS(60.0f);


static inline void sUpdateParticlesAllowedToRelease()
{
	
	if (AppIsHammer() ||
		AppIsHellgate() )
	{
		return;
	}
	
	//might in the future add this to the UI.
	static float sParticleDropOffPresets[] = { .1f, .2f, .3f, .4f, .5f, .6f, .7f, .8f, .9f, 1.0f };
	static int nParticlePreset( 0 );
	static int sIndexOfFrameCount( 0 );
	static int sTotalFrameCount(0);
	static float sLastFrames[ PARTICLE_FRAME_COUNT ];

	float fFPS = AppCommonGetDrawFrameRate();	
	
	if( fFPS == 0 )
		return;		
	sLastFrames[ sIndexOfFrameCount ] = fFPS;
	sTotalFrameCount += ( sTotalFrameCount < PARTICLE_FRAME_COUNT )?1:0;
	sIndexOfFrameCount += ( sIndexOfFrameCount >=PARTICLE_FRAME_COUNT )?-sIndexOfFrameCount:1;
	float fAverageFPS = 0;
	for( int t = 0; t < sTotalFrameCount; t++ )
	{
		fAverageFPS += sLastFrames[ t ];
	}
	fAverageFPS /= (float)sTotalFrameCount;	
	if( bForceFPS )
	{
		fAverageFPS = fForceFPS;
	}
	static float sParticleDif = PARTICLE_FPS_MAX - PARTICLE_FPS_MIN;
	static float sTurnOnParticleDropCount = PARTICLE_COUNT_ACTIVATE_AT_FPS - PARTICLE_FPS_MIN;
	fAverageFPS -= PARTICLE_FPS_MIN;
	fAverageFPS = MAX( fAverageFPS, 0.0f );
	fAverageFPS = MIN( fAverageFPS, sParticleDif );
	sParticleReleaseDelta = fAverageFPS/sParticleDif;	
	if( fAverageFPS < sTurnOnParticleDropCount)
	{
		
		float fParticles = (float)e_ParticlesGetDrawnTotal();
		fParticles = MAX( fParticles, (float)c_ParticlesGetUpdateTotal() );
		fParticles = 1.0f - ( fParticles/PARTICLE_COUNT_MAX );
		sParticleReleaseDelta *= fParticles;
		
	}

	sParticleReleaseDelta = ( sParticleReleaseDelta < sParticleDropOffPresets[ nParticlePreset ] )?sParticleDropOffPresets[ nParticlePreset ]:sParticleReleaseDelta;
}



//returns the delta to modify the number of partiles to release
static inline float sParticlesCanReleaseDelta( PARTICLE_SYSTEM * pParticleSystem )
{
	
	if (AppIsHammer() ||
		AppIsHellgate() )
	{
		return 1;
	}
	else
	{	
		if( pParticleSystem &&
			pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_PARTICLE_IS_LIGHT )
			return 1.0f;
		return MAX( pParticleSystem->fMinParticlesAllowedToDrop, sParticleReleaseDelta );	
	}	
}

#if ISVERSION(DEVELOPMENT)
float c_ParticleGetDelta()
{
	return sParticleReleaseDelta;
}
#endif


void c_ParticleSetFPS( float fps )
{
	if( fps < 0 )
	{	
		bForceFPS = FALSE;
		return;
	}
	bForceFPS = TRUE;
	fForceFPS = fps;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sParticleSystemUsesHavokFX ( 
	PARTICLE_SYSTEM * pParticleSystem )
{
	if ( ! AppIsHellgate() )
		return FALSE;
	if ( ! pParticleSystem->pDefinition )
		return FALSE;
	if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_CAN_USE_HAVOKFX ) == 0 )
		return FALSE;
	if ( ! e_HavokFXIsEnabled() )
		return FALSE;
	return (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_USING_HAVOK_FX) != 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static float sReadPathRopeValueAndAdvancePointer( char ** ppszFileCurr )
{
	float fValue = PStrToFloat( *ppszFileCurr );
	*ppszFileCurr += ( **ppszFileCurr == '-' ) ? 8 : 7;
	float fAbs = fabsf( fValue );
	while ( fAbs > 10.0f )
	{
		*ppszFileCurr += 1;
		fAbs /= 10.0f;
	}
	return fValue;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sRopePathUpdateFile(
	const char * pszFilename )
{
	float * pfLengths = NULL;
	VECTOR * pvPoints = NULL;

	char pszSourceFileName[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrReplaceExtension(pszSourceFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFilename, "ase");

	char pszBinaryFileName[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrReplaceExtension(pszBinaryFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFilename, ROPE_PATH_FILE_EXTENSION);

	// check and see if we need to update at all
	if ( ! AppCommonAllowAssetUpdate() || ! FileNeedsUpdate(pszBinaryFileName, pszSourceFileName, ROPE_PATH_FILE_MAGIC_NUMBER, ROPE_PATH_FILE_VERSION) )
		return FALSE;

	// -- Create Model File --
	HANDLE hBinFile = CreateFile( pszBinaryFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	ASSERTV_RETFALSE( hBinFile != INVALID_HANDLE_VALUE, "Couldn't open rope file for writing!  Maybe it was checked in when it shouldn't have been?\n\n%s", pszBinaryFileName );

	BOOL bReturnValue = FALSE;

	DECLARE_LOAD_SPEC( tSourceSpec, pszSourceFileName );

	void * pFileStart = PakFileLoadNow(tSourceSpec);

	if ( ! pFileStart )
	{
		goto cleanup;
	}

	FILE_HEADER tHeader;
	tHeader.dwMagicNumber = 0;
	tHeader.nVersion = ROPE_PATH_FILE_VERSION;

	DWORD dwBytesWritten = 0;
	int nFileOffsetCurr = 0;
	BOOL bWritten = WriteFile( hBinFile, &tHeader, sizeof( FILE_HEADER ), &dwBytesWritten, NULL );
	REF(bWritten);
	nFileOffsetCurr += dwBytesWritten;

	// find the knot count
	char * pszFileString = (char *) pFileStart;
	const char * pszVertCount = PStrStr( pszFileString, VERTEX_COUNT_TOKEN );
	if ( ! pszVertCount )
		goto cleanup;

	char * pszFileCurr = const_cast<char*>(pszVertCount + PStrLen( VERTEX_COUNT_TOKEN ) + 1);
	char * pszFileEnd = (char *)pszFileString + tSourceSpec.bytesread;

	ROPE_PATH_FILE tPathFile;
	ZeroMemory( &tPathFile, sizeof( ROPE_PATH_FILE ) );
	tPathFile.nPointCount = PStrToInt( pszFileCurr );
	if ( ! tPathFile.nPointCount )
		goto cleanup;

	while ( *pszFileCurr != '\n' && pszFileCurr < pszFileEnd )
		pszFileCurr++;

	pszFileCurr++; // get past the return

	if ( pszFileCurr >= pszFileEnd )
		goto cleanup;

	pvPoints = (VECTOR *) MALLOC( NULL, sizeof( VECTOR ) * tPathFile.nPointCount );
	for ( int i = 0; i < tPathFile.nPointCount; i++ )
	{// 		*SHAPE_VERTEX_KNOT	0	58.5213	-16.0350	0.0000
		pszFileCurr += 2; // get past the opening tabs

		while ( *pszFileCurr != '\t' && pszFileCurr < pszFileEnd )
			pszFileCurr++; // to the next tab

		pszFileCurr++; // past the tab

		while ( *pszFileCurr != '\t' && pszFileCurr < pszFileEnd )
			pszFileCurr++; // to the next tab

		pszFileCurr++; // past the tab

		ASSERT_BREAK ( pszFileCurr < pszFileEnd );

		VECTOR * pvPointCurr = & pvPoints[ i ];

		pvPointCurr->fX = sReadPathRopeValueAndAdvancePointer( &pszFileCurr );
		pvPointCurr->fY = sReadPathRopeValueAndAdvancePointer( &pszFileCurr );
		pvPointCurr->fZ = sReadPathRopeValueAndAdvancePointer( &pszFileCurr );

		pszFileCurr++; // past the return
	}

	// calculate the segment lengths
	pfLengths = (float *) MALLOC( NULL, sizeof( float ) * tPathFile.nPointCount );
	for ( int i = 0; i < tPathFile.nPointCount - 1; i++ )
	{
		pfLengths[ i ] = VectorLength( pvPoints[ i ] - pvPoints[ i + 1 ] );
		tPathFile.fTotalLength += pfLengths[ i ];
	}
	pfLengths[ tPathFile.nPointCount - 1 ] = 0.0f;

	tPathFile.pvPoints = (VECTOR *) (sizeof( ROPE_PATH_FILE ) + nFileOffsetCurr);
	tPathFile.pfLengths = (float *) (CAST_PTR_TO_INT(tPathFile.pvPoints) + (sizeof( VECTOR ) * tPathFile.nPointCount));
	WriteFile( hBinFile, &tPathFile, sizeof( ROPE_PATH_FILE ), &dwBytesWritten, NULL );
	nFileOffsetCurr += dwBytesWritten;

	ASSERT(CAST_PTR_TO_INT(tPathFile.pvPoints) == nFileOffsetCurr);
	WriteFile( hBinFile, pvPoints, sizeof( VECTOR ) * tPathFile.nPointCount, &dwBytesWritten, NULL );
	nFileOffsetCurr += dwBytesWritten;

	ASSERT(CAST_PTR_TO_INT(tPathFile.pfLengths) == nFileOffsetCurr );
	WriteFile( hBinFile, pfLengths, sizeof( float ) * tPathFile.nPointCount, &dwBytesWritten, NULL );
	nFileOffsetCurr += dwBytesWritten;

	// go to the beginning again and set the magic number
	SetFilePointer( hBinFile, 0, 0, FILE_BEGIN );
	tHeader.dwMagicNumber = ROPE_PATH_FILE_MAGIC_NUMBER; 
	WriteFile( hBinFile, &tHeader, sizeof( FILE_HEADER ), &dwBytesWritten, NULL );
	ASSERT( dwBytesWritten == sizeof( FILE_HEADER) );
	bReturnValue = TRUE;

cleanup:
	CloseHandle( hBinFile );

	FREE( NULL, pFileStart );
	if ( pvPoints )
		FREE( NULL, pvPoints );
	if ( pfLengths )
		FREE( NULL, pfLengths );

	return bReturnValue;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sProcessRopePathFile(
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

	if (data.m_IOSize == 0) 
	{
		return S_FALSE;
	}

	ASSERTX_RETFAIL( spec->buffer, spec->fixedfilename );

	CLEAR_MASK(spec->flags, PAKFILE_LOAD_FREE_BUFFER);

	ROPE_PATH_HASH * pHash = HashGet( sgtRopePathHash, CAST_PTR_TO_INT(spec->callbackData));

	FILE_HEADER * pHeader = (FILE_HEADER *)spec->buffer;
	ASSERT_RETFAIL( pHeader->nVersion == ROPE_PATH_FILE_VERSION );
	ASSERT_RETFAIL( pHeader->dwMagicNumber == ROPE_PATH_FILE_MAGIC_NUMBER );

	pHash->pFile = (ROPE_PATH_FILE *) ((BYTE *)spec->buffer + sizeof(FILE_HEADER));
	pHash->pFile->pbFileStart = (BYTE *) spec->buffer;
	if ( pHash->pFile->nPointCount )
	{
		pHash->pFile->pvPoints = (VECTOR *)(pHash->pFile->pbFileStart + CAST_PTR_TO_INT(pHash->pFile->pvPoints));
		pHash->pFile->pfLengths = (float *)(pHash->pFile->pbFileStart + CAST_PTR_TO_INT(pHash->pFile->pfLengths));
	}
	else
	{
		pHash->pFile->pvPoints = NULL;
		pHash->pFile->pfLengths = NULL;
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sRopePathGet(
	const char * pszFilename )
{
	{
		for ( ROPE_PATH_HASH * pHash = HashGetFirst( sgtRopePathHash );
			pHash;
			pHash = HashGetNext( sgtRopePathHash, pHash ) )
		{
			if ( PStrCmp( pHash->pszFilename, pszFilename ) == 0 )
				return pHash->nId;
		}
	}

	ROPE_PATH_HASH * pHash = HashAdd( sgtRopePathHash, sgnRopePathHashIndex++ );
	pHash->pFile = NULL;
	PStrCopy( pHash->pszFilename, pszFilename, ROPE_PATH_FILE_NAME_LENGTH );
	
	char pszTemp[ MAX_PATH ];
	PStrPrintf( pszTemp, MAX_PATH, "%s%s", FILE_PATH_PARTICLE_ROPE_PATHS, pszFilename );
	
	char pszFilenameWithPath[ MAX_PATH ];
	FileGetFullFileName( pszFilenameWithPath, pszTemp, MAX_PATH );

	sRopePathUpdateFile( pszFilenameWithPath );

	char pszBinFileWithPath[ MAX_PATH ];
	PStrReplaceExtension( pszBinFileWithPath, MAX_PATH, pszFilenameWithPath, ROPE_PATH_FILE_EXTENSION );
	DECLARE_LOAD_SPEC(spec, pszBinFileWithPath);
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER;
	spec.fpLoadingThreadCallback = sProcessRopePathFile;
	spec.callbackData = CAST_TO_VOIDPTR(pHash->nId);
	spec.priority = ASYNC_PRIORITY_PARTICLE_DEFINITIONS;

	PakFileLoad(spec);

	return pHash->nId;
}

// ************************************************************************************************
// ************************************************************************************************
static void c_sParticleSystemSetFlag( 
	int nParticleSystemId,
	DWORD dwFlag,
	BOOL bSet,
	BOOL bIncludeEnds );

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL c_sParticleSystemHasRope ( 
	PARTICLE_SYSTEM * pSystem )
{
	if ( ! pSystem->pDefinition )
		return FALSE;
	return (pSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_HAS_ROPE) != 0 &&
		(pSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ASYNC) == 0;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL c_ParticleSystemHasRope( 
	int nParticleSystemId)
{
	PARTICLE_SYSTEM* pSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pSystem)
	{
		return FALSE;
	}
	return c_sParticleSystemHasRope(pSystem);
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PARTICLE_SYSTEM * c_ParticleSystemGet(
	int nId )
{
	return HashGet( sgtParticleSystems, nId );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sParticleSystemAddRef( PARTICLE_SYSTEM_DEFINITION * pDefinition )
{
	if ( ! pDefinition )
		return;
	if ( c_GetToolMode() )
		return;
	pDefinition->tRefCount.AddRef();
}

static void sParticleSystemReleaseRef( PARTICLE_SYSTEM_DEFINITION * pDefinition )
{
	if ( ! pDefinition )
		return;
	if ( c_GetToolMode() )
		return;
	pDefinition->tRefCount.Release();
}

void c_ParticleSystemAddRef( int nParticleSystemDefId )
{
	if ( nParticleSystemDefId == INVALID_ID )
		return;
	PARTICLE_SYSTEM_DEFINITION * pDefinition = (PARTICLE_SYSTEM_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_PARTICLE, nParticleSystemDefId );
	sParticleSystemAddRef( pDefinition );
}

void c_ParticleSystemReleaseRef( int nParticleSystemDefId )
{
	if ( nParticleSystemDefId == INVALID_ID )
		return;
	PARTICLE_SYSTEM_DEFINITION * pDefinition = (PARTICLE_SYSTEM_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_PARTICLE, nParticleSystemDefId );
	sParticleSystemReleaseRef( pDefinition );
}

//-------------------------------------------------------------------------------------------------
// Brad, this is where you decide whether we can do async systems - do we have cores?  - Tyler
//-------------------------------------------------------------------------------------------------
static BOOL c_sCanDoAsyncSystems()
{
	BOOL bRet;
#ifdef INTEL_CHANGES
	bRet = ( gtCPUInfo.m_AvailLogical > 1 );
#else
	bRet = FALSE;
#endif

	return bRet;
}

#ifdef INTEL_CHANGES
static BOOL c_sAllowedAsyncSystems()
{
	// gtCPUInfo has been populated during CommonInit()
	return( gtCPUInfo.m_AvailLogical - 1 );
}

#endif
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleSystemsInit()
{
	RandInit(sgParticlesRand);
	HashInit(sgtParticleSystems, NULL, 256);
	HashInit(sgtRopePathHash, NULL, 16);

#if !ISVERSION(SERVER_VERSION)

	if ( e_DX10IsEnabled() )
		c_ParticleSetGlobalBit( PARTICLE_GLOBAL_BIT_DX10_ENABLED, TRUE );
	
#ifdef INTEL_CHANGES
	if ( c_sCanDoAsyncSystems() )
	{
		c_ParticleSetGlobalBit( PARTICLE_GLOBAL_BIT_ASYNC_SYSTEMS_ENABLED, TRUE );
		
		// INTEL_NOTE: this initialization is dependent upon the bit, so enabling it later from the console is "bad"
		HashInit(sgtAsyncParticleSystems, NULL, PARTICLE_SYSTEM_ASYNC_MAX);
		sgbAsyncParticleSystemsValid = TRUE;

		sgbParticleSystemAsyncVerticesBufferAvailable = (BOOL *) MALLOC( NULL, PARTICLE_SYSTEM_ASYNC_MAX * sizeof( BOOL ) );
		
		for( int i = 0; i < PARTICLE_SYSTEM_ASYNC_MAX; i++ )
		{
			sgbParticleSystemAsyncVerticesBufferAvailable[i] = TRUE;
			sgpParticleSystemAsyncVerticesBuffer[ i ] = NULL;
			sgpParticleSystemAsyncMainThreadVerticesBuffer[ i ] = NULL;
		}

		sgnParticleSystemAsyncCount = 0;
		sgnParticleSystemAsyncMax = c_sAllowedAsyncSystems();

#ifdef THREAD_PROFILER
		sgtThreadProfilerEventParticles = __itt_event_createA("c_ParticlesDraw", 15);
		sgtThreadProfilerEventAsyncParticles = __itt_event_createA("sAsyncParticlesJob", 18);
#endif
	}
#else
	if ( c_sCanDoAsyncSystems() )
		c_ParticleSetGlobalBit( PARTICLE_GLOBAL_BIT_ASYNC_SYSTEMS_ENABLED, TRUE );
#endif //#ifndef INTEL_CHANGES
		
	e_ParticleSystemsInitCallbacks( c_ParticleSystemGet, UnitsGetNearbyUnitsForParticles );

	V( e_ParticlesDrawInit() );
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemsDestroy()
{
	V( e_ParticleSystemsReleaseGlobalResources() );

	for (PARTICLE_SYSTEM* pSystem = HashGetFirst(sgtParticleSystems); pSystem; pSystem = HashGetNext(sgtParticleSystems, pSystem))
	{
		pSystem->tParticles.Destroy();
		pSystem->tKnots.Destroy();
		pSystem->tWaveOffsets.Destroy();
		if ( pSystem->pDefinition )
			sParticleSystemReleaseRef( pSystem->pDefinition );
	}
	HashFree(sgtParticleSystems);

	for (ROPE_PATH_HASH* pHash = HashGetFirst(sgtRopePathHash); 
		pHash; 
		pHash = HashGetNext(sgtRopePathHash, pHash))
	{
		if ( pHash->pFile )
			FREE( NULL, pHash->pFile->pbFileStart );
	}
	HashFree(sgtRopePathHash);

	V( e_ParticleSystemsReleaseDefinitionResources() );

	if ( sgpDrawList )
		FREE( NULL, sgpDrawList );

	if ( sgpParticleSortList )
		FREE( NULL, sgpParticleSortList );

	sgpDrawList = NULL;
	sgnDrawListCount = 0;

#ifdef INTEL_CHANGES
#if !ISVERSION(SERVER_VERSION)
	if ( c_ParticleTestGlobalBit( PARTICLE_GLOBAL_BIT_ASYNC_SYSTEMS_ENABLED ) )
	{
		// need to wait here on end of async particle jobs
		ASYNC_PARTICLE_SYSTEM *pAsyncParticleSystem = NULL;
		while ( (pAsyncParticleSystem = HashGetFirst(sgtAsyncParticleSystems) ) != NULL )
		{
			pAsyncParticleSystem->bMainThreadEnabled = FALSE;
			WaitForSingleObject( pAsyncParticleSystem->hCallbackFinished, INFINITE );

			// clean up async system with code taken from sAsyncParticlesComplete
			// destroy the event
			CloseHandle( pAsyncParticleSystem->hCallbackFinished );
			
			// make the particle system disappear
			MEMORYPOOL *pMemoryPool = pAsyncParticleSystem->pMemoryPool;

			// this will remove pAsyncParticleSystem from the hash
			c_ParticleSystemDestroy( pAsyncParticleSystem->nId, FALSE, TRUE );

#if USE_MEMORY_MANAGER
			pAsyncParticleSystem->pMemoryPoolHEAP->Term();
			IMemoryManager::GetInstance().RemoveAllocator(pAsyncParticleSystem->pMemoryPoolHEAP);
			delete pAsyncParticleSystem->pMemoryPoolHEAP;
			pAsyncParticleSystem->pMemoryPoolHEAP = NULL;
			
			pMemoryPool->Term();			
			delete pMemoryPool;			
#else			
			MemoryPoolFree( pMemoryPool);
#endif
		}
		HashFree(sgtAsyncParticleSystems);
		sgnParticleSystemAsyncMax = 0;
		sgbAsyncParticleSystemsValid = FALSE;

		for ( int i = 0; i < PARTICLE_SYSTEM_ASYNC_MAX; i++ )
		{
			if ( sgpParticleSystemAsyncVerticesBuffer[ i ] )
			{
#if USE_MEMORY_MANAGER
				FREE( g_StaticAllocator, sgpParticleSystemAsyncVerticesBuffer[ i ] );
#else			
				FREE( NULL, sgpParticleSystemAsyncVerticesBuffer[ i ] );
#endif
				sgpParticleSystemAsyncVerticesBuffer[ i ] = NULL;
			}
			if ( sgpParticleSystemAsyncMainThreadVerticesBuffer[ i ] )
			{
#if USE_MEMORY_MANAGER
				FREE( g_StaticAllocator, sgpParticleSystemAsyncMainThreadVerticesBuffer[ i ] );
#else			
				FREE( NULL, sgpParticleSystemAsyncMainThreadVerticesBuffer[ i ] );
#endif
				sgpParticleSystemAsyncMainThreadVerticesBuffer[ i ] = NULL;
			}

		}
		FREE( NULL, sgbParticleSystemAsyncVerticesBufferAvailable );
	}
#endif
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int sCompareParticleSystems( PARTICLE_SYSTEM * pSystem, int nOrder )
{
	if ( pSystem->pDefinition->nDrawOrder == nOrder )
		return 0;
	if ( pSystem->pDefinition->nDrawOrder > nOrder )
		return -1;
	return 1;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sParticleSystemTextureDefinitionLoaded( 
	void * pTextureDef,
	EVENT_DATA * pCallbackData )
{
	int nParticleSystemDefId = (int)pCallbackData->m_Data1;

	TEXTURE_DEFINITION * pTextureDefinition = (TEXTURE_DEFINITION *) pTextureDef;

	PARTICLE_SYSTEM_DEFINITION * pParticleSystemDef = (PARTICLE_SYSTEM_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_PARTICLE, nParticleSystemDefId );
	ASSERT_RETURN( pParticleSystemDef );
	
	if ( pTextureDefinition->nFramesU > 0 || pTextureDefinition->nFramesV > 0 )
	{
		pParticleSystemDef->pfTextureFrameSize[ 0 ] = 1.0f / (float) pTextureDefinition->nFramesU;
		pParticleSystemDef->pfTextureFrameSize[ 1 ] = 1.0f / (float) pTextureDefinition->nFramesV;
		pParticleSystemDef->dwFlags |= PARTICLE_SYSTEM_DEFINITION_FLAG_TEXTURE_ANIMATES;
	} else {
		pParticleSystemDef->pfTextureFrameSize[ 0 ] = 1.0f;
		pParticleSystemDef->pfTextureFrameSize[ 1 ] = 1.0f;
	}
	pParticleSystemDef->pnTextureFrameCount[ 0 ] = pTextureDefinition->nFramesU;
	pParticleSystemDef->pnTextureFrameCount[ 1 ] = pTextureDefinition->nFramesV;

}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void c_sParticleSystemLoadTextures( 
	PARTICLE_SYSTEM_DEFINITION * pDefinition )
{
	if ( pDefinition->nTextureId == INVALID_ID && pDefinition->pszTextureName[ 0 ] != 0 )
	{
		const char * pszFileName = pDefinition->pszTextureName;
		char szFile[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%stextures\\%s", FILE_PATH_PARTICLE, pszFileName );
		V( e_TextureNewFromFile( &pDefinition->nTextureId, szFile, TEXTURE_GROUP_PARTICLE, TEXTURE_DIFFUSE ) );

		e_TextureAddRef( pDefinition->nTextureId );

		if ( e_TextureIsValidID( pDefinition->nTextureId ) )
		{
			int nTextureDefinition  = e_TextureGetDefinition( pDefinition->nTextureId );
			TEXTURE_DEFINITION * pTextureDefinition = (TEXTURE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_TEXTURE, nTextureDefinition );
			EVENT_DATA tEventData( pDefinition->tHeader.nId );
			if ( !pTextureDefinition )
			{ 
				DefinitionAddProcessCallback( DEFINITION_GROUP_TEXTURE, nTextureDefinition, 
					c_sParticleSystemTextureDefinitionLoaded, &tEventData );
			} else {
				c_sParticleSystemTextureDefinitionLoaded( pTextureDefinition, &tEventData );
			}
		}
	}

	V( e_ParticleSystemLoadGPUSimTextures( pDefinition ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sParticleSystemSetPosition( 
	PARTICLE_SYSTEM * pParticleSystem, 
	const VECTOR & vPositionIn,
	const VECTOR & vNormal,
	ROOMID idRoom,
	const MATRIX * pmWorld )
{
	if ( ! pParticleSystem->pDefinition )
	{ // we haven't loaded the definition yet
		pParticleSystem->vPosition = vPositionIn;
		pParticleSystem->vNormalToUse	= vNormal;
		VectorNormalize( pParticleSystem->vNormalToUse );
		pParticleSystem->vNormalIn		= pParticleSystem->vNormalToUse;
		pParticleSystem->idRoom	   = idRoom;
		if ( pmWorld )
			pParticleSystem->mWorld = *pmWorld;
		return;
	}

	VECTOR vPosition = vPositionIn;
	if( AppIsHellgate() )	// Travis - temporarily doing this - this is used in another way for Tugboat, until I can get a chance to add a new flag
	{
		if ( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_FIRST_PERSON ) != 0 ) //&&
			//( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_DONT_CHANGE_FOV ) != 0 )
		{
			c_CameraAdjustPointForFirstPersonFOV( AppGetCltGame(), vPosition, TRUE );
		}
	}

	if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_SYSTEM_FACE_UP )
	{
		pParticleSystem->vNormalToUse = sgvUpVector;
	} else {
		pParticleSystem->vNormalToUse = vNormal;
		VectorNormalize( pParticleSystem->vNormalToUse );
	}
	pParticleSystem->vNormalIn = vNormal;
	pParticleSystem->vPosition = vPosition;

	if ( pmWorld )
	{
		pParticleSystem->mWorld = *pmWorld;
		pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_WORLD_MATRIX_SET;
	}
	else
	{
		MatrixIdentity( &pParticleSystem->mWorld );
		pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_WORLD_MATRIX_SET;
	}

	if ( VectorIsZero( pParticleSystem->vPositionPrev ) || 
		(pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_CULLED_BY_DISTANCE ) != 0 ||
		(pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_CULLED_FOR_SPEED ) != 0 ||
		(pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DELAYING ) != 0 ||
		(pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_HAS_UPDATED_ONCE) == 0)
	{
		pParticleSystem->vPositionPrev = pParticleSystem->vPosition;
		pParticleSystem->fPositionPrevTime = pParticleSystem->fElapsedTime;
	}
	if ( VectorIsZero( pParticleSystem->vPositionLastParticleSpawned ) )
	{
		pParticleSystem->vPositionLastParticleSpawned = pParticleSystem->vPosition;
	}

	pParticleSystem->idRoom = idRoom;
	if ( pParticleSystem->nSoundId != INVALID_ID )
	{
		c_SoundSetPosition( pParticleSystem->nSoundId, vPosition );
	}
	if ( pParticleSystem->nFootstepId != INVALID_ID )
	{
		c_SoundSetPosition( pParticleSystem->nFootstepId, vPosition );
	}

	if ( pParticleSystem->nLightId != INVALID_ID )
	{
		VECTOR vNormalUp( 0, 0, 1 );
		const VECTOR vSystemNormal = pParticleSystem->vNormalToUse;
		if ( VectorLengthSquared( vSystemNormal - vNormalUp ) < 0.01f )
		{
			vNormalUp	 = VECTOR( 0, -1, 0 );
		} else if ( VectorLengthSquared( vSystemNormal + vNormalUp ) < 0.01f ) {
			vNormalUp	 = VECTOR( 0,  1, 0 );
		} else {
			VECTOR vNormalRight;
			VectorCross( vNormalRight, vNormalUp, vSystemNormal );
			VectorNormalize( vNormalRight, vNormalRight );
			VectorCross( vNormalUp, vNormalRight, vSystemNormal );
			VectorNormalize( vNormalUp, vNormalUp );
		}
		MATRIX mSystemMatrix;
		MatrixFromVectors( mSystemMatrix, pParticleSystem->vPosition, vNormalUp, vSystemNormal );

		VECTOR vLightPosition;
		MatrixMultiply( &vLightPosition, &pParticleSystem->pDefinition->vLightOffset, &mSystemMatrix );

		e_LightSetPosition( pParticleSystem->nLightId, vLightPosition );
	}

	if ( AppIsHellgate() && sParticleSystemUsesHavokFX( pParticleSystem ) && (pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_USE_HAVOKFX_PARTICLES) != 0 )
	{
		float fZOffset = c_sGetValueFromPath( 0.0f, pParticleSystem->pDefinition->tLaunchOffsetZ );
		VECTOR4 vEmitterPos( pParticleSystem->vPosition, 0.0f );
		vEmitterPos.fZ += fZOffset;
		e_HavokFXSetSystemParamVector( pParticleSystem->nId, HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_POS, vEmitterPos );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static float sParticleSystemGetRopeLength(
	PARTICLE_SYSTEM * pParticleSystem )
{
	float fDistanceCurr = 0;
	for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
		nKnotCurr != INVALID_ID; 
		nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr ) )
	{
		KNOT * pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );
		fDistanceCurr += pKnotCurr ? pKnotCurr->fSegmentLength : 0.0f;
	}
	return fDistanceCurr;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline float sRandGetFloat( 
	float fMax )
{
	return RandGetFloat(sgParticlesRand, 0.0f, fMax);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sGetOtherSystemDef ( 
	PARTICLE_SYSTEM_DEFINITION * pParticleSystemDef,
	char * pszOtherSystemName,
	int & nOtherSystemDef )
{
	BOOL bIsBlank = FALSE;
	if ( pszOtherSystemName[ 0 ] == 0 )
		bIsBlank = TRUE;
	if ( PStrCmp( pszOtherSystemName, BLANK_STRING ) == 0 )
		bIsBlank = TRUE;

	if ( bIsBlank )
	{
		nOtherSystemDef = INVALID_ID;
	} else

	if (AppIsHammer() || nOtherSystemDef == INVALID_ID)
	{
		char szPath[ MAX_XML_STRING_LENGTH ];
		unsigned int nLength = PStrGetPath( szPath, MAX_XML_STRING_LENGTH, pParticleSystemDef->tHeader.pszName );
		char szParticleName[ DEFAULT_FILE_WITH_PATH_SIZE ];
		if ( nLength == PStrLen( pParticleSystemDef->tHeader.pszName ) )
			PStrCopy( szParticleName, pszOtherSystemName, DEFAULT_FILE_WITH_PATH_SIZE );
		else
			PStrPrintf( szParticleName, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szPath, pszOtherSystemName );
		nOtherSystemDef = DefinitionGetIdByName( DEFINITION_GROUP_PARTICLE, szParticleName );
	}
}

//-------------------------------------------------------------------------------------------------
// this is a wrapper function to help Hammer do collision checks
//-------------------------------------------------------------------------------------------------
static BOOL sRoomUsesHavokFX( 
	ROOMID idRoom )
{
	if (!AppIsHellgate())
		return FALSE;
	if (!AppIsHammer())
	{
		GAME * pGame = AppGetCltGame();
		ROOM * pRoom = RoomGetByID( pGame, idRoom );
		if ( ! pRoom )
			return FALSE;
		LEVEL * pLevel = RoomGetLevel( pRoom );
		if ( ! pLevel )
			return FALSE;
		//const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet( pRoom->pLevel );
		//return pLevelDef ? pLevelDef->bCanUseHavokFX : FALSE;
		const DRLG_DEFINITION * pDRLGDef = RoomGetDRLGDef( pRoom );
		return pDRLGDef ? pDRLGDef->bCanUseHavokFX : FALSE;
	}
	else
	{
		return TRUE;
	}
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL c_sParticleSystemApplyDefinition( 
	PARTICLE_SYSTEM * pParticleSystem )
{
#if !ISVERSION(SERVER_VERSION)
	PARTICLE_SYSTEM_DEFINITION * pDefinition = (PARTICLE_SYSTEM_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_PARTICLE, pParticleSystem->nDefinitionId );
	if ( ! pDefinition )
		return FALSE;

	if ( e_HavokFXIsEnabled() && ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_CAN_USE_HAVOKFX ) != 0 &&
		 !AppIsHammer() && (pParticleSystem->idRoom == INVALID_ID || !RoomGetByID( AppGetCltGame(), pParticleSystem->idRoom )) )
	{// we need a room to decide whether to use havok fx.  So wait for it.
		return FALSE;
	}

#ifdef INTEL_CHANGES
	BOOL bIsAsync = ( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ASYNC ) != 0 );
	MEMORYPOOL * pMemoryPool = NULL;
	if ( bIsAsync )
	{
		// figure out which memory pool to use
		// async systems should be in the async particle systems hash
		ASYNC_PARTICLE_SYSTEM *pAsyncParticleSystem = HashGet( sgtAsyncParticleSystems, pParticleSystem->nId );
		ASSERT( pAsyncParticleSystem != NULL );
		pMemoryPool = pAsyncParticleSystem->pMemoryPool;
	}
#endif

	VECTOR vPositionIn = pParticleSystem->vPosition;

	pParticleSystem->pDefinition = pDefinition;
	ASSERT( pParticleSystem->pDefinition );

	pParticleSystem->fMinParticlesAllowedToDrop = (float)MIN( 1.0f, pDefinition->fMinParticlesPercentDropRate );


	MATRIX * pmWorld = (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_WORLD_MATRIX_SET ) != 0 ? &pParticleSystem->mWorld : NULL;
	c_sParticleSystemSetPosition( pParticleSystem, vPositionIn, pParticleSystem->vNormalIn, pParticleSystem->idRoom, pmWorld );

	if ( c_sParticleSystemHasRope ( pParticleSystem ) )
	{
		if ( pDefinition->nKnotCountMax < pDefinition->nKnotCount )
			pDefinition->nKnotCountMax = pDefinition->nKnotCount;

#ifdef INTEL_CHANGES
		ArrayInit(pParticleSystem->tKnots, pMemoryPool, MAX(MAX(pDefinition->nKnotCount, pDefinition->nKnotCountMax), 5));
#else
		ArrayInit(pParticleSystem->tKnots, NULL, MAX(MAX(pDefinition->nKnotCount, pDefinition->nKnotCountMax), 5));
#endif
	}
#ifdef INTEL_CHANGES
	ArrayInit(pParticleSystem->tParticles, pMemoryPool, 10);
#else
	ArrayInit(pParticleSystem->tParticles, NULL, 10);
#endif

	ArrayInit(pParticleSystem->tWaveOffsets, NULL, 1);

	pParticleSystem->fDuration = pParticleSystem->pDefinition->fDuration;
	if ( pParticleSystem->fCircleRadius == 0.0f )
		pParticleSystem->fCircleRadius = pParticleSystem->pDefinition->fCircleRadius;

	// add to the draw list
#ifdef INTEL_CHANGES
	// async systems should not be added to the draw list, but others should
	// make sure we have enough room if necessary
	if ( ! bIsAsync && ( sgnDrawListCount + 1 > sgnDrawListAllocated ) )
	{
		if ( sgnDrawListAllocated > 0 )
			sgnDrawListAllocated *= 2;
		else 
			sgnDrawListAllocated = 20;
		sgpDrawList = (PARTICLE_SYSTEM **) REALLOC( NULL, sgpDrawList, sgnDrawListAllocated * sizeof( PARTICLE_SYSTEM * ) );
	}
#else
	if ( sgnDrawListCount + 1 > sgnDrawListAllocated )
	{
		if ( sgnDrawListAllocated > 0 )
			sgnDrawListAllocated *= 2;
		else 
			sgnDrawListAllocated = 20;
		sgpDrawList = (PARTICLE_SYSTEM **) REALLOC( NULL, sgpDrawList, sgnDrawListAllocated * sizeof( PARTICLE_SYSTEM * ) );
	}
#endif

	// figure out some shader constants
	BOOL bGlowEnabled = (pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_GLOW) != 0;
	if ( bGlowEnabled &&
		! pParticleSystem->pDefinition->tLaunchRopeGlow.IsEmpty() &&
		( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_DRAW_ROPE ) != 0 )
	{
		pParticleSystem->pDefinition->dwFlags2 |= PARTICLE_SYSTEM_DEFINITION_FLAG2_GLOWCONSTANT;
	}
	else if ( bGlowEnabled && ! pParticleSystem->pDefinition->tParticleGlow.IsEmpty() )
	{
		pParticleSystem->pDefinition->dwFlags2 |= PARTICLE_SYSTEM_DEFINITION_FLAG2_GLOWCONSTANT;
	}
	else
	{
		pParticleSystem->pDefinition->dwFlags2 &= ~ PARTICLE_SYSTEM_DEFINITION_FLAG2_GLOWCONSTANT;
	}

	if ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_LOCK_DURATION )
		pParticleSystem->fDuration = pParticleSystem->pDefinition->fDuration;

	EFFECT_DEFINITION * pEffectDef = e_ParticleSystemGetEffectDef( pParticleSystem->pDefinition );
	if ( pEffectDef && TESTBIT( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_IS_SCREEN_EFFECT ) )
	{
		c_ParticleSystemSetDrawLayer( pParticleSystem->nId, DRAW_LAYER_SCREEN_EFFECT, TRUE );
	}

#ifdef INTEL_CHANGES
	// async systems are not added to draw list, other systems are
	if ( ! bIsAsync )
	{
		BOOL bAddedToDrawList = FALSE;
		for ( int i = 0; i < sgnDrawListCount; i++ )
		{
			if ( sgpDrawList[ i ]->pDefinition->nDrawOrder >  pParticleSystem->pDefinition->nDrawOrder )
				continue;
			if ( sgpDrawList[ i ]->pDefinition->nDrawOrder == pParticleSystem->pDefinition->nDrawOrder )
			{
				if ( sgpDrawList[ i ]->pDefinition->tHeader.nId > pParticleSystem->pDefinition->tHeader.nId )
					continue;
			}
			MemoryMove( sgpDrawList + i + 1, (sgnDrawListAllocated - i - 1) * sizeof(PARTICLE_SYSTEM*), sgpDrawList + i, (sgnDrawListCount - i) * sizeof(PARTICLE_SYSTEM *));
			sgpDrawList[ i ] = pParticleSystem;
			sgnDrawListCount++;
			bAddedToDrawList = TRUE;
			break;
		}
		if ( !bAddedToDrawList )
		{
			sgpDrawList[ sgnDrawListCount ] = pParticleSystem;
			sgnDrawListCount++;
		}
	}
#else
	BOOL bAddedToDrawList = FALSE;
	for ( int i = 0; i < sgnDrawListCount; i++ )
	{
		if ( sgpDrawList[ i ]->pDefinition->nDrawOrder >  pParticleSystem->pDefinition->nDrawOrder )
			continue;
		if ( sgpDrawList[ i ]->pDefinition->nDrawOrder == pParticleSystem->pDefinition->nDrawOrder )
		{
			if ( sgpDrawList[ i ]->pDefinition->tHeader.nId > pParticleSystem->pDefinition->tHeader.nId )
				continue;
		}
		MemoryMove( sgpDrawList + i + 1, (sgnDrawListAllocated - i - 1) * sizeof(PARTICLE_SYSTEM*), sgpDrawList + i, (sgnDrawListCount - i) * sizeof(PARTICLE_SYSTEM *));
		sgpDrawList[ i ] = pParticleSystem;
		sgnDrawListCount++;
		bAddedToDrawList = TRUE;
		break;
	}
	if ( !bAddedToDrawList )
	{
		sgpDrawList[ sgnDrawListCount ] = pParticleSystem;
		sgnDrawListCount++;
	}
#endif

	// follow system - it should already exist
	if ( pDefinition->pszFollowParticleSystem[ 0 ] != 0 &&
		pDefinition->nFollowParticleSystem == INVALID_ID )
	{
		if ( PStrCmp( pDefinition->pszFollowParticleSystem, BLANK_STRING ) == 0 )
		{
			pDefinition->pszFollowParticleSystem[ 0 ] = 0;
		} else {
			sGetOtherSystemDef( pDefinition, pDefinition->pszFollowParticleSystem, pDefinition->nFollowParticleSystem );
		}
	}
	pParticleSystem->nParticleSystemFollow = INVALID_ID;
	if ( pDefinition->nFollowParticleSystem != INVALID_ID )
	{
		int nOtherSystemId = pParticleSystem->nParticleSystemPrev;
		while ( nOtherSystemId != INVALID_ID )
		{
			PARTICLE_SYSTEM* pOtherSystem = HashGet(sgtParticleSystems, nOtherSystemId);
			ASSERT_BREAK(pOtherSystem);

			if ( pOtherSystem->pDefinition->tHeader.nId == pDefinition->nFollowParticleSystem )
			{
				pParticleSystem->nParticleSystemFollow = nOtherSystemId;
				break;
			}
			nOtherSystemId = pOtherSystem->nParticleSystemPrev;
		}
	}

	BOOL bKillSystem = FALSE;
	BOOL bForceNonHavokFX = FALSE;
	if ( e_HavokFXIsEnabled() )
	{
		if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_NOT_WITH_HAVOKFX ) != 0 &&
			 ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_FORCE_NON_HAVOK_FX ) == 0 )
			bKillSystem = TRUE;

		if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_CAN_USE_HAVOKFX ) != 0 )
		{
			if ( (!AppIsHammer() && pParticleSystem->idRoom == INVALID_ID) || ! sRoomUsesHavokFX( pParticleSystem->idRoom ) )
			{
				bKillSystem = TRUE;
			}
			else
			{
				HAVOKFX_PARTICLE_TYPE eHavokFXParticleType = ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_USE_HAVOKFX_PARTICLES ) ? HAVOKFX_PARTICLE : HAVOKFX_RIGID_BODY;

				if ( ! e_HavokAddSystem( pParticleSystem->nId, eHavokFXParticleType ) )
				{
					bKillSystem = TRUE;
				}
				else
					pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_USING_HAVOK_FX;
			}
			if ( bKillSystem )
				bForceNonHavokFX = TRUE;
		}

	} else {
		if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_CAN_USE_HAVOKFX ) != 0 )
			bKillSystem = TRUE;
	}

	{
		BOOL bCensorshipEnabled = c_ParticleTestGlobalBit( PARTICLE_GLOBAL_BIT_CENSORSHIP_ENABLED );
		if ( ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_CENSOR ) != 0 && bCensorshipEnabled )
			bKillSystem = TRUE;

		if ( ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_CENSOR_REPLACEMENT ) != 0 && ! bCensorshipEnabled )
			bKillSystem = TRUE;
	}

	BOOL bForceNonAsync = ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_FORCE_NON_ASYNC ) != 0;
	{
		BOOL bAsyncEnabled = c_ParticleTestGlobalBit( PARTICLE_GLOBAL_BIT_ASYNC_SYSTEMS_ENABLED );
		if ( bForceNonAsync )
			bAsyncEnabled = FALSE;

		if ( ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_ASYNC_SIMULATION ) != 0 && ! bAsyncEnabled )
			bKillSystem = TRUE;

		if ( ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_ASYNC_SIMULATION_REPLACEMENT ) != 0 && bAsyncEnabled )
			bKillSystem = TRUE;

#ifdef INTEL_CHANGES
		// do we need to spawn off an async system to shadow this system?
		if (
			bAsyncEnabled &&
			( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_ASYNC_SIMULATION ) != 0 ) &&
			!bIsAsync &&
			( sgnParticleSystemAsyncCount < sgnParticleSystemAsyncMax )
			)
		{
			// okay, we can async, we should async, and we're not our own twin
			// let's make a twin!
			// INTEL_NOTE: we should do something with this return value -- system might not be created
			c_ParticleSystemNew( pParticleSystem->nDefinitionId, 
				&pParticleSystem->vPosition,
				&pParticleSystem->vNormalToUse,
				pParticleSystem->idRoom,
				pParticleSystem->nId,
				&pParticleSystem->vVelocity,
				( ( pParticleSystem->dwFlags && PARTICLE_SYSTEM_FLAG_FORCE_NON_HAVOK_FX ) != 0 ),
				FALSE,
				TRUE );
		}
#endif
	}

	if ( ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_USES_GPU_SIM ) != 0 
		&& !e_ParticlesIsGPUSimEnabled() ) 
		bKillSystem = TRUE; 

	{
		BOOL bDX10Enabled = c_ParticleTestGlobalBit( PARTICLE_GLOBAL_BIT_DX10_ENABLED );
		if ( ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10 ) != 0 && ! bDX10Enabled ) 
			bKillSystem = TRUE; 

		if ( ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_REPLACEMENT ) != 0 && bDX10Enabled ) 
			bKillSystem = TRUE; 
	}

	if ( ! bKillSystem )
	{
		// just in case we haven't loaded it yet
		c_ParticleSystemLoad( pParticleSystem->nDefinitionId );
	}

	int nParticleSystemId = pParticleSystem->nId;
	ROOMID idRoom = pParticleSystem->idRoom;
	REF(idRoom);

	// Next system - create it
	sGetOtherSystemDef (pDefinition,
						pDefinition->pszNextParticleSystem,
						pDefinition->nNextParticleSystem );
	if ( pDefinition->nNextParticleSystem != INVALID_ID )
	{ // this is just in case we added a next system while the definition was being loaded - so add to the end
		c_ParticleSystemAddToEnd( nParticleSystemId, pDefinition->nNextParticleSystem, bForceNonHavokFX, bForceNonAsync );
	}

	// rope end system - create it
	sGetOtherSystemDef (pDefinition,
						pDefinition->pszRopeEndParticleSystem,
						pDefinition->nRopeEndParticleSystem );

	// there is a subtle memory issue here.  We might have reallocated the particle system array in the above calls
	pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);

	if ( pDefinition->nRopeEndParticleSystem != INVALID_ID )
	{
		if ( pParticleSystem->nParticleSystemRopeEnd != INVALID_ID )
		{
			c_ParticleSystemAddToEnd( pParticleSystem->nParticleSystemRopeEnd, pDefinition->nRopeEndParticleSystem );
		} else {
			c_ParticleSystemSetRopeEndSystem( nParticleSystemId, pDefinition->nRopeEndParticleSystem );
		}
	}

	// there is a subtle memory issue here.  We might have reallocated the particle system array in the above calls
	pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	ASSERT(pParticleSystem);
	if (pParticleSystem)
	{
		// reapply the flags that might have been set before the definition was loaded
		c_sParticleSystemSetFlag( nParticleSystemId, pParticleSystem->dwFlags, TRUE, FALSE );
		c_ParticleSystemSetRegion( nParticleSystemId, pParticleSystem->nRegionID );
	}

	// add ref here, in case the system gets killed and then release
	sParticleSystemAddRef( pDefinition );

	if ( bKillSystem )
	{
		c_ParticleSystemKill( nParticleSystemId, FALSE, TRUE );
		pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_MOVE_WHEN_DEAD;
	} 
	else if ( sParticleSystemUsesHavokFX( pParticleSystem ) )
	{
		VECTOR4 vCrawl( 0.0f, 0.0f, 0.0f, 0.0f );
		if ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_HAVOKFX_CRAWL )
			vCrawl.fX = 1.0f;
		else
			vCrawl.fX = -1.0f;

		e_HavokFXSetSystemParamVector(pParticleSystem->nId, HAVOKFX_PHYSICS_SYSTEM_PARAM_CRAWL_OR_FLY, vCrawl );

	} 
	else if ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_USES_GPU_SIM )
	{
		V( e_ParticleSystemGPUSimInit( pParticleSystem ) );
	}

	// CHB 2007.09.12 - Don't bother starting worker thread unless
	// async is enabled. If async is enabled later, the worker
	// thread will be created in sShouldSkipProcessing().
	if ( bIsAsync && e_OptionState_GetActive()->get_bAsyncEffects() )
	{
		PARTICLE_SYSTEM * pOriginalParticleSystem = HashGet( sgtParticleSystems, nParticleSystemId );
		ASYNC_PARTICLE_SYSTEM * pAsyncParticleSystem = HashGet( sgtAsyncParticleSystems, nParticleSystemId );
		if ( pAsyncParticleSystem && pOriginalParticleSystem )
		{
			pAsyncParticleSystem->bMainThreadEnabled = TRUE;
			sCreateAsyncParticleSystemJob( pOriginalParticleSystem, pAsyncParticleSystem );
		}
	}
	// CML 2007.05.14 - Hack to get dPVS working.
	c_ParticleSystemSetVisible( pParticleSystem->nId, TRUE );
#else
    UNREFERENCED_PARAMETER(pParticleSystem);
#endif //SERVER_VERSION

return TRUE;
}

#if DEBUG_MEMORY_ALLOCATIONS	
#define	useHeaderAndFooter true
#else
#define useHeaderAndFooter false
#endif	
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int c_ParticleSystemNew( 
	int nParticleSystemDefId, 
	const VECTOR * pvPositionIn, 
	const VECTOR * pvNormalIn, 
	ROOMID idRoom,
	int nParticleSystemPrev,
	const VECTOR * pvVelocityIn,
	BOOL bForceNonHavokFX,
	BOOL bForceNonAsync,
#ifdef INTEL_CHANGES
	BOOL bIsAsync,
#endif
	const MATRIX * pmWorld )
{
	// Add to list
#ifdef INTEL_CHANGES
	int nParticleSystemId = 0;
	PARTICLE_SYSTEM* pParticleSystem = NULL;
	if ( bIsAsync )
	{
		// figure out which async data index is available
		int nDataIndex = -1;
		for( int i = 0; i < PARTICLE_SYSTEM_ASYNC_MAX; i++ )
		{
			if( sgbParticleSystemAsyncVerticesBufferAvailable[i] )
			{
				// mark this one for our use
				sgbParticleSystemAsyncVerticesBufferAvailable[i] = FALSE;
				nDataIndex = i;

				if ( ! sgpParticleSystemAsyncVerticesBuffer[ i ] )
				{
					ASSERT_RETINVALID( ! sgpParticleSystemAsyncMainThreadVerticesBuffer[ i ] );
					
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
					sgpParticleSystemAsyncMainThreadVerticesBuffer[ i ] = (PARTICLE_QUAD_VERTEX *) MALLOC( g_StaticAllocator, PARTICLE_SYSTEM_ASYNC_PARTICLES_MAX * sizeof( PARTICLE_QUAD_VERTEX ) );			
					sgpParticleSystemAsyncVerticesBuffer[ i ] = (PARTICLE_QUAD_VERTEX *) MALLOC( g_StaticAllocator, PARTICLE_SYSTEM_ASYNC_PARTICLES_MAX * sizeof( PARTICLE_QUAD_VERTEX ) );			
#else					
					sgpParticleSystemAsyncMainThreadVerticesBuffer[ i ] = (PARTICLE_QUAD_VERTEX *) MALLOC( NULL, PARTICLE_SYSTEM_ASYNC_PARTICLES_MAX * sizeof( PARTICLE_QUAD_VERTEX ) );			
					sgpParticleSystemAsyncVerticesBuffer[ i ] = (PARTICLE_QUAD_VERTEX *) MALLOC( NULL, PARTICLE_SYSTEM_ASYNC_PARTICLES_MAX * sizeof( PARTICLE_QUAD_VERTEX ) );			
#endif
				}
				sgnParticleSystemAsyncCount++;
				break;
			}
		}
		if( nDataIndex == -1 )
		{
			return INVALID_ID;
		}

		// add to the async particles hash, using the old ID
		nParticleSystemId = nParticleSystemPrev;
		ASYNC_PARTICLE_SYSTEM *pAsyncParticleSystem = HashAdd( sgtAsyncParticleSystems, nParticleSystemId );
		if (!pAsyncParticleSystem)
		{
			return INVALID_ID;
		}

		// initialize the async structure
		
#if USE_MEMORY_MANAGER		
		
		CMemoryAllocatorHEAP<true, useHeaderAndFooter, 80>* heapAllocator = new CMemoryAllocatorHEAP<true, useHeaderAndFooter, 80>(L"ParticleHEAP");
		ASSERT(heapAllocator->Init(IMemoryManager::GetInstance().GetDefaultAllocator(), 8 * MEGA, 10 * MEGA, 10 * MEGA));
		ASSERT(IMemoryManager::GetInstance().AddAllocator(heapAllocator));
		pAsyncParticleSystem->pMemoryPoolHEAP = heapAllocator;

		CMemoryAllocatorWRAPPER* poolAllocator = new CMemoryAllocatorWRAPPER();
		poolAllocator->Init(IMemoryManager::GetInstance().GetDefaultAllocator(), heapAllocator);
		pAsyncParticleSystem->pMemoryPool = poolAllocator;
		
#else
		pAsyncParticleSystem->pMemoryPool = MemoryPoolInit(L"c_particles", MEMORY_BINALLOCATOR_MT);		
#endif
		
		pAsyncParticleSystem->hCallbackFinished = CreateEvent( NULL, TRUE, TRUE, NULL );
		pAsyncParticleSystem->nAsyncVerticesBufferIndex = nDataIndex;
		pAsyncParticleSystem->fAsyncUpdateTime = 0.0f;
		
		// setup pointer for the rest of this function
		pParticleSystem = &( pAsyncParticleSystem->tParticleSystem );
	}
	else
	{
		nParticleSystemId = sgnParticleSystemGUID++;
		pParticleSystem = HashAdd(sgtParticleSystems, nParticleSystemId);
	}
#else
	int nParticleSystemId = sgnParticleSystemGUID++;
	PARTICLE_SYSTEM* pParticleSystem = HashAdd(sgtParticleSystems, nParticleSystemId);
#endif
	if (!pParticleSystem)
	{
		return INVALID_ID;
	}
	pParticleSystem->nId = nParticleSystemId;
	pParticleSystem->nParticleSystemPrev = nParticleSystemPrev;
	pParticleSystem->nDefinitionId = nParticleSystemDefId;

	pParticleSystem->fDuration					= 0.0f;
	pParticleSystem->fElapsedTime				= 0.0f;
	pParticleSystem->dwFlags					= 0;
	pParticleSystem->fTimeLastParticleCreated	= 0.0f;
	pParticleSystem->fLastParticleBurstTime		= -1.0f; 
	pParticleSystem->fRangePercent				= 1.0f; 
	pParticleSystem->fGlobalAlpha				= 1.0f;

	pParticleSystem->nLightId = INVALID_ID;
	pParticleSystem->nSoundId = INVALID_ID;
	pParticleSystem->nFootstepId = INVALID_ID;
	pParticleSystem->pDefinition = NULL;

	pParticleSystem->nPortalID = INVALID_ID;
	pParticleSystem->nRegionID = REGION_INVALID;

	pParticleSystem->vRopeEndNormal = VECTOR( 0, 0, 1 );
	pParticleSystem->vVelocity = pvVelocityIn ? *pvVelocityIn : VECTOR(0);

	// we will set all of these officially when applying the definition
	pParticleSystem->vPosition = pvPositionIn ? * pvPositionIn : VECTOR( 1000.0f );
	pParticleSystem->vNormalIn = pvNormalIn ? *pvNormalIn : VECTOR( 0.0f, 0.0f, 1.0f );
	VectorNormalize( pParticleSystem->vNormalIn );
	pParticleSystem->vNormalToUse = pParticleSystem->vNormalIn;
	pParticleSystem->idRoom	   = idRoom;
	if ( pmWorld )
	{
		pParticleSystem->mWorld = *pmWorld;
		pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_WORLD_MATRIX_SET;
	}
	else
		MatrixIdentity( &pParticleSystem->mWorld );

	// attractor
	pParticleSystem->vAttractor = pParticleSystem->vPosition;
	pParticleSystem->nParticleSystemNext	= INVALID_ID;
	pParticleSystem->nParticleSystemRopeEnd = INVALID_ID;
	pParticleSystem->nParticleSystemFollow  = INVALID_ID;

	pParticleSystem->nHavokFXParticleCount = 0;

	if ( bForceNonHavokFX )
		pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_FORCE_NON_HAVOK_FX;

	if ( bForceNonAsync )
		pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_FORCE_NON_ASYNC;

#ifdef INTEL_CHANGES
	if ( bIsAsync )
	{
		// mark twins with a flag so we don't loop forever
		pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_ASYNC;
	}

	// this will create an additional async system (a twin)
	// if the definition calls for it and the flags allow it
#endif
	c_sParticleSystemApplyDefinition( pParticleSystem );

	pParticleSystem->vPositionPrev = VECTOR( 0.0f );

	return nParticleSystemId;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int c_ParticleSystemAddToEnd( 
	int nParticleSystemFirstId,
	int nParticleSystemDefId,
	BOOL bForceNonHavokFX,
	BOOL bForceNonAsync )
{
	PARTICLE_SYSTEM* pParticleSystemLast = HashGet(sgtParticleSystems, nParticleSystemFirstId);
	if (!pParticleSystemLast)
	{
		return INVALID_ID;
	}

	while ( pParticleSystemLast )
	{
		// if the system is already there, then don't add another one.  Otherwise, things stack up
		if ( pParticleSystemLast->nDefinitionId == nParticleSystemDefId )
			return pParticleSystemLast->nId;

		if ( pParticleSystemLast->nParticleSystemNext == INVALID_ID )
			break;
		PARTICLE_SYSTEM * pNext = HashGet(sgtParticleSystems, pParticleSystemLast->nParticleSystemNext);
		if ( ! pNext )
			break;
		pParticleSystemLast = pNext;
	}

	int nParticleSystemLastId = pParticleSystemLast->nId;
	VECTOR vPosition = pParticleSystemLast->vPosition;
	VECTOR vNormal   = pParticleSystemLast->vNormalIn;
	int nParticleSystemNewId = c_ParticleSystemNew( nParticleSystemDefId, 
													& vPosition,
													& vNormal,
													pParticleSystemLast->idRoom,
													nParticleSystemLastId,
													& pParticleSystemLast->vVelocity,
													bForceNonHavokFX,
													bForceNonAsync );

	// there is a subtle memory issue here.  We might have reallocated the particle system array in the above calls
	pParticleSystemLast = HashGet(sgtParticleSystems, nParticleSystemLastId);
	ASSERT(pParticleSystemLast);
	if (pParticleSystemLast)
	{
		pParticleSystemLast->nParticleSystemNext = nParticleSystemNewId;
	}

	return nParticleSystemNewId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int c_ParticleSystemSetRopeEndSystem( 
	int nParticleSystemId,
	int nRopeEndSystemDefId )
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pParticleSystem)
	{
		return INVALID_ID;
	}
	if ( pParticleSystem->nParticleSystemRopeEnd != INVALID_ID )
	{// remove the old one
		c_ParticleSystemDestroy( pParticleSystem->nParticleSystemRopeEnd );
	}

	int nParticleSystemNewId = INVALID_ID;
	if ( nRopeEndSystemDefId == INVALID_ID )
	{
		pParticleSystem->nParticleSystemRopeEnd = INVALID_ID;
	} 
	else 
	{
		nParticleSystemNewId = c_ParticleSystemNew( nRopeEndSystemDefId, 
													& pParticleSystem->vRopeEnd,
													& pParticleSystem->vRopeEndNormal,
													pParticleSystem->idRoom,
													nParticleSystemId );

		// there is a subtle memory issue here.  We might have reallocated the particle system array in the above calls
		pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
		ASSERT(pParticleSystem);
		if (pParticleSystem)
		{
			pParticleSystem->nParticleSystemRopeEnd = nParticleSystemNewId;
		}
	}

	return nParticleSystemNewId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int c_ParticleSystemGetRopeEndSystem( 
	int nParticleSystemId)
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pParticleSystem)
	{
		return INVALID_ID;
	}
	return pParticleSystem->nParticleSystemRopeEnd;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemWarpToCurrentPosition(
	int nParticleSystemId )
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pParticleSystem)
	{
		return;
	}
	pParticleSystem->vPositionPrev = pParticleSystem->vPosition;
	pParticleSystem->vPositionLastParticleSpawned = pParticleSystem->vPosition;
	pParticleSystem->vTrailPosLast = pParticleSystem->vPosition;
	if ( pParticleSystem->nParticleSystemNext != INVALID_ID )
	{
		c_ParticleSystemWarpToCurrentPosition( pParticleSystem->nParticleSystemNext );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleSystemMove  ( 
	int nParticleSystemId, 
	const VECTOR & vPosition, 
	VECTOR * pvNormalIn, 
	ROOMID idRoom,
	const MATRIX * pmWorld )
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pParticleSystem)
	{
		return;
	}

	VECTOR vNormal( 0.0f, 0.0f, 1.0f );
	if ( pvNormalIn && ! VectorIsZero( *pvNormalIn ) )
	{
		vNormal = *pvNormalIn;
	} 

	if ( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DEAD ) == 0 ||
		 ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_MOVE_WHEN_DEAD ) != 0 )
	{
		VECTOR vVelocity( 0.0f );
		c_sParticleSystemSetPosition( pParticleSystem, vPosition, vNormal, idRoom, pmWorld );
	}

	if ( pParticleSystem->nParticleSystemNext != INVALID_ID )
	{
		c_ParticleSystemMove( pParticleSystem->nParticleSystemNext, vPosition, &vNormal, idRoom, pmWorld );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static float c_sGetValueFromPath( 
	float fPathTime, 
	const CInterpolationPath<CFloatPair> & tPath)
{
	PARTICLE_HITCOUNT(PART_GETVALUE)
	CFloatPair tPair = tPath.GetValue( fPathTime );
	float fDiff = tPair.y - tPair.x;
	if ( fDiff )
	{
		float fOffset = sRandGetFloat( fDiff );
		return tPair.x + fOffset;
	}
	else
	{
		return tPair.x;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static float c_sGetSpawnRateValueFromPath( 
	PARTICLE_SYSTEM * pParticleSystem,
	float fPathTime, 
	const CInterpolationPath<CFloatPair> & tPath )
{
	PARTICLE_HITCOUNT(PART_GETVALUE)
	CFloatPair tPair = tPath.GetValue( fPathTime );
	float fDiff = tPair.y - tPair.x;
	if ( fDiff )
	{
		if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_USES_PARTICLE_SPAWN_THROTTLE )
		{
			return tPair.x + fDiff * pParticleSystem->fParticleSpawnThrottle;
		} else {
			float fOffset = sRandGetFloat( fDiff );
			return tPair.x + fOffset;
		}
	}
	else
	{
		return tPair.x;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static float c_sGetValueFromPath( 
	float fPathTime, 
	const CInterpolationPath<CFloatPair> & tPath,
	float fPercent,
	float fPercentMult )
{
	PARTICLE_HITCOUNT(PART_GETVALUE)
	CFloatPair tPair = tPath.GetValue( fPathTime );
	float fDiff = tPair.y - tPair.x;
	if ( fDiff )
	{
		fPercent *= fPercentMult;
		fPercent = fPercent - FLOOR( fPercent );
		return tPair.x + (fPercent * fDiff);
	}
	else
	{
		return tPair.x;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static float CubeRootCheap( 
	float fInput,
	float fMaxError )
{
	float fGuess = fInput / 10.0f; // I need a better first guess
	// get a decent first guess which is greater than solution
	float fGuessPrev = fGuess;
	while ( fGuess * fGuess * fGuess > fInput )
	{
		fGuessPrev = fGuess;
		fGuess /= 2.0f;
	}
	fGuess = fGuessPrev;

	// hone in on answer
	float fError;
	do {
		float fGuessSquared = fGuess * fGuess;
		float fNewGuess = fGuess - ( fGuessSquared * fGuess - fInput ) / ( 3.0f * fGuessSquared );

		fError = fabs( fNewGuess - fGuess );

		fGuess = fNewGuess;
	} while( fError > fMaxError );

	return fGuess;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static float c_sGetRadiusFromPath( 
	float fPathTime, 
	const CInterpolationPath<CFloatPair> * pPath )
{
	PARTICLE_HITCOUNT(PART_GETVALUE)
	CFloatPair tPair = pPath->GetValue( fPathTime );
	float fDiff = tPair.y - tPair.x;
	if ( fDiff != 0.0f )
	{
		float fCubedMin = tPair.x * tPair.x * tPair.x;
		float fCubedMax = tPair.y * tPair.y * tPair.y;
		float fCubedDiff = fCubedMax - fCubedMin;

		float fOffset = sRandGetFloat( fCubedDiff );
		float fCubedValue = fCubedMin + fOffset;

		return CubeRootCheap( fCubedValue, fDiff / 5.0f );

	} else {
		return tPair.x;
	}
}

//-------------------------------------------------------------------------------------------------
static PARTICLE * c_sLaunchParticle( 
	PARTICLE_SYSTEM * pParticleSystem, 
	float fPathTime, 
	VECTOR * pvPositionIn = NULL,
	VECTOR * pvNormalIn = NULL,
	float fScaleIn = 0.0f)
{
	// create a particle
	int nNewParticleId;
	PARTICLE * pParticle = pParticleSystem->tParticles.Add( & nNewParticleId );
	if ( ! pParticle ) // we can only hold so many particles - just bail if we can't get another
		return NULL;

	pParticle->dwFlags   = PARTICLE_FLAG_JUST_CREATED;
	pParticle->fScale    = fScaleIn != 0.0f ? fScaleIn : c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchScale );
	pParticle->fScaleInit = pParticle->fScale;
	pParticle->nFollowKnot = INVALID_ID;

	// Set Normal and Position
	VECTOR vNormal;
	if ( pvNormalIn && pvPositionIn )
	{
		vNormal = *pvNormalIn;
		pParticle->vPosition = *pvPositionIn;
	} 
	else if ( ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_PARTICLES_SPAWN_ON_FOLLOW_SYSTEM ) != 0 &&
		pParticleSystem->nParticleSystemFollow != INVALID_ID )
	{
		PARTICLE_SYSTEM* pParticleSystemFollow = HashGet(sgtParticleSystems, pParticleSystem->nParticleSystemFollow);
		ASSERT_RETFALSE(pParticleSystemFollow);
		int nFollowCount = pParticleSystemFollow->tParticles.Count();
		if ( !nFollowCount )
		{
			pParticleSystem->tParticles.Remove( nNewParticleId );
			return NULL;
		}
		int nParticleToSpawnOnCount = RandGetNum(sgParticlesRand, nFollowCount);
		int nParticleToSpawnOn = pParticleSystemFollow->tParticles.GetFirst();
		for ( int i = 0; i < nParticleToSpawnOnCount; i++ )
		{
			nParticleToSpawnOn = pParticleSystemFollow->tParticles.GetNextId( nParticleToSpawnOn );
			ASSERT_RETFALSE( nParticleToSpawnOn != INVALID_ID );
		}
		PARTICLE * pParticleToSpawnOn = pParticleSystemFollow->tParticles.Get( nParticleToSpawnOn );

		float fLength = VectorNormalize( vNormal, pParticleToSpawnOn->vVelocity );
		if ( fLength == 0.0f )
			vNormal = pParticleSystem->vNormalToUse;
		pParticle->vPosition = pParticleToSpawnOn->vPosition;
	} else {
		vNormal = pParticleSystem->vNormalToUse;
		VectorNormalize( vNormal );
		pParticle->vPosition = pParticleSystem->vPosition;
	}
	if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_SYSTEM_FACE_UP )
		vNormal = VECTOR( 0.0f, 0.0f, 1.0f );


	// Set the launch vector
	float fRotation = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchDirRotation );
	float fPitch    = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchDirPitch );
	// TRAVIS:sometimes we can get invalid numbers in here - catch 'em before they screw up the matrices!
	fPitch = VerifyFloat( fPitch, 360 );
	fRotation = VerifyFloat( fRotation, 360 );
	fRotation	= DegreesToRadians ( fRotation );
	fPitch		= DegreesToRadians ( fPitch );
	MATRIX mLaunchMatrix;
	MatrixRotationYawPitchRoll ( mLaunchMatrix, 0.0f, fPitch, fRotation );

	// got a zero matrix - just use identity
	if( mLaunchMatrix._11 == 0 && mLaunchMatrix._12 == 0 && mLaunchMatrix._13 == 0 )
	{
		MatrixIdentity( &mLaunchMatrix );
		vNormal = VECTOR( 0, 1, 0 );
	}
	VECTOR vNormalUp( 0, 0, 1 );
	if ( VectorLengthSquared( vNormal - vNormalUp ) < 0.01f )
	{
		vNormalUp	 = VECTOR( 0, -1, 0 );
	} else if ( VectorLengthSquared( vNormal + vNormalUp ) < 0.01f ) {
		vNormalUp	 = VECTOR( 0,  1, 0 );
	} else {
		VECTOR vNormalRight;
		VectorCross( vNormalRight, vNormalUp, vNormal );
		VectorNormalize( vNormalRight );
		VectorCross( vNormalUp, vNormalRight, vNormal );
		VectorNormalize( vNormalUp );
	}
	if( VerifyFloat( vNormal.fX, 1 ) != vNormal.fX )
	{
		vNormal = VECTOR( 0, 1, 0 );
		vNormalUp = VECTOR( 0, 0, 1 );
	}
	MATRIX mRotateToNormal;
	MatrixFromVectors( mRotateToNormal, VECTOR(0,0,0), vNormalUp, vNormal );
	MatrixMultiply( &mLaunchMatrix, &mLaunchMatrix, &mRotateToNormal );
	MATRIX mRotateToNormalInverse;
	MatrixInverse( &mRotateToNormalInverse, &mRotateToNormal );
	MatrixMultiply( &mLaunchMatrix, &mRotateToNormalInverse, &mLaunchMatrix );
	MatrixMultiplyNormal( &pParticle->vVelocity, & vNormal, &mLaunchMatrix );

	VECTOR vOffset;
	vOffset.fX = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchOffsetX );
	vOffset.fY = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchOffsetZ ); // and I mean Z
	vOffset.fZ = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchOffsetY ); // and I mean Y
	if ( ! pParticleSystem->pDefinition->tLaunchSphereRadius.IsEmpty() )
	{
		float fRadius = c_sGetRadiusFromPath( fPathTime, & pParticleSystem->pDefinition->tLaunchSphereRadius ); 
		VECTOR vRandVector = RandGetVector(sgParticlesRand);
		VectorNormalize( vRandVector, vRandVector );
		vOffset += vRandVector * fRadius;
	}
	if ( ! pParticleSystem->pDefinition->tLaunchCylinderRadius.IsEmpty() )
	{
	
		// I used the following page for reference if you're curious, we should make it faster - Colin
		// http://mathworld.wolfram.com/DiskPointPicking.html
		
		float flAngle = RandGetFloat( sgParticlesRand, TWOxPI );
		float fRadius = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchCylinderRadius);
		float flSquareRootRadius = sqrt( fRadius );
		
		VECTOR vCylinder;
		vCylinder.fX = flSquareRootRadius * cosf( flAngle );
		vCylinder.fZ = flSquareRootRadius * sinf( flAngle );
		vCylinder.fY = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchCylinderHeight );
		vOffset += vCylinder;
		
	}
	
	if ( ! VectorIsZero( vOffset ) )
	{
		MatrixMultiply( & vOffset, & vOffset, & mRotateToNormal );
		pParticle->vPosition += vOffset;
	}

	float fSpeedInit = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchSpeed );
	if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_RANGE_AFFECTS_SPEED )
		fSpeedInit *= pParticleSystem->fRangePercent;

	float fLaunchSpeedFromSystemForward = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchSpeedFromSystemForward );
	if ( fLaunchSpeedFromSystemForward != 0.0f )
	{
		float fSystemSpeed = VectorLength( pParticleSystem->vVelocity );
		if ( fSystemSpeed != 0.0f )
		{
			VECTOR vSystemDirection = pParticleSystem->vVelocity;
			vSystemDirection /= fSystemSpeed;
			// this assumes that we still have a normalized particle velocity
			float fSystemPercent = VectorDot( vSystemDirection, pParticle->vVelocity );
			fSpeedInit += fSystemSpeed * fSystemPercent;
		}
	}

	pParticle->vVelocity *= fSpeedInit;
	if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_FOLLOW_ROPE ) 
	{
		pParticle->vVelocity = VECTOR( fSpeedInit, 0 , 0 );
	}
	else if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ATTACH_PARTICLES )
	{
		pParticle->vVelocity = pParticleSystem->vVelocity;
	} 
	else 
	{
		// add particle system velocity
		float fLaunchVelocityFromSystem = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchVelocityFromSystem );
		if ( fLaunchVelocityFromSystem != 0.0f )
		{
			VECTOR vVelocityFromSystem;
			VectorScale( vVelocityFromSystem, pParticleSystem->vVelocity, fLaunchVelocityFromSystem );
			pParticle->vVelocity += vVelocityFromSystem;
		}
	}

	// Set orientation
	pParticle->fRotation = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchRotation );
	pParticle->fCenterRotation = 0.0f;

	// Set timing
	float fDuration = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tParticleDurationPath );
	if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_RANGE_AFFECTS_DURATION )
		fDuration *= pParticleSystem->fRangePercent;
	pParticle->fDuration = fDuration;
	pParticle->fElapsedTime = 0.0f;

	if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_RANDOM_ANIM_FRAME )
	{
		float fTotalFrames = (float) pParticleSystem->pDefinition->pnTextureFrameCount[ 0 ] * pParticleSystem->pDefinition->pnTextureFrameCount[ 1 ];
		if ( fTotalFrames > 1.0f )
			pParticle->fFrame = fTotalFrames * RandGetFloat(sgParticlesRand);
		else
			pParticle->fFrame = 0.0f;
	} else {
		pParticle->fFrame = 0.0f;
	}

	if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_RANDOMLY_MIRROR_TEXTURE )
	{
		if (RandSelect(sgParticlesRand, 1, 2))
		{
			pParticle->dwFlags |= PARTICLE_FLAG_MIRROR_TEXTURE;
		}
	}

	pParticle->fRopeDistance = 0.0f;
	pParticle->fRandPercent = sRandGetFloat( 1.0f );

	if ( sParticleSystemUsesHavokFX(pParticleSystem) )
	{
		//if ( pParticleSystem->nHavokFXParticleCount > 1000 )
		//	e_HavokFXRemoveFXRigidBody( 0, pParticleSystem->nId, TRUE );
		MATRIX mWorld;
		VECTOR vUp( 0, 0, 1 );
		VECTOR vForward;
		if ( VectorIsZeroEpsilon( pParticle->vVelocity ) )
			vForward = pParticleSystem->vNormalToUse;
		else
			VectorNormalize( vForward, pParticle->vVelocity );
		if ( fabsf(vForward.fZ) == 1.0f )
			vUp = VECTOR( 0, 1, 0 );
		MatrixFromVectors( mWorld, pParticle->vPosition, vUp, vForward );
		if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_USE_HAVOKFX_PARTICLES ) == 0 )
			if(e_HavokFXAddFXRigidBody( mWorld, pParticle->vVelocity, pParticleSystem->nId ))
     		pParticleSystem->nHavokFXParticleCount++;	

		pParticleSystem->tParticles.Remove( nNewParticleId );
		pParticle = NULL;
	}

	return pParticle;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeAdjustKnotCountAndLength(
	GAME * pGame,
	PARTICLE_SYSTEM * pParticleSystem,
	PARTICLE_SYSTEM * pParticleSystemFollow );
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeSetPositions(
	GAME * pGame,
	PARTICLE_SYSTEM * pParticleSystem )
{
	PARTICLE_SYSTEM * pParticleSystemFollow = NULL;
	if ( ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_USE_FOLLOW_SYSTEM_ROPE ) != 0 &&
		pParticleSystem->nParticleSystemFollow != INVALID_ID )
	{
		pParticleSystemFollow = HashGet(sgtParticleSystems, pParticleSystem->nParticleSystemFollow);
	} 


	if ( pParticleSystemFollow && pParticleSystemFollow->tKnots.GetFirst() != INVALID_ID )
	{	// follow the position of the follow system's rope

		c_ParticleSystemSetRopeEnd( pParticleSystem->nId, 
									pParticleSystemFollow->vRopeEnd, 
									pParticleSystemFollow->vRopeEndNormal );

		c_sRopeAdjustKnotCountAndLength( pGame, pParticleSystem, pParticleSystemFollow );

		for ( int nKnotCurrId = pParticleSystem		 ->tKnots.GetFirst(),
				nKnotFollowId = pParticleSystemFollow->tKnots.GetFirst(); 
			nKnotCurrId != INVALID_ID && nKnotFollowId != INVALID_ID; 
			nKnotCurrId		= pParticleSystem		->tKnots.GetNextId( nKnotCurrId ),
			nKnotFollowId	= pParticleSystemFollow	->tKnots.GetNextId( nKnotFollowId ))
		{
			KNOT * pKnotCurr	= pParticleSystem		->tKnots.Get( nKnotCurrId );
			KNOT * pKnotFollow	= pParticleSystemFollow	->tKnots.Get( nKnotFollowId );

			pKnotCurr->vPosition = pKnotFollow->vPosition;
			pKnotCurr->vNormal	 = pKnotFollow->vNormal;
			pKnotCurr->vPositionIdeal = pKnotFollow->vPosition;
			pKnotCurr->fSegmentLength = pKnotFollow->fSegmentLength;
			pKnotCurr->fDistanceFromStart = pKnotFollow->fDistanceFromStart;
		}

	} 
	else 
	{
		// update the rope positions based on rope end or normal
		VECTOR vDirToEnd = pParticleSystem->vNormalToUse;
		int nKnotTotal = pParticleSystem->tKnots.Count();
		float fSegmentLength = pParticleSystem->pDefinition->fSegmentSize;
		float fDistanceToEnd = 0.0f;
		BOOL bRopeEndLerps = pParticleSystem->pDefinition->fRopeEndSpeed < 1.0f && ! VectorIsZero( pParticleSystem->vRopeEndIdeal );
		if (( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ROPE_END_ATTACHED ) != 0 || bRopeEndLerps )
		{
			VECTOR vDeltaToRopeEnd = pParticleSystem->vRopeEnd - pParticleSystem->vPosition;
			fDistanceToEnd = VectorLength( vDeltaToRopeEnd );
			if ( fDistanceToEnd != 0.0f && nKnotTotal > 1 )
			{
				fSegmentLength = fDistanceToEnd / ( nKnotTotal - 1 );

				if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_STRAIGHT_TO_END )
				{
					pParticleSystem->vNormalToUse = vDeltaToRopeEnd;
					pParticleSystem->vNormalToUse /= fDistanceToEnd;
				} 
				else if ( pParticleSystem->fRopeBendMaxXY > 0.0f || pParticleSystem->fRopeBendMaxZ > 0.0f )
				{
					VECTOR vNormalToEnd = vDeltaToRopeEnd;
					vNormalToEnd /= fDistanceToEnd;
					VECTOR vSystemNormal = pParticleSystem->vNormalToUse;
					VectorNormalize( vSystemNormal );
					float fDot = VectorDot( vSystemNormal, vNormalToEnd );
					if ( fDot < pParticleSystem->fRopeBendMaxXY )
					{
						fDot = PIN( fDot, -1.0f, 1.0f );
						float fAngle = acosf( fDot );
						if ( fAngle != 0.0f )
						{
							float fLerp = acosf( pParticleSystem->fRopeBendMaxXY ) / fAngle;
							pParticleSystem->vNormalToUse = VectorLerp( vSystemNormal, vNormalToEnd, fLerp );
							VectorNormalize( pParticleSystem->vNormalToUse );
						}
					}
				}
			}
		} 
		
		if ( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ROPE_END_ATTACHED ) == 0 )
		{
			float fRopeLength = pParticleSystem->pDefinition->fSegmentSize * (nKnotTotal - 1);
			VECTOR vDeltaToEnd;
			VectorScale( vDeltaToEnd, vDirToEnd, fRopeLength );
			c_ParticleSystemSetRopeEnd( pParticleSystem->nId, 
										pParticleSystem->vPosition + vDeltaToEnd,
										-pParticleSystem->vNormalToUse,
										FALSE );

			if ( bRopeEndLerps )
				fDistanceToEnd = fRopeLength;
		}

		VECTOR vPosCurr = pParticleSystem->vPosition;
		VECTOR vSystemNormal = pParticleSystem->vNormalToUse;
		VECTOR vDirCurr	= vSystemNormal;
		// there is some extra computation here to curve the rope to the target
		float fLerpDelta = 1.0f / (float)nKnotTotal; 
		float fLerpCurr = fLerpDelta;
		int nKnotCount = 0;

		float fTotalLength = 0;
		for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
			nKnotCurr != INVALID_ID; 
			nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr ) )
		{
			KNOT * pKnot = pParticleSystem->tKnots.Get( nKnotCurr );

			if ( nKnotTotal - nKnotCount != 0 )
				pKnot->fSegmentLength = fDistanceToEnd / (float)(nKnotTotal - nKnotCount);
			else 
				pKnot->fSegmentLength = pParticleSystem->pDefinition->fSegmentSize;
			fTotalLength += pKnot->fSegmentLength;
			pKnot->vNormal			= vDirCurr;
			pKnot->vPositionIdeal	= vPosCurr;
			pKnot->vPosition		= vPosCurr;

			vDirToEnd = pParticleSystem->vRopeEnd - vPosCurr;
			fDistanceToEnd = VectorLength( vDirToEnd );
			if ( fDistanceToEnd != 0.0f )
				vDirToEnd /= fDistanceToEnd; // normalize

			VECTOR vPosDelta = vDirCurr;
			vPosDelta *= pKnot->fSegmentLength;
			vPosCurr += vPosDelta;
			vDirCurr = VectorLerp( vDirToEnd, vSystemNormal, fLerpCurr );
			VectorNormalize( vDirCurr, vDirCurr );
			nKnotCount++;
			fLerpCurr = fLerpDelta * ((int)nKnotCount + 1);
		}
		fTotalLength -= fSegmentLength;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sCreateWave( 
	PARTICLE_SYSTEM * pParticleSystem, 
	float fPathTime,
	int nIndex )
{
	VECTOR vWave;
	VECTOR* vWavePrev = NULL;
	if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ROPE_WAVES_RANDOM_CYCLE )
	{
		int nIndexPrev = nIndex - 1;
		if ( nIndexPrev >= 0 )
		{
			vWavePrev = pParticleSystem->tWaveOffsets.Get(nIndexPrev);
		}

		switch (RandGetNum(sgParticlesRand) % 4)
		{
			case 0: vWave.fX = -1.0f; vWave.fY = -1.0f; break;
			case 1: vWave.fX = 1.0f; vWave.fY = -1.0f; break;
			case 2: vWave.fX = -1.0f; vWave.fY = 1.0f; break;
			case 3: vWave.fX = 1.0f; vWave.fY = 1.0f; break;
		}

		// make sure that we don't repeat the last wave...
		if(vWavePrev)
		{
			if ( ( vWavePrev->fX > 0.0f && vWave.fX > 0.0f ) ||
				 ( vWavePrev->fX < 0.0f && vWave.fX < 0.0f ) )
			{
				if ( ( vWavePrev->fY > 0.0f && vWave.fY > 0.0f ) ||
					 ( vWavePrev->fY < 0.0f && vWave.fY < 0.0f ) )
				{
					switch (RandGetNum(sgParticlesRand, 3))
					{
					case 0:
						vWave.fX = -vWave.fX;
					case 1:
						vWave.fY = -vWave.fY;
						break;
					case 2:
						vWave.fX = -vWave.fX;
						break;
					}
				}
			}
		}
	} else {
		ASSERT( MAX_WAVE_OFFSETS % 2 == 0 );
		vWave.fX = nIndex % 2 == 0 ? -1.0f : 1.0f;
		vWave.fY = nIndex % 2 == 0 ? -1.0f : 1.0f;
	}
	vWave.fZ = 0.0f;

	vWave.fX *= c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tRopeWaveAmplitudeUp );
	vWave.fY *= c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tRopeWaveAmplitudeSide );
	
	VECTOR* newWaveoffset = pParticleSystem->tWaveOffsets.Add(NULL);	
	*newWaveoffset = vWave;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeUpdatePathPhysics( 
	GAME * pGame, 
	PARTICLE_SYSTEM * pParticleSystem, 
	BOOL bParticleSystemIsNew )
{
	ASSERT( pParticleSystem->pDefinition->nRopePathId != INVALID_ID );
	ROPE_PATH_HASH * pHash = HashGet( sgtRopePathHash, pParticleSystem->pDefinition->nRopePathId );
	ROPE_PATH_FILE * pFile = pHash->pFile;
	if ( ! pFile )
		return;

	if ( pParticleSystem->tKnots.IsEmpty() )
	{
		float fDistanceCurr = pFile->fTotalLength;
		for ( int i = pHash->pFile->nPointCount - 1; i >= 0 ; i-- )
		{
			float fPathTime = fDistanceCurr / pFile->fTotalLength;
			KNOT * pKnot = c_sRopeAddKnot( pGame, pParticleSystem, pFile->pfLengths[ i ], fPathTime, FALSE );
			REF(pKnot);
			fDistanceCurr -= pFile->pfLengths[ i ];
		}
	}

	float fScaleNew = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tRopePathScale );

	int nKnotIndex = 0;
	for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
		nKnotCurr != INVALID_ID; 
		nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr ),
		nKnotIndex++ )
	{
		KNOT * pKnot = pParticleSystem->tKnots.Get( nKnotCurr );
		pKnot->vPosition = pFile->pvPoints[ nKnotIndex ];
		pKnot->vPosition *= fScaleNew;
		pKnot->vPosition += pParticleSystem->vPosition;

		pKnot->fSegmentLength = pFile->pfLengths[ nKnotIndex ] * fScaleNew;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeUpdateWavePhysics(
	GAME * pGame,
	PARTICLE_SYSTEM * pParticleSystem, 
	BOOL bParticleSystemIsNew, 
	float fTimeDelta )
{
	if ( bParticleSystemIsNew )
	{
		for ( int i = 0; i < MAX_WAVE_OFFSETS; i++ )
		{
			c_sCreateWave( pParticleSystem, 0.0f, i );
		}
	}

	// update position if the particle system has moved
	BOOL bSetRopePositions = FALSE;
	int nKnotFirst = pParticleSystem->tKnots.GetFirst(); 
	if ( nKnotFirst != INVALID_ID )
	{
		KNOT * pKnot = pParticleSystem->tKnots.Get( nKnotFirst );
		if ( pKnot->vPosition != pParticleSystem->vPosition )
		{
			bSetRopePositions = TRUE;
		}
	}
	if ( ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_USE_FOLLOW_SYSTEM_ROPE ) != 0 &&
		pParticleSystem->nParticleSystemFollow != INVALID_ID )
	{
		bSetRopePositions = TRUE;
	}
	if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ROPE_END_MOVED )
	{
		bSetRopePositions = TRUE;
		pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_ROPE_END_MOVED;
	}

	if ( bSetRopePositions )
	{
		c_sRopeSetPositions( pGame, pParticleSystem );
	}

	int nStartPrev = (int) pParticleSystem->fWaveStart;
	pParticleSystem->fWaveStart += fTimeDelta * c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tRopeWaveSpeed );
	int nStartCurr = (int) pParticleSystem->fWaveStart;
	if ( nStartPrev != nStartCurr )
	{
		c_sCreateWave( pParticleSystem, pParticleSystem->fPathTime, (nStartCurr + 1) % MAX_WAVE_OFFSETS );
	}

	float fWaveFrequency = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tRopeWaveFrequency );
	if ( fWaveFrequency <= 0.0f )
	{
		fWaveFrequency = 0.1f;
	}

	float fDistanceCurr = 0.0f;
	int nKnotIndex = 0;
	float fLengthCurrent = 0;
	KNOT * pKnotPrev = NULL;
	for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
		nKnotCurr != INVALID_ID; 
		nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr ),
		nKnotIndex++ )
	{
		KNOT * pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );

		int nKnotNext = pParticleSystem->tKnots.GetNextId( nKnotCurr );
		KNOT * pKnotNext = nKnotNext != INVALID_ID ? pParticleSystem->tKnots.Get( nKnotNext ) : NULL;

		BOOL bKnotMoves = pKnotPrev != NULL;
		if ( !pKnotNext && ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ROPE_END_ATTACHED ) != 0 )
			bKnotMoves = FALSE;

		if ( bKnotMoves )
		{
			fLengthCurrent += pKnotCurr->fSegmentLength;
			float fWaveCurrent = pParticleSystem->fWaveStart - fLengthCurrent * fWaveFrequency;

			int nWaveNext = (int) fWaveCurrent;
			int nWavePrev = nWaveNext + 1;
			float fNextPercent = (fWaveCurrent - (float) nWaveNext);
			fNextPercent = 0.5f * cos( PI * fNextPercent ) + 0.5f;
			//ASSERT( fNextPercent >= 0.0f && fNextPercent <= 1.0f );
			if ( fNextPercent > 1.0f )
				fNextPercent = 1.0f;
			if ( fNextPercent < 0.0f )
				fNextPercent = 0.0f;

			while ( nWavePrev < 0 )
			{
				nWavePrev += MAX_WAVE_OFFSETS;
			}
			while ( nWaveNext < 0 )
			{
				nWaveNext += MAX_WAVE_OFFSETS;
			}
			nWavePrev %= MAX_WAVE_OFFSETS;
			nWaveNext %= MAX_WAVE_OFFSETS;

			VECTOR *pWaveOffsetNext = pParticleSystem->tWaveOffsets.Get(nWaveNext);
			VECTOR *pWaveOffsetPrev = pParticleSystem->tWaveOffsets.Get(nWavePrev);
			ASSERT_RETURN(pWaveOffsetPrev && pWaveOffsetNext);
			VECTOR vOffset = VectorLerp( *pWaveOffsetNext, *pWaveOffsetPrev, fNextPercent );

			VECTOR vUp( 0.0f, 0.0f, 1.0f );
			if ( pKnotCurr->vNormal == vUp )
			{
				vUp = VECTOR( 0.0f, 1.0f, 0.0f );
			}
			VECTOR vRight;
			VectorCross( vRight, pKnotCurr->vNormal, vUp );
			VectorCross( vUp, pKnotCurr->vNormal, vRight );
			vUp		*= vOffset.fX * (1.0f - pKnotCurr->fDampening);
			vRight	*= vOffset.fY * (1.0f - pKnotCurr->fDampening);
			pKnotCurr->vPosition = pKnotCurr->vPositionIdeal;
			pKnotCurr->vPosition += vUp;
			pKnotCurr->vPosition += vRight;
		}
		if ( pKnotPrev )
		{
			fDistanceCurr += VectorLength( pKnotCurr->vPosition - pKnotPrev->vPosition );
			pKnotCurr->fDistanceFromStart = fDistanceCurr;
		}

		// if the end moved, then update it
		if ( !pKnotNext && ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ROPE_END_ATTACHED ) == 0 ) 
		{
			c_ParticleSystemSetRopeEnd( pParticleSystem->nId, pKnotCurr->vPosition,
										-pKnotCurr->vNormal, FALSE, FALSE );
		}

		pKnotPrev = pKnotCurr;
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeSetPerLengthValues( 
	GAME * pGame, 
	PARTICLE_SYSTEM * pParticleSystem,
	KNOT * pKnot,
	float fPathTime )
{
	pKnot->fScale	 = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchRopeScale );
	pKnot->dwDiffuse = e_GetColorFromPath( fPathTime, & pParticleSystem->pDefinition->tLaunchRopeColor );
	pKnot->fAlphaInit= c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchRopeAlpha );
	pKnot->dwDiffuse += ((BYTE) (255 * pKnot->fAlphaInit) << 24);

	if ( !pParticleSystem->pDefinition->tLaunchRopeGlow.IsEmpty() )
		pKnot->fGlowInit = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tLaunchRopeGlow );
	else
		pKnot->fGlowInit = 0.0f;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static KNOT * c_sRopeAddKnot( 
	GAME * pGame, 
	PARTICLE_SYSTEM * pParticleSystem,
	float fSegmentLength,
	float fPathTime,
	BOOL bAddEnd )
{
	// create a particle
	int nNewKnotId;
	KNOT * pKnot = pParticleSystem->tKnots.Add( & nNewKnotId, bAddEnd );
	if ( ! pKnot ) // we can only hold so many knots - just bail if we can't get another
		return NULL;

	ZeroMemory( pKnot, sizeof( pKnot ) );

	pKnot->fDistanceFromStart = 0.0f;
	pKnot->fSegmentLength = fSegmentLength;
	c_sRopeSetPerLengthValues( pGame, pParticleSystem, pKnot, fPathTime );
	pKnot->fRandPercent = sRandGetFloat( 1.0f );
	pKnot->vVelocity = VECTOR(0);
	pKnot->fDampening   = c_sGetValueFromPath( fPathTime, pParticleSystem->pDefinition->tRopeDampening );

	return pKnot;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeRemoveKnot( 
	PARTICLE_SYSTEM * pParticleSystem,
	int nKnot )
{
	KNOT * pKnot = pParticleSystem->tKnots.Get( nKnot );
	pKnot->dwFlags |= KNOT_FLAG_REMOVED; // this helps us not use removed knots - if they get readded, then we can still use them though
	pParticleSystem->tKnots.Remove( nKnot );
}

//-------------------------------------------------------------------------------------------------
// this only needs to be called if there might be particles watching the knots
//-------------------------------------------------------------------------------------------------
static void c_sRopeClearKnots( 
	PARTICLE_SYSTEM * pParticleSystem )
{
	if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_FOLLOW_ROPE )
	{
		for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
			nKnotCurr != INVALID_ID; 
			nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr ) )
		{
			KNOT * pKnot = pParticleSystem->tKnots.Get( nKnotCurr );
			pKnot->dwFlags |= KNOT_FLAG_REMOVED;
		}
	}
	pParticleSystem->tKnots.Clear();
}

//-------------------------------------------------------------------------------------------------
// this is a wrapper function to help Hammer do collision checks
//-------------------------------------------------------------------------------------------------
static float sParticleLineCollideLen( 
	ROOMID idRoom,
	const VECTOR & vPosition, 
	const VECTOR & vDirection, 
	float fMaxLen, 
	VECTOR * pvNormal = NULL )
{
	GAME * pGame = AppGetCltGame();

	if (!AppIsHammer())
	{
		ROOM * pRoom = RoomGetByID( pGame, idRoom );
		if ( ! pRoom )
			return fMaxLen;
		LEVEL * pLevel = RoomGetLevel( pRoom );
		return LevelLineCollideLen( pGame, pLevel, vPosition, vDirection, fMaxLen, pvNormal);
	}
	else
	{
		// assume that z = 0 is our only collision in Hammer
		if ( vDirection.fZ * vPosition.fZ >= 0.0f )
			return fMaxLen;
		return min( fMaxLen, -vPosition.fZ / vDirection.fZ );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeHoseResetFirstKnotData(
	GAME* pGame,
	PARTICLE_SYSTEM * pParticleSystem, 
	KNOT * pKnot)
{ 
	VECTOR vSystemNormal = pParticleSystem->vNormalToUse;
	pKnot->vPosition		= pParticleSystem->vPosition;
	pKnot->vPositionIdeal	= pParticleSystem->vPosition;
	pKnot->vNormal			= vSystemNormal;
	float fHosePressure = pParticleSystem->fHosePressure;
	if ( ! VectorIsZero( pParticleSystem->vVelocity ) )
	{
		VECTOR vSystemDirection;
		float fSystemSpeed = VectorLength( pParticleSystem->vVelocity );
		VectorScale( vSystemDirection, pParticleSystem->vVelocity, 1.0f / fSystemSpeed );
		float fSystemContribution = VectorDot( vSystemDirection, vSystemNormal );
		fHosePressure += fSystemContribution * fSystemSpeed;
	}
	VectorScale( pKnot->vVelocity, vSystemNormal, fHosePressure );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeUpdateTrail(
	GAME* pGame,
	PARTICLE_SYSTEM * pParticleSystem, 
	BOOL bParticleSystemIsNew, 
	float fTimeDelta )
{
	// if we have too many knots, remove some
	int nKnotCount = pParticleSystem->tKnots.Count();
	int nKnotMax = min( pParticleSystem->pDefinition->nKnotCountMax, 100 );
	if ( nKnotCount >= nKnotMax )
	{
		int nKnotCurr = pParticleSystem->tKnots.GetFirst();
		for( int i = 0; i < nKnotMax; i++ )
		{
			nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr );
		}

		while ( nKnotCurr != INVALID_ID )
		{
			int nKnotNext = pParticleSystem->tKnots.GetNextId( nKnotCurr );
			pParticleSystem->tKnots.Remove( nKnotCurr );
			nKnotCurr = nKnotNext;
		}
	}

	// remove knots that have timed out
	for( int nKnotCurr = pParticleSystem->tKnots.GetFirst();
		nKnotCurr != INVALID_ID; )
	{
		int nKnotNext = pParticleSystem->tKnots.GetNextId( nKnotCurr );
		KNOT * pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );
		pKnotCurr->fRemainingTime -= fTimeDelta;
		if ( pKnotCurr->fRemainingTime <= 0.0f )
			pParticleSystem->tKnots.Remove( nKnotCurr );
		nKnotCurr = nKnotNext;
	}

	VECTOR vDelta = pParticleSystem->vPosition - pParticleSystem->vTrailPosLast;
	float fSegmentSize = max( pParticleSystem->pDefinition->fSegmentSize, 0.05f );
	float fDistanceSquared = VectorLengthSquared( vDelta );
	KNOT * pKnotFirst = NULL;

	if ( (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DYING) != 0 )
	{ // don't do any adding and don't move the first knot
	}
	else if ( bParticleSystemIsNew || pParticleSystem->tKnots.IsEmpty() ) 
	{ 
		pKnotFirst = c_sRopeAddKnot( pGame, pParticleSystem, pParticleSystem->pDefinition->fSegmentSize, 0.0f, FALSE );
		pParticleSystem->vTrailPosLast = pParticleSystem->vPosition;
		pKnotFirst->fRemainingTime = pParticleSystem->pDefinition->fTrailKnotDuration;
	}
	else if ( fDistanceSquared > fSegmentSize * fSegmentSize )
	{
		float fDistance = sqrt( fDistanceSquared );
		int nKnotsToAdd = (int)(fDistance / fSegmentSize);

		VECTOR vVelocityPrev = pParticleSystem->vNormalToUse;
		VECTOR vNormalPrev   = pParticleSystem->vNormalToUse;
		int nKnotFirst = pParticleSystem->tKnots.GetFirst();
		if ( nKnotFirst != INVALID_ID )
		{
			int nKnotNext = pParticleSystem->tKnots.GetNextId( nKnotFirst );
			int nKnotToUse = ( nKnotNext != INVALID_ID ) ? nKnotNext : nKnotFirst;
			KNOT * pKnotToUse = pParticleSystem->tKnots.Get( nKnotToUse );
			vVelocityPrev = pKnotToUse->vVelocity;
			vNormalPrev   = pKnotToUse->vNormal;
		}

		// add new knots
		VECTOR vDeltaToAlongVelocity = vVelocityPrev;
		vDeltaToAlongVelocity *= fDistance;
		VECTOR vPositionPrev = pParticleSystem->vTrailPosLast;
		for ( int i = -1; i < nKnotsToAdd - 1; i++ )
		{
			KNOT * pKnotNew;
			if ( i == -1 && nKnotFirst != INVALID_ID )
			{
				pKnotNew = pParticleSystem->tKnots.Get( nKnotFirst );
			} else {
				pKnotNew = c_sRopeAddKnot( pGame, pParticleSystem, pParticleSystem->pDefinition->fSegmentSize, 0.0f, FALSE );
			}

			float fLerp = (nKnotsToAdd > 1) ? (float)(i + 2) / nKnotsToAdd : 1.0f;

			VECTOR vDeltaToEnd = pParticleSystem->vPosition - vPositionPrev;

			VECTOR vDeltaNew = VectorLerp( vDeltaToEnd, vDeltaToAlongVelocity, fLerp );
			VectorNormalize( vDeltaNew );

			vVelocityPrev = vDeltaNew;

			vNormalPrev = VectorLerp( pParticleSystem->vNormalToUse, vNormalPrev, fLerp );
			VectorNormalize( vNormalPrev );
			pKnotNew->vNormal = vNormalPrev;

			vDeltaNew *= fSegmentSize;

			pKnotNew->vPosition = vPositionPrev + vDeltaNew;
			vPositionPrev = pKnotNew->vPosition;

			pKnotNew->fRemainingTime = pParticleSystem->pDefinition->fTrailKnotDuration - (1.0f - fLerp) * fTimeDelta;

		}

		nKnotFirst = pParticleSystem->tKnots.GetFirst();
		pKnotFirst = pParticleSystem->tKnots.Get( nKnotFirst );
		if ( pKnotFirst )
			pParticleSystem->vTrailPosLast = pKnotFirst->vPosition;

		//pKnotFirst = c_sRopeAddKnot( pGame, pParticleSystem, pParticleSystem->pDefinition->fSegmentSize, 0.0f, FALSE );
		//pKnotFirst->fRemainingTime = pParticleSystem->pDefinition->fTrailKnotDuration;
		//pKnotFirst->vNormal = vNormalPrev;

	} else { // grab the first knot so that we can move it along with the particle system - no poppiness
		int nKnotFirst = pParticleSystem->tKnots.GetFirst();
		if ( nKnotFirst != INVALID_ID )
			pKnotFirst = pParticleSystem->tKnots.Get( nKnotFirst );
	}

	if ( pKnotFirst )
		pKnotFirst->vPosition = pParticleSystem->vPosition;
	
	float fDistanceCurr = 0.0f;
	// update the segment sizes and other properties that vary along the length of the rope
	for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
		nKnotCurr != INVALID_ID;  )
	{
		KNOT * pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );

		float fKnotPathTime = 1.0f - pKnotCurr->fRemainingTime / pParticleSystem->pDefinition->fTrailKnotDuration;

		pKnotCurr->fDistanceFromStart = fDistanceCurr;
		fDistanceCurr += pKnotCurr->fSegmentLength;

		c_sRopeSetPerLengthValues( pGame, pParticleSystem, pKnotCurr, fKnotPathTime );

		nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr );;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeUpdateHosePhysics(
	GAME* pGame,
	PARTICLE_SYSTEM * pParticleSystem, 
	BOOL bParticleSystemIsNew, 
	float fTimeDelta )
{
	ASSERT_RETURN( pParticleSystem->fHosePressure != 0.0f );

	ASSERT_RETURN( pParticleSystem->pDefinition->fSegmentSize != 0.0f );

	VECTOR vGravity( 0, 0, -9.800723f );
	vGravity *= fTimeDelta;

	int nKnotCount = 0;
	BOOL bCollided = FALSE;
	// move all of the knots forward by the time delta
	for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
		nKnotCurr != INVALID_ID; )
	{
		int nKnotNext = pParticleSystem->tKnots.GetNextId( nKnotCurr );
		nKnotCount++;
		KNOT * pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );

		pKnotCurr->vVelocity += vGravity;

		float fDistanceToMove = VectorLength( pKnotCurr->vVelocity );
		pKnotCurr->vNormal = pKnotCurr->vVelocity;
		pKnotCurr->vNormal /= fDistanceToMove;
		fDistanceToMove *= fTimeDelta;
		VECTOR vCollideNormal;
		float fDistanceToCollision = sParticleLineCollideLen( pParticleSystem->idRoom, pKnotCurr->vPosition, pKnotCurr->vNormal, fDistanceToMove, &vCollideNormal);
		VECTOR vDelta = pKnotCurr->vNormal;
		vDelta *= fDistanceToCollision;
		pKnotCurr->vPosition += vDelta;
		pKnotCurr->vPositionIdeal = pKnotCurr->vPosition;

		// we collided, destroy the rest of the rope
		if ( fDistanceToCollision < fDistanceToMove )
		{
			c_ParticleSystemSetRopeEnd( pParticleSystem->nId, 
				pKnotCurr->vPosition,
				vCollideNormal,
				FALSE );
			bCollided = TRUE;

			for ( ; nKnotCurr != INVALID_ID;  )
			{
				int nKnotNext = pParticleSystem->tKnots.GetNextId( nKnotCurr );
				c_sRopeRemoveKnot( pParticleSystem, nKnotCurr );
				nKnotCurr = nKnotNext;
			}
			break;
		} else {
			if ( nKnotNext == INVALID_ID )
			{
				c_ParticleSystemSetRopeEnd( pParticleSystem->nId, 
											pKnotCurr->vPosition,
											VECTOR( 0, 0, 1 ),
											FALSE );
			}
			nKnotCurr = nKnotNext;
		} 
	}

	if ( pParticleSystem->nParticleSystemRopeEnd != INVALID_ID )
	{
		c_ParticleSystemSetDraw( pParticleSystem->nParticleSystemRopeEnd, bCollided );
	}

	// see if we need to add a new knot to the beginning or just pull back the first knot
	int nKnotFirst = pParticleSystem->tKnots.GetFirst();
	BOOL bAddFirstKnot = FALSE;
	if ( nKnotFirst == INVALID_ID )
	{
		bAddFirstKnot = TRUE;
	} else {
		int nKnotNext = pParticleSystem->tKnots.GetNextId( nKnotFirst );
		if ( nKnotNext == INVALID_ID )
		{
			bAddFirstKnot = TRUE;
		} else {
			KNOT * pKnotNext = pParticleSystem->tKnots.Get( nKnotNext );
			float fDistanceToNextSq = VectorDistanceSquared( pParticleSystem->vPosition, pKnotNext->vPosition );
			float fSegmentSize = pParticleSystem->pDefinition->fSegmentSize;
			if ( fDistanceToNextSq > fSegmentSize * fSegmentSize )
			{
				bAddFirstKnot = TRUE;
			} 
		}
	}

	if ( bParticleSystemIsNew )
		bAddFirstKnot = TRUE;

	if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DYING )
	{
		bAddFirstKnot = FALSE;
	} else if ( nKnotFirst != INVALID_ID ) {
		KNOT * pKnotFirst = pParticleSystem->tKnots.Get( nKnotFirst );
		c_sRopeHoseResetFirstKnotData( pGame, pParticleSystem, pKnotFirst );
	}

	if ( bAddFirstKnot )
	{
		nKnotCount++;
		KNOT * pKnotFirst = c_sRopeAddKnot( pGame, pParticleSystem, pParticleSystem->pDefinition->fSegmentSize, 0.0f, FALSE );
		c_sRopeHoseResetFirstKnotData( pGame, pParticleSystem, pKnotFirst );
	}

	if (nKnotCount > 0)
	{
		float fKnotPathTimePerKnot = 1.0f / (float) nKnotCount;
		float fKnotPathTime = 0.0f;
		float fDistanceCurr = 0.0f;
		// update the segment sizes and other properties that vary along the length of the rope
		for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
			nKnotCurr != INVALID_ID;  )
		{
			KNOT * pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );

			int nKnotNext = pParticleSystem->tKnots.GetNextId( nKnotCurr );
			if ( nKnotNext != INVALID_ID )
			{
				KNOT * pKnotNext = pParticleSystem->tKnots.Get( nKnotNext );
				VECTOR vDelta = pKnotNext->vPosition - pKnotCurr->vPosition;
				pKnotCurr->fSegmentLength = VectorLength( vDelta );
			}
			pKnotCurr->fDistanceFromStart = fDistanceCurr;
			fDistanceCurr += pKnotCurr->fSegmentLength;

			c_sRopeSetPerLengthValues( pGame, pParticleSystem, pKnotCurr, fKnotPathTime );

			fKnotPathTime += fKnotPathTimePerKnot;
			nKnotCurr = nKnotNext;
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeUpdateCircle(
	GAME* pGame,
	PARTICLE_SYSTEM * pParticleSystem, 
	BOOL bParticleSystemIsNew, 
	float fTimeDelta )
{
	ASSERT_RETURN( pParticleSystem->fCircleRadius != 0.0f );
	ASSERT_RETURN( pParticleSystem->pDefinition->fSegmentSize != 0.0f );

	BOOL bHolyRadius = pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_HOLYRADIUS;
	float fCircumference = 2.0f * PI * pParticleSystem->fCircleRadius;
	float fKnotPositions = 1 + fCircumference / pParticleSystem->pDefinition->fSegmentSize;

	{ // make sure that we have the right number of knots
		int nKnotCountDesired = (int)fKnotPositions + 1;
		int nKnotsCountCurrent = pParticleSystem->tKnots.Count();
		float fKnotPercent = 0.0f;
		for ( int i = nKnotsCountCurrent; i < nKnotCountDesired; i++ )
		{
			c_sRopeAddKnot( pGame, pParticleSystem, pParticleSystem->pDefinition->fSegmentSize, fKnotPercent, FALSE );
			fKnotPercent += 1.0f / fKnotPositions;
			nKnotsCountCurrent++;
		}
		int nToDelete = nKnotsCountCurrent - nKnotCountDesired;
		for ( int i = 0; i < nToDelete; i++ )
		{
			int nKnot = pParticleSystem->tKnots.GetFirst();
			if ( nKnot == INVALID_ID )
				break;
			c_sRopeRemoveKnot( pParticleSystem, nKnot );
		}
	}

	float fRadiansPerKnot = 2.0f * PI / fKnotPositions;
	float fDistanceCurr = 0.0f;

	KNOT * pKnotLast = NULL;
	float fKnotIndex = 0.0f;

	VECTOR vSideVector;
	if ( bHolyRadius )
		vSideVector = VECTOR( 0, 1, 0 );
	else
	{
		if ( fabsf( pParticleSystem->vNormalToUse.fZ ) > 0.98f )
		{
			vSideVector = VECTOR( 1, 0, 0 );
		} else {
			VectorCross( vSideVector, VECTOR( 0, 0, 1 ), pParticleSystem->vNormalToUse );
			VectorNormalize( vSideVector );
		}
	}

	float fZOffset = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tRopeZOffsetOverTime );

	for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
		  nKnotCurr != INVALID_ID;
		  nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr ), fKnotIndex = fKnotIndex + 1.0f )
	{
		KNOT * pKnot = pParticleSystem->tKnots.Get( nKnotCurr );
		MATRIX mRotation;
		float fRadians = fRadiansPerKnot * fKnotIndex;
		if ( bHolyRadius )
			MatrixTransform( &mRotation, NULL, fRadians );
		else
			MatrixRotationAxis( mRotation, pParticleSystem->vNormalToUse, fRadians );

		VECTOR vDirection;
		MatrixMultiplyNormal( &vDirection, &vSideVector, &mRotation );
		VectorNormalize( vDirection );

		VECTOR vPosition = pParticleSystem->vPosition;
		if ( bHolyRadius )
			vPosition.fZ += 1.0f;

		//float fDistFromCenter = sParticleLineCollideLen( pParticleSystem->idRoom, vPosition, vDirection, 
		//												 pParticleSystem->fCircleRadius );
		float fDistFromCenter = pParticleSystem->fCircleRadius;

		VECTOR vDelta = vDirection;
		vDelta *= fDistFromCenter;

		vPosition += vDelta;

		if ( bHolyRadius )
		{
			float fDistDownMax = 4.0f;
			float fDistDown = sParticleLineCollideLen( pParticleSystem->idRoom, vPosition, VECTOR( 0, 0, -1.0f ), fDistDownMax, &pKnot->vNormal );

			if ( fDistDown == fDistDownMax )
			{
				pKnot->vNormal = VECTOR( 0, 0, 1 );
			}
			pKnot->vPositionIdeal = vPosition;
			pKnot->vPositionIdeal.fZ -= fDistDown;
		} else {
			pKnot->vPositionIdeal = vPosition;
		}

		pKnot->vPositionIdeal.fZ += fZOffset;

		if ( bParticleSystemIsNew || ! bHolyRadius )
			pKnot->vPosition = pKnot->vPositionIdeal;
		else
			pKnot->vPosition = VectorLerp( pKnot->vPosition, pKnot->vPositionIdeal, 0.85f );

		if ( pKnotLast )
		{
			float fDist = VectorLength( pKnot->vPosition - pKnotLast->vPosition );
			fDistanceCurr += fDist;
			pKnot->fDistanceFromStart = fDistanceCurr;
		}
		pKnotLast = pKnot;
	}

	// connect the ends
	if ( pKnotLast )
	{
		int nFirstKnot = pParticleSystem->tKnots.GetFirst();
		KNOT * pFirst = pParticleSystem->tKnots.Get( nFirstKnot );
		pFirst->vPositionIdeal = pKnotLast->vPositionIdeal;
		pFirst->vPosition = pKnotLast->vPosition;
		pFirst->vNormal   = pKnotLast->vNormal;
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeUpdateContour(
	GAME* pGame,
	PARTICLE_SYSTEM * pParticleSystem, 
	BOOL bParticleSystemIsNew, 
	float fTimeDelta )
{
	VECTOR vUpVector(0,0,1);
	VECTOR vSideVector;
	VectorCross( vSideVector, pParticleSystem->vNormalToUse, vUpVector );
	VectorNormalize( vSideVector );

	float fUpOffset = 12.0f;
	VECTOR vPositionUp = pParticleSystem->vPosition;
	vPositionUp.fZ += fUpOffset;

	float fDistSideMax = 30.0f;
	float fDistSide    = sParticleLineCollideLen( pParticleSystem->idRoom, vPositionUp, vSideVector, fDistSideMax, NULL );
	float fDistSideNeg = sParticleLineCollideLen( pParticleSystem->idRoom, vPositionUp, -vSideVector, fDistSideMax, NULL );

	float fKnotPositions = 1 + (fDistSide + fDistSideNeg) / pParticleSystem->pDefinition->fSegmentSize;

	{ // make sure that we have the right number of knots
		int nKnotCountDesired = (int)fKnotPositions + 1;
		int nKnotsCountCurrent = pParticleSystem->tKnots.Count();
		float fKnotPercent = 0.0f;
		for ( int i = nKnotsCountCurrent; i < nKnotCountDesired; i++ )
		{
			c_sRopeAddKnot( pGame, pParticleSystem, pParticleSystem->pDefinition->fSegmentSize, fKnotPercent, FALSE );
			fKnotPercent += 1.0f / fKnotPositions;
			nKnotsCountCurrent++;
		}
		int nToDelete = nKnotsCountCurrent - nKnotCountDesired;
		for ( int i = 0; i < nToDelete; i++ )
		{
			int nKnot = pParticleSystem->tKnots.GetFirst();
			if ( nKnot == INVALID_ID )
				break;
			c_sRopeRemoveKnot( pParticleSystem, nKnot );
		}
	}

	vPositionUp = vPositionUp + vSideVector * fDistSide;

	float fDistanceCurr = 0.0f;

	KNOT * pKnotLast = NULL;

	for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
		nKnotCurr != INVALID_ID;
		nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr ) )
	{
		KNOT * pKnot = pParticleSystem->tKnots.Get( nKnotCurr );

		float fDistDownMax = fUpOffset + 12.0f;
		float fDistDown = sParticleLineCollideLen( pParticleSystem->idRoom, vPositionUp, VECTOR( 0, 0, -1.0f ), fDistDownMax, &pKnot->vNormal );

		if ( fDistDown == fDistDownMax )
		{
			pKnot->vNormal = VECTOR( 0, 0, 1 );
		}
		pKnot->vPositionIdeal = vPositionUp;
		pKnot->vPositionIdeal.fZ -= fDistDown;

		if ( bParticleSystemIsNew )
			pKnot->vPosition = pKnot->vPositionIdeal;
		else
			pKnot->vPosition = VectorLerp( pKnot->vPosition, pKnot->vPositionIdeal, 0.85f );

		if ( pKnotLast )
		{
			float fDist = VectorLength( pKnot->vPosition - pKnotLast->vPosition );
			fDistanceCurr += fDist;
			pKnot->fDistanceFromStart = fDistanceCurr;
		}

		vPositionUp += vSideVector * -pParticleSystem->pDefinition->fSegmentSize;

		pKnotLast = pKnot;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeUpdateNova(
	GAME* pGame,
	PARTICLE_SYSTEM * pParticleSystem, 
	BOOL bParticleSystemIsNew, 
	float fTimeDelta )
{
	if ( bParticleSystemIsNew )
	{
		int nKnotCount = pParticleSystem->pDefinition->nKnotCountMax;
		if ( pParticleSystem->fNovaAngle != 0.0f )
		{
			nKnotCount = 1 + (int)( (pParticleSystem->fNovaAngle * nKnotCount) / TWOxPI );
		}
		float fAnglePerKnot = TWOxPI / (pParticleSystem->pDefinition->nKnotCountMax - 1);

		float fPathTime = pParticleSystem->fNovaStart / TWOxPI;
		float fTimePerKnot = fAnglePerKnot / TWOxPI;
		pParticleSystem->tKnots.Clear();
		for ( int i = 0; i < nKnotCount; i++ )
		{
			KNOT * pKnot = c_sRopeAddKnot( pGame, pParticleSystem, 0.1f, fPathTime, FALSE );

			pKnot->vPosition = pParticleSystem->vPosition;

			MATRIX mRotation;
			MatrixTransform( &mRotation, NULL, pParticleSystem->fNovaStart + fAnglePerKnot * i );
			MatrixMultiplyNormal( &pKnot->vVelocity, &cgvYAxis, &mRotation );
			pKnot->vNormal = pKnot->vVelocity;

			fPathTime += fTimePerKnot;
		}
	}

	float fSpeed = VectorLength( pParticleSystem->vVelocity );
	fSpeed *= fTimeDelta;

	KNOT * pKnotPrev = NULL;
	float fDistanceCurr = 0.0f;
	for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
		nKnotCurr != INVALID_ID;
		nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr ) )
	{
		KNOT * pKnot = pParticleSystem->tKnots.Get( nKnotCurr );

		VECTOR vDelta = pKnot->vVelocity;
		vDelta *= fSpeed;
		pKnot->vPosition += vDelta;

		if ( pKnotPrev )
		{
			pKnot->fSegmentLength = VectorLength( pKnotPrev->vPosition - pKnot->vPosition );
			fDistanceCurr += pKnot->fSegmentLength;
			pKnot->fDistanceFromStart = fDistanceCurr ;
		}
		pKnotPrev = pKnot;
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeCalcKnotCountAndSegmentLength(
	GAME * pGame,
	PARTICLE_SYSTEM * pParticleSystem,
	float & fSegmentLength,
	int & nKnotCount )
{
	nKnotCount = pParticleSystem->pDefinition->nKnotCount;
	fSegmentLength = pParticleSystem->pDefinition->fSegmentSize;
	if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ROPE_END_ATTACHED )
	{
		VECTOR vDeltaToRopeEnd = pParticleSystem->vRopeEndIdeal - pParticleSystem->vPosition;
		float fDistanceToRopeEnd = VectorLength( vDeltaToRopeEnd );

		if ( fSegmentLength > 0.0f ) 
		{
			if ( fDistanceToRopeEnd != 0.0f )
			{
				nKnotCount = (int) ( fDistanceToRopeEnd / fSegmentLength ) + 1;
				if ( pParticleSystem->pDefinition->nKnotCountMax != 0 )
				{
					nKnotCount = max( nKnotCount, pParticleSystem->pDefinition->nKnotCount );
					nKnotCount = min( nKnotCount, pParticleSystem->pDefinition->nKnotCountMax );
				}
			} else {
				nKnotCount = 2;
			}
		} 
	} 
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeAdjustKnotCountAndLength(
	GAME * pGame,
	PARTICLE_SYSTEM * pParticleSystem,
	PARTICLE_SYSTEM * pParticleSystemFollow )
{
	float fSegmentLength;
	int nKnotCountTarget;
	if ( pParticleSystemFollow )
	{
		fSegmentLength = pParticleSystemFollow->pDefinition->fSegmentSize;
		nKnotCountTarget = pParticleSystemFollow->tKnots.Count();
	} else {
		c_sRopeCalcKnotCountAndSegmentLength( pGame, pParticleSystem, fSegmentLength, nKnotCountTarget );
	}

	if ( nKnotCountTarget == 0 )
	{
		c_sRopeClearKnots( pParticleSystem );
		return;
	}
	int nKnotCount = 0;
	int nKnotNext = INVALID_ID;
	for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
		nKnotCurr != INVALID_ID; )
	{
		nKnotNext = pParticleSystem->tKnots.GetNextId( nKnotCurr );

		if ( nKnotCount >= nKnotCountTarget )
		{
			c_sRopeRemoveKnot( pParticleSystem, nKnotCurr );
		} else
		{
			KNOT * pKnot = pParticleSystem->tKnots.Get( nKnotCurr );
			pKnot->fSegmentLength = fSegmentLength;
		}

		nKnotCurr = nKnotNext;
		nKnotCount++;
	}
	for ( int i = nKnotCount; i < nKnotCountTarget; i++ )
	{
		float fPathTime  = (float) (i) / (float)(nKnotCount - 1);
		c_sRopeAddKnot( pGame, pParticleSystem, fSegmentLength, fPathTime, FALSE );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeCreateKnots(
	GAME * pGame,
	PARTICLE_SYSTEM * pParticleSystem )
{
	float fSegmentLength;
	int nKnotCount;
	c_sRopeCalcKnotCountAndSegmentLength( pGame, pParticleSystem, fSegmentLength, nKnotCount );

	// since the knot array prepends entries, we add from the rope end to beginning
	for ( int i = nKnotCount - 1; i >= 0; i-- )
	{
		float fPathTime  = (float) (i) / (float)(nKnotCount - 1);
		c_sRopeAddKnot( pGame, pParticleSystem, fSegmentLength, fPathTime, FALSE );
	}

	c_sRopeSetPositions( pGame, pParticleSystem );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRopeUpdate(
	GAME* pGame,
	PARTICLE_SYSTEM* pParticleSystem, 
	BOOL bParticleSystemIsNew, 
	float fTimeDelta )
{
	PARTICLE_PERF(PART_ROPES)
	PARTICLE_HITCOUNT(PART_ROPES)
	pParticleSystem->pDefinition->nRuntimeRopeUpdatedCount++;
	// the hose makes its own knots
	if ( bParticleSystemIsNew && 
		( pParticleSystem->pDefinition->dwFlags  & PARTICLE_SYSTEM_DEFINITION_FLAG_ROPE_USES_HOSE_PHYSICS ) == 0 &&
		( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_HOLYRADIUS  ) == 0 &&
		( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_TRAIL_PHYSICS  ) == 0 &&
		pParticleSystem->pDefinition->nRopePathId == INVALID_ID )
	{
		c_sRopeCreateKnots( pGame, pParticleSystem );
	}
	if ( pParticleSystem->vRopeEndIdeal != pParticleSystem->vRopeEnd )
	{
		pParticleSystem->vRopeEnd = VectorLerp( pParticleSystem->vRopeEndIdeal, pParticleSystem->vRopeEnd, pParticleSystem->pDefinition->fRopeEndSpeed );
	}

	if ( pParticleSystem->pDefinition->nRopePathId != INVALID_ID )
	{
		c_sRopeUpdatePathPhysics( pGame, pParticleSystem, bParticleSystemIsNew );
	}
	else if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ROPE_USES_WAVE_PHYSICS )
	{
		c_sRopeUpdateWavePhysics( pGame, pParticleSystem, bParticleSystemIsNew, fTimeDelta );
	} 
	else if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ROPE_USES_HOSE_PHYSICS )
	{
		c_sRopeUpdateHosePhysics(pGame, pParticleSystem, bParticleSystemIsNew, fTimeDelta );
	} 
	else if ( pParticleSystem->pDefinition->dwFlags2 & 
		(PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_HOLYRADIUS | PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_CIRCLE_PHYSICS) )
	{
		c_sRopeUpdateCircle(pGame, pParticleSystem, bParticleSystemIsNew, fTimeDelta );
	} 
	else if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_NOVA_PHYSICS )
	{
		c_sRopeUpdateNova(pGame, pParticleSystem, bParticleSystemIsNew, fTimeDelta );
	} 
	else if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_TRAIL_PHYSICS )
	{
		c_sRopeUpdateTrail(pGame, pParticleSystem, bParticleSystemIsNew, fTimeDelta );
	} 
	else if ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_ROPE_USES_CONTOUR_PHYSICS )
	{
		c_sRopeUpdateContour(pGame, pParticleSystem, bParticleSystemIsNew, fTimeDelta );
	} 
	else {
		BOOL bRopeEndMoved = (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ROPE_END_MOVED) != 0;
		pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_ROPE_END_MOVED;

		if ( (pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_USE_FOLLOW_SYSTEM_ROPE) != 0 ||
			bRopeEndMoved )
		{
			c_sRopeSetPositions( pGame, pParticleSystem );
		}
	}

	pParticleSystem->fTextureSlideX += fTimeDelta * c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tAnimationSlidingRateX );
	pParticleSystem->fTextureSlideY += fTimeDelta * c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tAnimationSlidingRateY );
	pParticleSystem->fTextureSlideX = pParticleSystem->fTextureSlideX - FLOOR( pParticleSystem->fTextureSlideX );
	pParticleSystem->fTextureSlideY = pParticleSystem->fTextureSlideY - FLOOR( pParticleSystem->fTextureSlideY );
	if ( ! pParticleSystem->pDefinition->tRopeMetersPerTexture.IsEmpty() )
	{
		pParticleSystem->fRopeMetersPerTexture = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tRopeMetersPerTexture );
	} else {
		pParticleSystem->fRopeMetersPerTexture = 0.0f;
	}

	for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
		nKnotCurr != INVALID_ID; 
		nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr ) )
	{
		KNOT * pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );

		if ( ! pParticleSystem->pDefinition->tRopeAlpha.IsEmpty() )
		{
			float fAlphaChange = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tRopeAlpha );
			float fAlpha = pKnotCurr->fAlphaInit * fAlphaChange;
			pKnotCurr->dwDiffuse &= 0x00ffffff;
			pKnotCurr->dwDiffuse += ((BYTE) (255 * fAlpha) << 24);
		}

		if ( ! pParticleSystem->pDefinition->tRopeGlow.IsEmpty() )
		{
			float fGlowChange = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tRopeGlow );
			float fGlow = pKnotCurr->fGlowInit * fGlowChange;
			if ( fGlow > 1.0f )
				fGlow = 1.0f;
			pKnotCurr->dwGlow = ((BYTE) (255 * fGlow) << 24);
		} else if ( pKnotCurr->fGlowInit != 0.0f )
		{
			pKnotCurr->dwGlow = ((BYTE) (255 * pKnotCurr->fGlowInit) << 24);
		} else {
			pKnotCurr->dwGlow = 0xff000000;
		}
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sSpawnParticlesByTime(
	PARTICLE_SYSTEM * pParticleSystem, 
	BOOL bParticleSystemIsNew, 
	float fTimeDelta )
{
	if ( pParticleSystem->pDefinition->tParticlesPerSecondPath.IsEmpty() )
		return;

	float fParticlesPerSecond = c_sGetSpawnRateValueFromPath ( pParticleSystem, pParticleSystem->fPathTime, pParticleSystem->pDefinition->tParticlesPerSecondPath );	
	if ( fParticlesPerSecond == 0.0f )
		return;
	
	fParticlesPerSecond = MAX( 1.0f, fParticlesPerSecond * sParticlesCanReleaseDelta( pParticleSystem ) );



	float fTimeBetweenParticles = 1.0f / fParticlesPerSecond;

	if ( pParticleSystem->fTimeLastParticleCreated == 0.0f )
		pParticleSystem->fTimeLastParticleCreated = pParticleSystem->fElapsedTime;
	else if ( pParticleSystem->fTimeLastParticleCreated < pParticleSystem->fElapsedTime )
	{		
		while( pParticleSystem->fTimeLastParticleCreated + fTimeBetweenParticles < pParticleSystem->fElapsedTime )
		{
			PARTICLE * pParticle = c_sLaunchParticle( pParticleSystem, pParticleSystem->fPathTime );
			if ( pParticle && ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_FOLLOW_ROPE ) != 0 )
				pParticle->nFollowKnot = pParticleSystem->tKnots.GetFirst();
			pParticleSystem->fTimeLastParticleCreated += fTimeBetweenParticles;
		}
	}

}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sSpawnParticlesAlongRope(
	PARTICLE_SYSTEM * pParticleSystem, 
	BOOL bParticleSystemIsNew, 
	float fTimeDelta )
{
	if ( ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_HAS_ROPE ) == 0 )
		return;

	if ( pParticleSystem->pDefinition->tParticlesPerMeterPerSecond.IsEmpty() )
		return;

	float fParticlesPerMeterPerSecond = c_sGetSpawnRateValueFromPath ( pParticleSystem, pParticleSystem->fPathTime, pParticleSystem->pDefinition->tParticlesPerMeterPerSecond );	
	if ( fParticlesPerMeterPerSecond == 0.0f )
		return;

	fParticlesPerMeterPerSecond = MAX( 1.0f, fParticlesPerMeterPerSecond * sParticlesCanReleaseDelta( pParticleSystem ) );


	// launch along the rope
	float fRopeMeters = sParticleSystemGetRopeLength( pParticleSystem );

	float fTimeBetweenParticles = 1.0f / ( fParticlesPerMeterPerSecond * fRopeMeters );
	if ( pParticleSystem->fTimeLastParticleCreated == 0.0f )
		pParticleSystem->fTimeLastParticleCreated = -fTimeBetweenParticles;
	
	
	while( pParticleSystem->fTimeLastParticleCreated + fTimeBetweenParticles < pParticleSystem->fElapsedTime )
	{
		VECTOR vPosition( 0 );
		VECTOR vNormal( 1, 0, 0 );
		float fScale = 0.0f;
		float fDistanceToSpawnAt = RandGetFloat(sgParticlesRand) * fRopeMeters;

		float fDistanceCurr = 0.0f;
		KNOT * pKnotPrev = NULL;
		KNOT * pKnotCurr = NULL;
		int nKnotCurr = pParticleSystem->tKnots.GetFirst();
		for ( ; nKnotCurr != INVALID_ID; 
			nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr ) )
		{
			pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );
			fDistanceCurr += pKnotCurr->fSegmentLength;
			if ( fDistanceCurr > fDistanceToSpawnAt )
				break;
			pKnotPrev = pKnotCurr;
		}

		if ( pKnotPrev )
		{
			float fPercentBetween = 0.0f;
			if ( pKnotCurr->fSegmentLength != 0.0f )
				fPercentBetween = (fDistanceToSpawnAt - (fDistanceCurr - pKnotCurr->fSegmentLength)) / pKnotCurr->fSegmentLength;
			fPercentBetween = PIN( fPercentBetween, 0.0f, 1.0f );
			vPosition = VectorLerp( pKnotCurr->vPosition, pKnotPrev->vPosition, fPercentBetween );

			vNormal = VectorLerp( pKnotCurr->vNormal, pKnotPrev->vNormal, fPercentBetween );

			VectorNormalize( vNormal );

			if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_USE_ROPE_SCALE )
			{
				fScale = fPercentBetween * pKnotCurr->fScale + (1.0f - fPercentBetween) * pKnotPrev->fScale;
			}
		} 
		else 
		{
			vPosition = pKnotCurr->vPosition;
			vNormal = pKnotCurr->vNormal;
			if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_USE_ROPE_SCALE )
			{
				fScale = pKnotCurr->fScale;
			}
		}

		if ( VectorIsZero( vNormal ) )
		{
			vNormal.fZ = 1.0f;
		} else {
			VectorNormalize( vNormal );
		}
 
		PARTICLE * pParticle = c_sLaunchParticle( pParticleSystem, pParticleSystem->fPathTime, &vPosition, &vNormal, fScale );
		if ( pParticle )
		{
			pParticle->fRopeDistance = fDistanceToSpawnAt;
			pParticle->nFollowKnot = nKnotCurr;
		}
		
		pParticleSystem->fTimeLastParticleCreated += fTimeBetweenParticles;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sSpawnParticlesInBursts(
	PARTICLE_SYSTEM * pParticleSystem, 
	BOOL bParticleSystemIsNew, 
	float fTimeDelta )
{
	if ( pParticleSystem->pDefinition->tParticleBurst.IsEmpty() )
		return;

	int nPointCount = pParticleSystem->pDefinition->tParticleBurst.GetPointCount();
	for ( int nPathPoint = 0; nPathPoint < nPointCount; nPathPoint++ )
	{
		const CInterpolationPath<CFloatPair>::CInterpolationPoint * pPathPoint 
			= pParticleSystem->pDefinition->tParticleBurst.GetPoint( nPathPoint );
		if ( pParticleSystem->fPathTime >= pPathPoint->m_fTime && pParticleSystem->fLastParticleBurstTime < pPathPoint->m_fTime )
		{
			pParticleSystem->fLastParticleBurstTime = pPathPoint->m_fTime;
			float fDiff = pPathPoint->m_tData.y - pPathPoint->m_tData.x;
			float fNewParticles = 0.0f;
			if ( fDiff )
			{
				if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_USES_PARTICLE_SPAWN_THROTTLE )
				{
					fNewParticles = pPathPoint->m_tData.x + pParticleSystem->fParticleSpawnThrottle * fDiff;
				} else {
					fNewParticles = pPathPoint->m_tData.x + sRandGetFloat( fDiff );
				}
			}
			else
				fNewParticles = pPathPoint->m_tData.x;
			int nNewCount = (int) FLOOR( fNewParticles * sParticlesCanReleaseDelta( pParticleSystem ) );

			
			for ( int nNew = 0; nNew < nNewCount; nNew++ )
			{
				c_sLaunchParticle( pParticleSystem, pParticleSystem->fPathTime );
			}
			break;
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sSpawnParticlesByDistance(
	PARTICLE_SYSTEM * pParticleSystem, 
	BOOL bParticleSystemIsNew, 
	float fTimeDelta )
{
	if ( pParticleSystem->pDefinition->tParticlesPerMeter.IsEmpty() )
		return;

	float fParticlesPerMeter = c_sGetSpawnRateValueFromPath ( pParticleSystem, pParticleSystem->fPathTime, pParticleSystem->pDefinition->tParticlesPerMeter );
	fParticlesPerMeter *= sParticlesCanReleaseDelta( pParticleSystem );

	if ( fParticlesPerMeter == 0.0f || 
		VectorIsZero( pParticleSystem->vPositionLastParticleSpawned ) ||
		(pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_WAS_NODRAW) != 0 )
	{
		pParticleSystem->vPositionLastParticleSpawned = pParticleSystem->vPosition;
		return;
	}
	if ( bParticleSystemIsNew )
	{
		return;
	}
	VECTOR vDelta = pParticleSystem->vPosition - pParticleSystem->vPositionLastParticleSpawned;
	float fDistanceToPrevSquared = VectorLengthSquared( vDelta );
	BOOL bSkipAddingParticles = FALSE;
	if ( fDistanceToPrevSquared > 50.0f && (pParticleSystem->fElapsedTime < 0.5f || (pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_START_AT_RANDOM_PERCENT) != 0) &&
		VectorLengthSquared( pParticleSystem->vPositionLastParticleSpawned ) < 10.0f )
	{ // just in case it was placed close to the origin and then warped into place
		bSkipAddingParticles = TRUE;
	}
	if ( fDistanceToPrevSquared > 100.0f ) // we just don't want to stretch it too far
		bSkipAddingParticles = TRUE;
	if ( bSkipAddingParticles )
	{
		pParticleSystem->vPositionLastParticleSpawned = pParticleSystem->vPosition;
		return;
	}

	float fMetersPerParticle = 1.0f / fParticlesPerMeter;
	if ( fDistanceToPrevSquared <= fMetersPerParticle * fMetersPerParticle )
		return;

	float fDistanceToPrev = sqrtf( fDistanceToPrevSquared );
	VECTOR vDirection = vDelta;
	ASSERT_RETURN( fDistanceToPrev != 0.0f );
	vDirection /= fDistanceToPrev;

	VECTOR vDeltaBetweenParticles = vDirection;
	vDeltaBetweenParticles *= fMetersPerParticle;
	VECTOR vPositionCurr = pParticleSystem->vPositionLastParticleSpawned + vDeltaBetweenParticles;
	VECTOR vSystemNormal = pParticleSystem->vNormalToUse;
	for ( float fDistance = 0.0f; fDistance < fDistanceToPrev; fDistance += fMetersPerParticle )
	{
		// yes, we could do something fancy like interpolate the normal, but I don't think anyone will notice
		c_sLaunchParticle( pParticleSystem, pParticleSystem->fPathTime, &vPositionCurr, &vSystemNormal );
		if ( fDistance + fMetersPerParticle > fDistanceToPrev )
		{
			pParticleSystem->vPositionLastParticleSpawned = vPositionCurr;
		}
		vPositionCurr += vDeltaBetweenParticles;
	}

}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
FP_HOLODECK_GETMODELID gfpHolodeckGetModelId = NULL;

void c_SetHolodeckGetModelIdFunction(
	FP_HOLODECK_GETMODELID fpHolodeckGetModelId)
{
	gfpHolodeckGetModelId = fpHolodeckGetModelId;
}



//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// I want this to be "inline" even if we are doing a debug build - it needs to be very fast
#define NEEDS_UPDATE(flag) (bJustCreated || (pParticleSystem->pDefinition->dwUpdateFlags & flag) != 0)
//#define NEEDS_UPDATE(flag) (TRUE)
static int c_sParticleSystemUpdate(
	PARTICLE_SYSTEM * const NOALIAS pParticleSystem, 
	BOOL bParticleSystemIsNew, 
	const VECTOR & vSystemPosDelta,
	float fTimeDelta )
{
	ASSERT_RETZERO( pParticleSystem->pDefinition );
	
	if ( (pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_SYSTEM_NORMAL_WITH_CAMERA) != 0 &&
		 (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ASYNC) == 0 )
	{
		GAME * pGame = AppGetCltGame();
		VECTOR vEyeLocation = c_GetCameraLocation( pGame );
		// can result in zero-len normal
		pParticleSystem->vNormalToUse = pParticleSystem->vPosition - vEyeLocation;
		if ( pParticleSystem->vNormalToUse == VECTOR( 0 ) )
			pParticleSystem->vNormalToUse = VECTOR( 1, 0, 0 );
		else
			VectorNormalize( pParticleSystem->vNormalToUse );
	}

	if ( bParticleSystemIsNew && ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_USE_LIGHTMAP ) != 0 &&
		(pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ASYNC) == 0)
	{
		PARTICLE_PERF(PART_LGHTMAP)
		GAME * pGame = AppGetCltGame();
		if (!AppIsHammer())
		{
			ROOM * pRoom = RoomGetByID( pGame, pParticleSystem->idRoom );
			if (!pRoom)
			{
				return 0;
			}
			int nRoomModel = RoomGetRootModel( pRoom );
			pParticleSystem->dwSystemColor = e_ModelGetLightmapColor( nRoomModel, pParticleSystem->vPosition, pParticleSystem->vNormalToUse );
		}
		else
		{
			int nRoomModel = INVALID_ID;
			if (gfpHolodeckGetModelId)
			{
				nRoomModel = gfpHolodeckGetModelId();
			}
			if ( nRoomModel != INVALID_ID )
				pParticleSystem->dwSystemColor = e_ModelGetLightmapColor( nRoomModel, pParticleSystem->vPosition, pParticleSystem->vNormalToUse );
			else
				pParticleSystem->dwSystemColor = ARGB_MAKE_FROM_INT( 255, 255, 255, 255 );
		}
	}

	if ( ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_USE_LIGHTMAP ) != 0 )
	{
		if ( pParticleSystem->fGlobalAlpha != 1.0f )
		{
			DWORD dwAlphaBits = (DWORD) (255 * pParticleSystem->fGlobalAlpha);
			pParticleSystem->dwSystemColor &= 0x00ffffff;
			pParticleSystem->dwSystemColor |= dwAlphaBits << 24;
		} else {
			pParticleSystem->dwSystemColor |= 0xff000000;
		}
	} 

	if ( bParticleSystemIsNew && pParticleSystem->pDefinition->nLaunchParticleCount > 0 &&
		(pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DYING) == 0 )
	{
		PARTICLE_PERF(PART_LAUNCH)
			
		int nLauncherParticleCount = pParticleSystem->pDefinition->nLaunchParticleCount;
		nLauncherParticleCount = (int)max( 1, ((float)pParticleSystem->pDefinition->nLaunchParticleCount * sParticlesCanReleaseDelta( pParticleSystem )) );

		for ( int i = 0; i < nLauncherParticleCount; i++ )
		{
			c_sLaunchParticle( pParticleSystem, pParticleSystem->fPathTime );
		}
		pParticleSystem->vPositionLastParticleSpawned = pParticleSystem->vPosition;
	}

	// create more particles
	if ( (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DYING) == 0 )
	{
		PARTICLE_PERF(PART_SPAWN)
		c_sSpawnParticlesByTime( pParticleSystem, bParticleSystemIsNew, fTimeDelta );

		c_sSpawnParticlesAlongRope( pParticleSystem, bParticleSystemIsNew, fTimeDelta );

		c_sSpawnParticlesInBursts( pParticleSystem, bParticleSystemIsNew, fTimeDelta );

		c_sSpawnParticlesByDistance( pParticleSystem, bParticleSystemIsNew, fTimeDelta );

	}


	BOOL bUsingHavokFX = sParticleSystemUsesHavokFX( pParticleSystem );

	// update attractor
	BOOL bAttractorDestroys;
	float fAttractorDestructionRadiusMin;
	float fAttractorDestructionRadiusMax;
	float fAttractorForceRadius;
	{
		PARTICLE_PERF(PART_ATTRACT)
		VECTOR vAttractorOffset;
		vAttractorOffset.fX = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tAttractorOffsetSideX );
		vAttractorOffset.fY = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tAttractorOffsetNormal );
		vAttractorOffset.fZ = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tAttractorOffsetSideY );
		VECTOR vAttractorCenter;
		if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_ATTRACTOR_AT_ROPE_END ) != 0 &&
			 ( pParticleSystem->pDefinition->dwFlags  & PARTICLE_SYSTEM_DEFINITION_FLAG_HAS_ROPE ) )
			vAttractorCenter = pParticleSystem->vRopeEnd;
		else
			vAttractorCenter = pParticleSystem->vPosition;
		if ( ! VectorIsZero( vAttractorOffset ) )
		{
			MATRIX mRotateToNormal;
			MatrixFromVectors ( mRotateToNormal, vAttractorCenter, VECTOR ( 0, 0, 1 ), pParticleSystem->vNormalToUse );
			MatrixMultiply( &pParticleSystem->vAttractor, &vAttractorOffset, &mRotateToNormal );
		} else {
			pParticleSystem->vAttractor = vAttractorCenter;
		}
		pParticleSystem->vAttractor.fZ += c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tAttractorWorldOffsetZ );

		bAttractorDestroys = ! pParticleSystem->pDefinition->tAttractorDestructionRadius.IsEmpty();
		fAttractorDestructionRadiusMin = 0.0f;
		fAttractorDestructionRadiusMax = 0.0f;
		if ( bAttractorDestroys )
		{
			CFloatPair tPair = pParticleSystem->pDefinition->tAttractorDestructionRadius.GetValue( pParticleSystem->fPathTime );
			
			VECTOR4 vAttractorDestructionRadius;
			vAttractorDestructionRadius.fX = fAttractorDestructionRadiusMin = tPair.x;
			vAttractorDestructionRadius.fY = fAttractorDestructionRadiusMax = tPair.y;

			if ( bUsingHavokFX )
			{
				e_HavokFXSetSystemParamVector( pParticleSystem->nId, HAVOKFX_PHYSICS_SYSTEM_PARAM_ATTRACTOR_DESTRUCTION_RADIUS_MIN_MAX, vAttractorDestructionRadius );
				//e_HavokFXSetSystemParam( HAVOKFX_PHYSICS_SYSTEM_PARAM_ATTRACTOR_DESTRUCTION_RADIUS_MAX, fAttractorDestructionRadiusMax );
			}
		}
		fAttractorForceRadius = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tAttractorForceRadius );

		if ( bUsingHavokFX )
		{
			ASSERTONCE( pParticleSystem->pDefinition->tAttractorForceOverRadius.GetPointCount() <= 1 );
			float fAttractorForce = c_sGetValueFromPath( 0.0f, pParticleSystem->pDefinition->tAttractorForceOverRadius );

			VECTOR4 vAttractorPosAndForce;
			vAttractorPosAndForce.fX = pParticleSystem->vAttractor.fX;
			vAttractorPosAndForce.fY = pParticleSystem->vAttractor.fY;
			vAttractorPosAndForce.fZ = pParticleSystem->vAttractor.fZ;
			vAttractorPosAndForce.fW = fAttractorForce;
			e_HavokFXSetSystemParamVector( pParticleSystem->nId, HAVOKFX_PHYSICS_SYSTEM_PARAM_ATTRACTOR_POSITION_AND_FORCE, vAttractorPosAndForce );

			if ( ! pParticleSystem->pDefinition->tParticleWindInfluence.IsEmpty() )
			{
				float fWindInfluence = c_sGetValueFromPath( pParticleSystem->fPathTime,
					pParticleSystem->pDefinition->tParticleWindInfluence );

				VECTOR4 vWindInSystem;
				vWindInSystem = sgvWindDirection * fWindInfluence;

				e_HavokFXSetSystemParamVector( pParticleSystem->nId, HAVOKFX_PHYSICS_SYSTEM_PARAM_WIND_FORCE, vWindInSystem );
			}

		}
	}

	if ( bUsingHavokFX )
	{ // Our Havok FX systems apply color and glow globally
		DWORD dwAlphaBits = pParticleSystem->dwSystemColor & 0xff000000;
		REF(dwAlphaBits);

		CFloatTriple tColorTriple = pParticleSystem->pDefinition->tParticleColor.GetValue( pParticleSystem->fPathTime );
		pParticleSystem->vHavokFXSystemColor.fX = tColorTriple.fX;
		pParticleSystem->vHavokFXSystemColor.fY = tColorTriple.fY;
		pParticleSystem->vHavokFXSystemColor.fZ = tColorTriple.fZ;
		float fAlpha = c_sGetValueFromPath ( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tParticleAlpha );
		fAlpha *= pParticleSystem->fGlobalAlpha;
		pParticleSystem->vHavokFXSystemColor.fW = fAlpha;

		float fGlow = c_sGetValueFromPath ( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tParticleGlow );
		pParticleSystem->fHavokFXSystemGlow = fGlow;

		float fScale = c_sGetValueFromPath ( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tParticleScale );
		pParticleSystem->fHavokFXSystemScale = fScale;


		if ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_USE_HAVOKFX_PARTICLES )
		{
			CFloatPair pairDuration = pParticleSystem->pDefinition->tParticleDurationPath.GetValue(pParticleSystem->fPathTime);
			//in editor range for this is between 0 and 50
			//pairDuration.fY *= 20;
			e_HavokFXSetSystemParamFloat(pParticleSystem->nId,HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_PARTICLE_LIFETIME,pairDuration.fY);

			CFloatPair pairRate = pParticleSystem->pDefinition->tParticlesPerSecondPath.GetValue(pParticleSystem->fPathTime);
			e_HavokFXSetSystemParamFloat(pParticleSystem->nId,HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_RATE,pairRate.fY);

			CFloatPair pairEmitterSize = pParticleSystem->pDefinition->tLaunchCylinderRadius.GetValue(pParticleSystem->fPathTime);
			VECTOR4 emRadius(pairEmitterSize.fY,pairEmitterSize.fY ,pairEmitterSize.fY,0);
			e_HavokFXSetSystemParamVector(pParticleSystem->nId,HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_SIZE,emRadius);

			CFloatPair pairLaunchSpeed = pParticleSystem->pDefinition->tLaunchSpeed.GetValue( pParticleSystem->fPathTime );
			VECTOR4 velMin(-0.025f,-0.025f,pairLaunchSpeed.fX,0); 
			e_HavokFXSetSystemParamVector(pParticleSystem->nId,HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_VEL_MIN,velMin);
			VECTOR4 velRange(0.005f,0.005f,pairLaunchSpeed.fY-pairLaunchSpeed.fX,0);
			e_HavokFXSetSystemParamVector(pParticleSystem->nId,HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_VEL_RANGE,velRange);

			CFloatPair pairAntiBounce = pParticleSystem->pDefinition->tParticleBounce.GetValue(pParticleSystem->fPathTime);
			VECTOR4 antiBounce(pairAntiBounce.fX,pairAntiBounce.fY,0,0);
			e_HavokFXSetSystemParamVector(pParticleSystem->nId,HAVOKFX_PHYSICS_SYSTEM_PARAM_PARTICLE_BEHAVIOR_ANTIBOUNCE,antiBounce);  

		}
		//if ( ! pParticleSystem->pDefinition->tParticleAcceleration.IsEmpty() ||
		//	 ! pParticleSystem->pDefinition->tParticleTurnSpeed.IsEmpty() )
		{
#define HAVOKFX_TURN_SPEED_DIVISOR 10.0f
#define HAVOKFX_ACCELERATION_DIVISOR 1000.0f
			float fAcceleration = c_sGetValueFromPath( pParticleSystem->fPathTime,
				pParticleSystem->pDefinition->tParticleAcceleration );
			float fTurnSpeed = c_sGetValueFromPath( pParticleSystem->fPathTime,
				pParticleSystem->pDefinition->tParticleTurnSpeed );
			VECTOR4 vZAccelerationVector;
			vZAccelerationVector.fX = fTurnSpeed / HAVOKFX_TURN_SPEED_DIVISOR;
			vZAccelerationVector.fY = fAcceleration / HAVOKFX_ACCELERATION_DIVISOR;
			vZAccelerationVector.fZ = 0.0f;

			e_HavokFXSetSystemParamVector( pParticleSystem->nId, HAVOKFX_PHYSICS_SYSTEM_PARAM_LINEAR_AND_ANGULAR_MULTIPLIERS, vZAccelerationVector );

		}

		CFloatPair tPair = pParticleSystem->pDefinition->tParticleSpeedBounds.GetValue( pParticleSystem->fPathTime );
		VECTOR4 vSpeedMax( tPair.fY, tPair.fY, tPair.fY, tPair.fY);
		e_HavokFXSetSystemParamVector( pParticleSystem->nId, HAVOKFX_PHYSICS_SYSTEM_PARAM_LINEAR_VEL_CAP, vSpeedMax );

		float zAcceleration = c_sGetValueFromPath ( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tParticleWorldAccelerationZ );
		VECTOR4 zAccelerationVector(0.0,0.0,zAcceleration,0.0);
		e_HavokFXSetSystemParamVector( pParticleSystem->nId, HAVOKFX_PHYSICS_SYSTEM_PARAM_Z_ACCELERATION,zAccelerationVector );

	}

	BOOL bRemoveAllParticles = FALSE;
	// if the system is dying, we have to kill the particle along with it.
	if ( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DYING ) != 0 &&
		(pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_DONT_DIE) != 0 )
	{
		bRemoveAllParticles = TRUE;
	}

	// Check whether there are any particles than need a screen texture.
	DWORD dwDrawLayerMask = DRAW_LAYER_SCREEN_EFFECT << PARTICLE_SYSTEM_SHIFT_DRAW_LAYER;
	if ( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_MASK_DRAW_LAYER ) == dwDrawLayerMask )
	{
		e_ParticlesSetNeedScreenTexture( TRUE );
	}

	// update current particles - this loop is more complicated because we might destroy stuff
	int nParticleCount = 0;
	int nParticleId = pParticleSystem->tParticles.GetFirst(); 
	while (	nParticleId != INVALID_ID )
	{
		nParticleCount++;

		int nParticleIdNext;
		PARTICLE * pParticle;
		float fParticlePathTime;
		// CHB 2006.08.17 - Reworked the logic here a bit to coax the compiler into generating better code.
		BOOL bRemoveParticle = bRemoveAllParticles;
		{
			nParticleIdNext = pParticleSystem->tParticles.GetNextId( nParticleId );
			pParticle = pParticleSystem->tParticles.Get( nParticleId );

			bRemoveParticle = bRemoveParticle || ((pParticle->dwFlags & PARTICLE_FLAG_DYING) != 0);

			bRemoveParticle = bRemoveParticle || (((pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_DONT_DIE) == 0) && (pParticle->fElapsedTime > pParticle->fDuration));

			if (!bRemoveParticle)
			{
				if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_FOLLOW_ROPE )
				{ 
					KNOT * pKnotCurr = pParticleSystem->tKnots.Get( pParticle->nFollowKnot );
					if ( !pKnotCurr || pKnotCurr->dwFlags & KNOT_FLAG_REMOVED )
						bRemoveParticle = TRUE;
				}
			}

			if ( bRemoveParticle )
			{
				//don't worry about removing them anymore, it's handled by removing the system
				//if ( bUsingHavokFX )
				//{
				//	e_HavokFXRemoveFXRigidBody( pParticle->nHavokFXRigidBodyId, pParticleSystem->nId, FALSE );
				//}
				pParticleSystem->tParticles.Remove( nParticleId );
				nParticleId = nParticleIdNext;
				continue;
			} 

			if ( bUsingHavokFX )
			{ // don't do much with a HavokFX particle
				pParticle->fElapsedTime += fTimeDelta;
				nParticleId = nParticleIdNext;
				continue;
			}

			fParticlePathTime = pParticle->fElapsedTime / pParticle->fDuration;
			if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_MASK_PARTICLES_LOOP )
			{
				// CHB 2006.08.18 - We really just want the fractional portion
				// here, such that 0 <= fParticlePathTime < 1.
				fParticlePathTime = Math_GetFractionalComponent(fParticlePathTime);
			}
			else if ( fParticlePathTime > 1.0f ) 
			{
				fParticlePathTime = 1.0f;
			}

			if (( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_FORCE_MOVE_PARTICLES ) != 0 ||
				( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_PARTICLES_MOVE_WITH_SYSTEM ) != 0 )
				&& pParticle->fElapsedTime > 0.0f)
			{
				pParticle->vPosition += vSystemPosDelta;
			}
		}

		BOOL bJustCreated = (pParticle->dwFlags & PARTICLE_FLAG_JUST_CREATED) != 0;
		BOOL bSystemIsFading = (pParticleSystem->dwFlags & (PARTICLE_SYSTEM_FLAG_FADE_IN | PARTICLE_SYSTEM_FLAG_FADE_OUT)) != 0;

		// update position
		if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ATTACH_PARTICLES )
		{ // particles attached to the system
			pParticle->vPosition = pParticleSystem->vPosition;
			pParticle->vVelocity = pParticleSystem->vVelocity;
		}
		else if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_FOLLOW_ROPE )
		{ // particles follow rope
			int nKnotCurr = pParticle->nFollowKnot;
			KNOT * pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );

			int nKnotPrev = pParticleSystem->tKnots.GetPrevId( nKnotCurr );
			KNOT * pKnotPrev = pParticleSystem->tKnots.Get( nKnotPrev );

			BOOL bLoopAround = (pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_PARTICLES_LOOP_ON_ROPE) != 0;
			for ( int i = 0; i < 2; i++ )// we should only go through this loop twice - and then only if we are looping around
			{
				if ( ! pKnotCurr )
					break;
				while ( pKnotCurr && pParticle->fRopeDistance > pKnotCurr->fDistanceFromStart )
				{
					nKnotPrev = nKnotCurr;
					pKnotPrev = pKnotCurr;

					nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr );
					pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );
				}
				while ( pKnotPrev && pParticle->fRopeDistance < pKnotPrev->fDistanceFromStart )
				{
					nKnotCurr = nKnotPrev;
					pKnotCurr = pKnotPrev;

					nKnotPrev = pParticleSystem->tKnots.GetPrevId( nKnotCurr );
					pKnotPrev = pParticleSystem->tKnots.Get( nKnotPrev );
				}
				if ( bLoopAround )
				{
					if ( ! pKnotCurr )
					{ // loop back to the beginning
						pParticle->fRopeDistance -= pKnotPrev->fDistanceFromStart;
						nKnotCurr = pParticleSystem->tKnots.GetFirst();
						pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );
						pKnotPrev = NULL;
						nKnotPrev = INVALID_ID;
					} 
					else if ( ! pKnotPrev ) 
					{// loop to the end
						nKnotPrev = INVALID_ID;
						nKnotCurr = pParticleSystem->tKnots.GetFirst();
						while ( nKnotCurr != INVALID_ID )
						{
							nKnotPrev = nKnotCurr;
							nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr );
						}
						nKnotCurr = nKnotPrev;
						if ( nKnotCurr != INVALID_ID )
							nKnotPrev = pParticleSystem->tKnots.GetPrevId( nKnotCurr );
						pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );
						pKnotPrev = pParticleSystem->tKnots.Get( nKnotPrev );
						ASSERT_CONTINUE( pKnotCurr );
						pParticle->fRopeDistance += pKnotCurr->fDistanceFromStart;
					}
				} else {
					break;
				}
			}
			pParticle->nFollowKnot = nKnotCurr;

			if ( ! pKnotCurr )
			{
				pParticle->dwFlags |= PARTICLE_FLAG_DYING;
			} 
			else if ( pKnotPrev )
			{
				float fPercentBetween = 0.0f;
				float fDeltaToCurr = pKnotCurr->fDistanceFromStart - pParticle->fRopeDistance;
				fDeltaToCurr = max( fDeltaToCurr, 0.0f );
				float fDeltaBetweenKnots = pKnotCurr->fDistanceFromStart - pKnotPrev->fDistanceFromStart;
				if ( fDeltaBetweenKnots != 0.0f )
					fPercentBetween = fDeltaToCurr / fDeltaBetweenKnots;
				ASSERT(fPercentBetween >= 0.0f && fPercentBetween <= 1.0f);
				pParticle->vPosition = VectorLerp( pKnotPrev->vPosition, pKnotCurr->vPosition, fPercentBetween );
				if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_USE_ROPE_SCALE )
				{
					pParticle->fScale = fPercentBetween * pKnotCurr->fScale + (1.0f - fPercentBetween) * pKnotPrev->fScale;
				}
			} 
			else 
			{
				pParticle->vPosition = pKnotCurr->vPosition;
				if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_USE_ROPE_SCALE )
				{
					pParticle->fScale = pKnotCurr->fScale;
				}
			}
			float fSpeedOrig = pParticle->vVelocity.fX;
			float fSpeedNew = fSpeedOrig + fTimeDelta * c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tParticleAcceleration );
			if ( fSpeedOrig != 0.0f )
				pParticle->vVelocity.fX *= fSpeedNew / fSpeedOrig;
			else
				pParticle->vVelocity.fX = fSpeedNew;

			pParticle->fRopeDistance += fTimeDelta * fSpeedNew;
		} 
		else 
		{ // particles have velocity, acceleration, etc...
			if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLE_GRAVITY )
			{
				pParticle->vVelocity += sgvGravity * fTimeDelta;
			} else if ( !pParticleSystem->pDefinition->tParticleWorldAccelerationZ.IsEmpty() ) {
				float fZAcceleration = c_sGetValueFromPath ( fParticlePathTime, 
															 pParticleSystem->pDefinition->tParticleWorldAccelerationZ, 
															 pParticle->fRandPercent,
															 6.7f );
				pParticle->vVelocity.fZ += fTimeDelta * fZAcceleration;
			} 
			float fAcceleration = c_sGetValueFromPath ( fParticlePathTime, 
														pParticleSystem->pDefinition->tParticleAcceleration, 
														pParticle->fRandPercent,
														17.2f );
			if ( fAcceleration != 0.0f )
			{
				VECTOR vDirection;
				VectorNormalize( vDirection, (VECTOR &)pParticle->vVelocity );
				VECTOR vAcceleration;
				VectorScale( vAcceleration, vDirection, fAcceleration * fTimeDelta );
				pParticle->vVelocity += vAcceleration;
			}

			if ( ! pParticleSystem->pDefinition->tParticleAttractorAcceleration.IsEmpty() ||
				fAttractorForceRadius != 0.0f )
			{
				float fAttractorAcceleration = c_sGetValueFromPath ( fParticlePathTime, 
					pParticleSystem->pDefinition->tParticleAttractorAcceleration, 
					pParticle->fRandPercent,
					43.8f );

				if ( fAttractorAcceleration != 0.0f || fAttractorForceRadius != 0.0f )
				{
					VECTOR vAttractorDelta = pParticleSystem->vAttractor - pParticle->vPosition;
					float fDistToAttractor = VectorLength( vAttractorDelta );
					VECTOR vDirection;
					if ( fDistToAttractor != 0.0f )
					{
						VectorScale( vDirection, vAttractorDelta, 1.0f / fDistToAttractor );
					}

					if ( fAttractorForceRadius != 0.0f )
					{
						float fAttractorPath = fDistToAttractor / fAttractorForceRadius;
						fAttractorAcceleration += c_sGetValueFromPath( fAttractorPath, pParticleSystem->pDefinition->tAttractorForceOverRadius );
					}

					if ( fAttractorAcceleration != 0.0f )
					{
						VECTOR vAcceleration;
						VectorScale( vAcceleration, vDirection, fAttractorAcceleration * fTimeDelta );
						pParticle->vVelocity += vAcceleration;
					}

					if ( bAttractorDestroys )
					{
						if ( fDistToAttractor >= fAttractorDestructionRadiusMin && 
							fDistToAttractor <= fAttractorDestructionRadiusMax )
							bRemoveParticle = TRUE;
					}
				}

				// apply speed bounds
				if ( ! pParticleSystem->pDefinition->tParticleSpeedBounds.IsEmpty() )
				{
					CFloatPair tPair = pParticleSystem->pDefinition->tParticleSpeedBounds.GetValue( fParticlePathTime );
					VECTOR vVelocityDirection;
					float fSpeedCurr = VectorNormalize( vVelocityDirection, pParticle->vVelocity );
					if ( fSpeedCurr < tPair.fX )
					{
						vVelocityDirection *= tPair.fX;
						pParticle->vVelocity = vVelocityDirection;
					} 
					else if ( fSpeedCurr > tPair.fY )
					{
						vVelocityDirection *= tPair.fY;
						pParticle->vVelocity = vVelocityDirection;
					}
				}
			}

			if ( ! pParticleSystem->pDefinition->tParticleWindInfluence.IsEmpty() )
			{
				float fWindInfluence = c_sGetValueFromPath( fParticlePathTime,
					pParticleSystem->pDefinition->tParticleWindInfluence,
					pParticle->fRandPercent,
					18.7f );
				if ( fWindInfluence != 0.0f )
				{
					float fWindDelta = sgfWindCurrent * fTimeDelta * fWindInfluence;
					pParticle->vVelocity += sgvWindDirection * fWindDelta;
				}
			}

			//VECTOR vPositionPrev = pParticle->vPosition;
			pParticle->vPosition +=	pParticle->vVelocity * fTimeDelta;
		}

		{
			if ( bRemoveParticle )
			{
				pParticleSystem->tParticles.Remove( nParticleId );
				nParticleId = nParticleIdNext;
				continue;
			} 

			if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_USE_LIGHTMAP )
			{
				pParticle->dwDiffuse = pParticleSystem->dwSystemColor;
			} else {
				if ( NEEDS_UPDATE(PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_COLOR) )
				{
					DWORD dwAlphaBits = pParticle->dwDiffuse & 0xff000000;
					pParticle->dwDiffuse = e_GetColorFromPath( fParticlePathTime, & pParticleSystem->pDefinition->tParticleColor );
					pParticle->dwDiffuse |= dwAlphaBits;
				}
				if ( bSystemIsFading || NEEDS_UPDATE(PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_ALPHA) )
				{
					float fAlpha = c_sGetValueFromPath ( fParticlePathTime, 
						pParticleSystem->pDefinition->tParticleAlpha, 
						pParticle->fRandPercent,
						32.5f );
					fAlpha *= pParticleSystem->fGlobalAlpha;
					pParticle->dwDiffuse &= 0x00ffffff;
					pParticle->dwDiffuse |= ((BYTE) (255 * fAlpha) << 24);
				}
				if ( NEEDS_UPDATE(PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_DISTORTION) )
				{
					if ( ! pParticleSystem->pDefinition->tParticleDistortionStrength.IsEmpty() )
					{
						float fDistortion = c_sGetValueFromPath ( fParticlePathTime, 
							pParticleSystem->pDefinition->tParticleDistortionStrength, 
							pParticle->fRandPercent,
							56.82f );
						pParticle->dwDiffuse &= 0xff000000;
						pParticle->dwDiffuse |= ((BYTE) (255 * fDistortion) << 16); // store it in the red
					}
				}
			}

			if ( NEEDS_UPDATE(PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_GLOW) )
			{
				if ( !pParticleSystem->pDefinition->tParticleGlow.IsEmpty() )
				{
					float fGlow = c_sGetValueFromPath ( fParticlePathTime, 
						pParticleSystem->pDefinition->tParticleGlow, 
						pParticle->fRandPercent,
						53.5f );
					pParticle->dwGlowColor = ((BYTE) (255 * fGlow) << 24);
				} 
				else if( (pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_GLOW) != 0 )
				{
					float fGlow = 1.0f;
					pParticle->dwGlowColor = ((BYTE) (255 * fGlow) << 24);
				}
				else
				{
					pParticle->dwGlowColor = 0;
				}
			}

			if ( (pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_USE_ROPE_SCALE) == 0 &&
				NEEDS_UPDATE(PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_SCALE) )
			{
				float fScaleAdjustment = c_sGetValueFromPath ( fParticlePathTime, 
																pParticleSystem->pDefinition->tParticleScale, 
																pParticle->fRandPercent,
																12.7f );
				pParticle->fScale = pParticle->fScaleInit * fScaleAdjustment;
			}

			if ( ! pParticleSystem->pDefinition->tParticleRotation.IsEmpty() )
			{
				float fRotationDelta = c_sGetValueFromPath ( fParticlePathTime, pParticleSystem->pDefinition->tParticleRotation, pParticle->fRandPercent, 18.4f );
				if ( fRotationDelta != 0.0f )
					pParticle->fRotation += fTimeDelta * 360 * fRotationDelta;
			}

			if ( ! pParticleSystem->pDefinition->tParticleCenterRotation.IsEmpty() )
			{
				float fRotationDelta = c_sGetValueFromPath( fParticlePathTime, pParticleSystem->pDefinition->tParticleCenterRotation,	pParticle->fRandPercent, 8.36f );
				if ( fRotationDelta != 0.0f )
					pParticle->fCenterRotation += fTimeDelta * 360 * fRotationDelta;
			}

			if ( NEEDS_UPDATE(PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_CENTER_X) )
				pParticle->fCenterX = c_sGetValueFromPath( fParticlePathTime, pParticleSystem->pDefinition->tParticleCenterX, pParticle->fRandPercent, 19.3f );
			if ( NEEDS_UPDATE(PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_CENTER_Y) )
				pParticle->fCenterY = c_sGetValueFromPath( fParticlePathTime, pParticleSystem->pDefinition->tParticleCenterY, pParticle->fRandPercent, 5.4f );
			if ( NEEDS_UPDATE(PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_STRETCH_BOX) )
				pParticle->fStretchBox		= c_sGetValueFromPath( fParticlePathTime, pParticleSystem->pDefinition->tParticleStretchBox,		pParticle->fRandPercent, 35.4f );
			if ( NEEDS_UPDATE(PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_STRETCH_DIAMOND) )
				pParticle->fStretchDiamond	= c_sGetValueFromPath( fParticlePathTime, pParticleSystem->pDefinition->tParticleStretchDiamond,	pParticle->fRandPercent, 55.8f );

			if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_TEXTURE_ANIMATES )
			{
				pParticle->fFrame += fTimeDelta * c_sGetValueFromPath ( fParticlePathTime, pParticleSystem->pDefinition->tAnimationRate, pParticle->fRandPercent, 11.7f );
				float fTotalFrames = (float)pParticleSystem->pDefinition->pnTextureFrameCount[ 0 ] * pParticleSystem->pDefinition->pnTextureFrameCount[ 1 ];
				pParticle->fFrame = FMODF( pParticle->fFrame, fTotalFrames );
			}
			pParticle->fElapsedTime += ( fTimeDelta * ( 1 + ( PARTICLE_TIMEDELTA_ACCELERATE_MAX * ( 1.0f - sParticlesCanReleaseDelta( pParticleSystem ) ) ) ) );
			nParticleId = nParticleIdNext;

		}

		pParticle->dwFlags &= ~PARTICLE_FLAG_JUST_CREATED;
	}
	pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_HAS_UPDATED_ONCE;
	return nParticleCount;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sClearParticleUpdateCounts()
{
	int nDef = DefinitionGetFirstId( DEFINITION_GROUP_PARTICLE );
	while ( nDef != INVALID_ID )
	{
		PARTICLE_SYSTEM_DEFINITION * pDef = (PARTICLE_SYSTEM_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_PARTICLE, nDef );
		if ( pDef )
		{
			pDef->nRuntimeParticleUpdatedCount = 0;
			pDef->nRuntimeRopeUpdatedCount = 0;
			nDef = DefinitionGetNextId( DEFINITION_GROUP_PARTICLE, nDef );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sParticleSystemHandleCullingAndFading(
	GAME * pGame,
	PARTICLE_SYSTEM * pParticleSystem,
	BOOL & bParticleSystemIsNew,
	const CAMERA_INFO * pCamera,
	float fTimeDelta )
{
	BOOL bWasCulled = (pParticleSystem->dwFlags & (PARTICLE_SYSTEM_FLAG_CULLED_BY_DISTANCE | PARTICLE_SYSTEM_FLAG_CULLED_FOR_SPEED)) != 0;
	BOOL bIsCulled = bWasCulled;

	// cull by distance
	VECTOR vDelta = CameraInfoGetPosition(pCamera) - pParticleSystem->vPosition;
	if ( VectorLengthSquared( vDelta ) > pParticleSystem->pDefinition->fCullDistance * pParticleSystem->pDefinition->fCullDistance ||
		( ( pParticleSystem->pDefinition->nCullPriority != PARTICLE_SYSTEM_CULL_PRIORITY_UNDEFINED && 
			pParticleSystem->pDefinition->nCullPriority > sgnCullPriority ) ) )
	{
		if ( ! bWasCulled && ! bParticleSystemIsNew )
			pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_FADE_OUT;
		else
		{
			pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_CULLED_BY_DISTANCE;
			pParticleSystem->fGlobalAlpha = 0.0f;
			bIsCulled = TRUE;
		}
	} else {
		if ( pParticleSystem->dwFlags & (PARTICLE_SYSTEM_FLAG_FADE_OUT | PARTICLE_SYSTEM_FLAG_CULLED_BY_DISTANCE) )
		{
			pParticleSystem->dwFlags &= ~(PARTICLE_SYSTEM_FLAG_FADE_OUT | PARTICLE_SYSTEM_FLAG_CULLED_BY_DISTANCE);
			pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_FADE_IN;
			if ( bWasCulled )
				pParticleSystem->fGlobalAlpha = 0.0f;
		} 
		bIsCulled = FALSE;
	}
	if ( !AppIsHammer() && 
		( pParticleSystem->idRoom == INVALID_ID ||
			( pParticleSystem->idRoom != INVALID_ID && ! RoomGetByID( pGame, pParticleSystem->idRoom ) ) ) )
	{
		bIsCulled = TRUE;
		pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_CULLED_BY_DISTANCE;
	}

	// cull for speed

	if ( pParticleSystem->dwFlags & ( PARTICLE_SYSTEM_FLAG_FADE_IN | PARTICLE_SYSTEM_FLAG_FADE_OUT ) )
	{
		float fAlphaDelta = fTimeDelta / PARTICLE_SYSTEM_FADE_TIME;
		if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_FADE_IN )
		{
			// we want to have one frame with the fade flag set and global alpha set to 1.0f
			if ( pParticleSystem->fGlobalAlpha == 1.0f )
			{
				pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_FADE_IN;
				pParticleSystem->fGlobalAlpha = 1.0f;
			} 
			pParticleSystem->fGlobalAlpha += fAlphaDelta;
			if ( pParticleSystem->fGlobalAlpha >= 1.0f )
			{
				pParticleSystem->fGlobalAlpha = 1.0f;
			}
		} else {
			pParticleSystem->fGlobalAlpha -= fAlphaDelta;
			if ( pParticleSystem->fGlobalAlpha <= 0.0f )
			{
				pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_FADE_OUT;
				pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_CULLED_BY_DISTANCE;
				bIsCulled = TRUE;
				pParticleSystem->fGlobalAlpha = 0.0f;
			} 
		}
	} 

	if ( bIsCulled && !bWasCulled )
	{
		pParticleSystem->tKnots.Clear();
		pParticleSystem->tParticles.Clear();
	} 
	else if ( !bIsCulled && bWasCulled )
	{
		if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_LOOPS )
		{
			bParticleSystemIsNew = TRUE;
			pParticleSystem->fElapsedTime = 0.0f;
			pParticleSystem->fLastParticleBurstTime = 0.0f;
			pParticleSystem->fTimeLastParticleCreated = 0.0f;
		}
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// CHB 2007.09.12
bool sShouldSkipProcessing(PARTICLE_SYSTEM * pParticleSystem, bool bUpdate = false)
{
#if ISVERSION(SERVER_VERSION)
	REF(pParticleSystem);
	REF(bUpdate);
#else
	if (pParticleSystem == 0)
	{
		// Difficult to process a null system.
		return true;
	}
	if (pParticleSystem->pDefinition == 0)
	{
		// Someone may want to process even if it doesn't have a definition.
		return false;
	}

	// Check to see if it's an async system,
	// and if the async option is disabled.
	bool bParticleSystemHasAsyncSimulation = ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_ASYNC_SIMULATION ) != 0;
	if (bParticleSystemHasAsyncSimulation)
	{
		// If dead or dying, be sure to process.
		if (
			((pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DEAD) != 0)
				||
			((pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DYING) != 0)
			)
		{
			return false;
		}

		ASYNC_PARTICLE_SYSTEM * pAsyncParticleSystem = HashGet( sgtAsyncParticleSystems, pParticleSystem->nId );
		if (pAsyncParticleSystem == 0)
		{
			// If no async component yet, continue processing
			// and allow one to be created.
			return false;
		}

		if (e_OptionState_GetActive()->get_bAsyncEffects())
		{
			// Async effects are enabled.
			// Restart the async thread if necessary.
			if (bUpdate && (pAsyncParticleSystem != 0) && (!pAsyncParticleSystem->bMainThreadEnabled))
			{
				pAsyncParticleSystem->bMainThreadEnabled = true;
				sCreateAsyncParticleSystemJob( pParticleSystem, pAsyncParticleSystem );
			}
		}
		else
		{
			// Async effects are disabled.
			// Stop the async thread if necessary.
			if (bUpdate && (pAsyncParticleSystem != 0) && pAsyncParticleSystem->bMainThreadEnabled)
			{
				pAsyncParticleSystem->bMainThreadEnabled = false;
				WaitForSingleObject( pAsyncParticleSystem->hCallbackFinished, INFINITE );
			}
			return true;
		}
	}
#endif
	// Should process.
	return false;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define MAX_TIME_DELTA 1.0f/30.0f
#define MIN_OPTIMIZE_DELTA 1.0f
static float sgfOptimizeTime = 0.0f;
void c_ParticleSystemUpdateAll(
	GAME* pGame,
	float fTimeDelta )
{
	sUpdateParticlesAllowedToRelease();
	e_ParticlesSetNeedScreenTexture( FALSE );

	if ( fTimeDelta <= 0.0f )
		return;

	const CAMERA_INFO * pCamera = c_CameraGetInfo( FALSE );
	if ( !pCamera )
		return;

	// query engine for current wind settings, except in Hammer (so as not to blow away particle editor dialog view settings)
//	if ( ! c_GetToolMode() )
	{
		c_ParticleGetWindFromEngine();
	}

	// throttle our updates
	sgfOptimizeTime += fTimeDelta;

	sgnParticlesUpdateSync = 0;
	BOOL bOptimize = FALSE;
	if ( sgfOptimizeTime > MIN_OPTIMIZE_DELTA )
	{
		sgfOptimizeTime -= MIN_OPTIMIZE_DELTA;
		bOptimize = TRUE;
	}

	if ( fTimeDelta > MAX_TIME_DELTA )
	{
		fTimeDelta = MAX_TIME_DELTA;
	}

	if ( sgbUpdateParticleCounts )
	{
		sClearParticleUpdateCounts();
	}

	// loop through all of the particle systems and update them
	for (PARTICLE_SYSTEM* pParticleSystem = HashGetFirst(sgtParticleSystems); pParticleSystem; pParticleSystem = HashGetNext(sgtParticleSystems, pParticleSystem))
	{
		if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DEAD )
			continue;

		BOOL bParticleSystemIsNew = FALSE;
		if ( ! pParticleSystem->pDefinition )
		{ // see if the definition is loaded now
			c_sParticleSystemApplyDefinition( pParticleSystem );

			if ( ! pParticleSystem->pDefinition )
			{ // we haven't loaded yet, so just record time until we are loaded
				pParticleSystem->fElapsedTime += fTimeDelta;
				continue; 
			} else {
				if ( pParticleSystem->pDefinition->fStartDelay > pParticleSystem->fElapsedTime )
					pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_DELAYING;
				else {
					pParticleSystem->fElapsedTime = FMODF( pParticleSystem->fElapsedTime, pParticleSystem->fDuration );
					bParticleSystemIsNew = TRUE;
				}
			}
		}

		// CHB 2007.09.12
		if (sShouldSkipProcessing(pParticleSystem, true))
		{
			continue;
		}

#ifdef INTEL_CHANGES
		// figure out if we have an async system
		BOOL bParticleSystemHasAsyncSimulation = ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_ASYNC_SIMULATION ) != 0 );

		if ( bOptimize )
		{
			// for async systems, optimization will happen the in callback
			if ( bParticleSystemHasAsyncSimulation )
			{
				pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_ASYNC_OPTIMIZE;
			}
			else
			{
				pParticleSystem->tParticles.OrderForSpeed();
			}
		}
#else
		if ( bOptimize )
		{
			//pParticleSystem->tKnots    .OrderForSpeed();
			pParticleSystem->tParticles.OrderForSpeed();
		}
#endif

		if ( pParticleSystem->fElapsedTime == 0.0f )
		{
			if ( pParticleSystem->pDefinition->fStartDelay > 0.0f )
				pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_DELAYING;
			else
				bParticleSystemIsNew = TRUE;
		} 
		else if ( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DELAYING ) != 0 &&
			pParticleSystem->fElapsedTime >= pParticleSystem->pDefinition->fStartDelay )
		{
			pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_DELAYING;
			pParticleSystem->fElapsedTime = 0.0f;
			bParticleSystemIsNew = TRUE;
		}

		if ( bParticleSystemIsNew && pParticleSystem->pDefinition && (pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_START_AT_RANDOM_PERCENT) != 0 )
		{
			pParticleSystem->fElapsedTime = RandGetFloat( sgParticlesRand, pParticleSystem->fDuration );
		}

		sParticleSystemHandleCullingAndFading( pGame, pParticleSystem, bParticleSystemIsNew, pCamera, fTimeDelta );

		if ( (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DELAYING) == 0 && 
				(pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DYING) == 0 && 
				(pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DEAD) == 0)
		{
			// light start
			if ( pParticleSystem->pDefinition->nLightDefId != INVALID_ID && 
				( pParticleSystem->dwFlags & (PARTICLE_SYSTEM_FLAG_NOLIGHTS | PARTICLE_SYSTEM_FLAG_NODRAW ) ) == 0 &&
				pParticleSystem->nLightId == INVALID_ID )
				pParticleSystem->nLightId = e_LightNew( pParticleSystem->pDefinition->nLightDefId, e_LightPriorityTypeGetValue( LIGHT_PRIORITY_TYPE_PARTICLE ), pParticleSystem->vPosition );

			// sound start
			if ( pParticleSystem->pDefinition->nSoundGroup != INVALID_ID &&
				( pParticleSystem->dwFlags & (PARTICLE_SYSTEM_FLAG_NOSOUND | PARTICLE_SYSTEM_FLAG_NODRAW ) ) == 0 &&
				pParticleSystem->nSoundId == INVALID_ID )
			{
				if((pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_FORCE_PLAY_SOUND_FIRST_TIME) || 
					RandGetFloat(sgParticlesRand, 0.0f, 1.0f) < pParticleSystem->pDefinition->fSoundPlayChance)
				{
					pParticleSystem->nSoundId = c_SoundPlay( pParticleSystem->pDefinition->nSoundGroup, 
						&pParticleSystem->vPosition,
						SOUND_PLAY_EXINFO( pParticleSystem->pDefinition->nVolume ));
				}
				else
				{
					// If we're not playing this sound the first time,
					// then still give it a valid ID so that we don't think
					// that the next time through is the first time through
					pParticleSystem->nSoundId = c_SoundPlay( INVALID_ID );
				}
			} 

			// footstep start
			if ( pParticleSystem->pDefinition->nFootstep != INVALID_ID &&
				( pParticleSystem->dwFlags & (PARTICLE_SYSTEM_FLAG_NOSOUND | PARTICLE_SYSTEM_FLAG_NODRAW ) ) == 0 &&
				pParticleSystem->nFootstepId == INVALID_ID )
			{
				if((pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_FORCE_PLAY_SOUND_FIRST_TIME) || 
					RandGetFloat(sgParticlesRand, 0.0f, 1.0f) < pParticleSystem->pDefinition->fSoundPlayChance)
				{
					pParticleSystem->nFootstepId = c_FootstepPlay( pParticleSystem->pDefinition->nFootstep, 
						pParticleSystem->vPosition);
				}
				else
				{
					// If we're not playing this sound the first time,
					// then still give it a valid ID so that we don't think
					// that the next time through is the first time through
					pParticleSystem->nFootstepId = c_SoundPlay( INVALID_ID );
				}
			} 
		}

		VECTOR vSystemPosDelta(0.0f, 0.0f, 0.0f);

		if (!VectorIsZero(pParticleSystem->vPositionPrev))
		{
			float fPrevDelta = pParticleSystem->fElapsedTime - pParticleSystem->fPositionPrevTime;
			if (fPrevDelta > 0.01f)
			{
				vSystemPosDelta = pParticleSystem->vPosition - pParticleSystem->vPositionPrev;
				pParticleSystem->vVelocity = vSystemPosDelta;
				pParticleSystem->vVelocity /= fPrevDelta;
				pParticleSystem->vPositionPrev = pParticleSystem->vPosition;
				pParticleSystem->fPositionPrevTime = pParticleSystem->fElapsedTime;
			}
		}
		else
		{
			pParticleSystem->vPositionPrev = pParticleSystem->vPosition;
			pParticleSystem->fPositionPrevTime = pParticleSystem->fElapsedTime;
		}

#ifdef INTEL_CHANGES
		if ( bParticleSystemHasAsyncSimulation )
		{
			// we'll need this for the asynchronous update later
			// INTEL_NOTE: not sure what all we need to push to the async callback, but here goes
			pParticleSystem->vAsyncPosDelta += vSystemPosDelta;

			// save this for later, for when we dispatch the callback in draw
			if ( bParticleSystemIsNew )
				pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_ASYNC_SYSTEM_IS_NEW;
		}
#endif
		// check to see if the system is done
		if ( (pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_LOOPS) == 0 )
		{
			if ( pParticleSystem->fElapsedTime > pParticleSystem->fDuration &&
				( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DELAYING ) == 0 )
			{
				if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_KILL_PARTICLES_WITH_SYSTEM )
				{
					pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_DEAD;
					continue;
				}
				else
					pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_DYING;
			}
		} else if ( (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DELAYING) == 0 )
		{
			if ( pParticleSystem->fElapsedTime > pParticleSystem->fDuration )
			{
				if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_REPLAY_SOUND_ON_LOOP ) != 0 &&
					pParticleSystem->pDefinition->nSoundGroup != INVALID_ID &&
					( pParticleSystem->dwFlags & (PARTICLE_SYSTEM_FLAG_NOSOUND | PARTICLE_SYSTEM_FLAG_NODRAW ) ) == 0 &&
					RandGetFloat(sgParticlesRand, 0.0f, 1.0f) < pParticleSystem->pDefinition->fSoundPlayChance)
				{
					pParticleSystem->nSoundId = c_SoundPlay( pParticleSystem->pDefinition->nSoundGroup, 
						&pParticleSystem->vPosition,
						SOUND_PLAY_EXINFO( pParticleSystem->pDefinition->nVolume ));
				} 
				if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_REPLAY_SOUND_ON_LOOP ) != 0 &&
					pParticleSystem->pDefinition->nFootstep != INVALID_ID &&
					( pParticleSystem->dwFlags & (PARTICLE_SYSTEM_FLAG_NOSOUND | PARTICLE_SYSTEM_FLAG_NODRAW ) ) == 0 &&
					RandGetFloat(sgParticlesRand, 0.0f, 1.0f) < pParticleSystem->pDefinition->fSoundPlayChance)
				{
					pParticleSystem->nFootstepId = c_FootstepPlay( pParticleSystem->pDefinition->nFootstep, 
						pParticleSystem->vPosition);
				} 
			}
			while ( pParticleSystem->fElapsedTime > pParticleSystem->fDuration )
			{
				pParticleSystem->fElapsedTime			  -= pParticleSystem->fDuration;
				pParticleSystem->fTimeLastParticleCreated -= pParticleSystem->fDuration;
				pParticleSystem->fPositionPrevTime		  -= pParticleSystem->fDuration;
				pParticleSystem->fLastParticleBurstTime   -= 1.0f;
#ifdef INTEL_CHANGES
				if ( bParticleSystemHasAsyncSimulation )
					pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_ASYNC_ROLLBACK_TIMES;
#endif
			}
		} 

		if ( pParticleSystem->idRoom == INVALID_ID )
		{
			pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_VISIBLE;
		}

		ASSERT ( pParticleSystem->pDefinition->fDuration != 0.0f );

		if ( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DELAYING ) == 0 )
		{
			// Particle System path time
			pParticleSystem->fPathTime = pParticleSystem->fElapsedTime / pParticleSystem->fDuration;

			// pass path time to light
			if ( pParticleSystem->nLightId != INVALID_ID )
			{
				e_LightSetLifePercent( pParticleSystem->nLightId, pParticleSystem->fPathTime );
			}

			if ( ! pParticleSystem->pDefinition->tAlphaRef.IsEmpty() )
			{
				pParticleSystem->fAlphaRef = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tAlphaRef );
			}
			if ( ! pParticleSystem->pDefinition->tAlphaMin.IsEmpty() )
			{
				pParticleSystem->fAlphaMin = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tAlphaMin );
			}

			if ( ( pParticleSystem->dwFlags & (PARTICLE_SYSTEM_FLAG_CULLED_BY_DISTANCE | PARTICLE_SYSTEM_FLAG_CULLED_FOR_SPEED) ) == 0 )
			{
				int nParticleCount = 0;
#ifdef INTEL_CHANGES
				// only process here if we're not asynchronous
				if ( !bParticleSystemHasAsyncSimulation )
				{
					if ( c_sParticleSystemHasRope ( pParticleSystem ) )
					{
						c_sRopeUpdate(pGame, pParticleSystem, bParticleSystemIsNew, fTimeDelta );
						nParticleCount++;
					}
					nParticleCount += c_sParticleSystemUpdate( pParticleSystem, bParticleSystemIsNew, vSystemPosDelta, fTimeDelta );
				}
				else
				{
					pParticleSystem->fAsyncUpdateTime += fTimeDelta;
				}
#else
				if ( c_sParticleSystemHasRope ( pParticleSystem ) )
				{
					c_sRopeUpdate( pGame, pParticleSystem, bParticleSystemIsNew, fTimeDelta );
					nParticleCount++;
				}
				nParticleCount += c_sParticleSystemUpdate( pParticleSystem, bParticleSystemIsNew, vSystemPosDelta, fTimeDelta );
#endif
				sgnParticlesUpdateSync += nParticleCount;
				pParticleSystem->pDefinition->nRuntimeParticleUpdatedCount += nParticleCount;
			} else {
				pParticleSystem->fLastParticleBurstTime = pParticleSystem->fPathTime;
				pParticleSystem->fTimeLastParticleCreated = pParticleSystem->fElapsedTime;
			}
		}

		pParticleSystem->fElapsedTime += fTimeDelta;
		pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_WAS_NODRAW;
	}

	{
		PARTICLE_PERF(PART_CLEANUP)
		for (PARTICLE_SYSTEM* pParticleSystem = HashGetFirst(sgtParticleSystems); pParticleSystem; )
		{
			PARTICLE_SYSTEM* pParticleSystemNext = HashGetNext(sgtParticleSystems, pParticleSystem);

			if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DYING )
			{
				if ( ! pParticleSystem->pDefinition )
				{
					pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_DEAD;
				}
				else if ( ((pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ROPE_USES_HOSE_PHYSICS ) != 0 ||
						   (pParticleSystem->pDefinition->dwFlags2& PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_TRAIL_PHYSICS ) != 0 ) &&
						  ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_HAS_ROPE ) != 0 )
				{
					if ( pParticleSystem->tKnots.GetFirst() == INVALID_ID )
					{
						pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_DEAD;
					}
				} 
				else if ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_USE_HAVOKFX_PARTICLES )
				{
					if ( ! e_HavokFXGetSystemParticleCount( pParticleSystem->nId ) )
						pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_DEAD;
				}
				else if ( pParticleSystem->tParticles.GetFirst() == INVALID_ID )
				{
					pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_DEAD;
				}
				else if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_KILL_PARTICLES_WITH_SYSTEM )
				{
					pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_DEAD;
				}
			}
			if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DEAD )
			{
				if ( pParticleSystem->nLightId != INVALID_ID )
				{
					e_LightRemove( pParticleSystem->nLightId );
					pParticleSystem->nLightId = INVALID_ID;
				}
				BOOL bDestroy = TRUE;
				int nNext = pParticleSystem->nParticleSystemNext;
				while ( nNext != INVALID_ID )
				{
					PARTICLE_SYSTEM* pNext = HashGet(sgtParticleSystems, nNext);
					if ( pNext && ( pNext->dwFlags & PARTICLE_SYSTEM_FLAG_DEAD ) == 0 )
					{
						bDestroy = FALSE;
						break;
					}
					nNext = pNext ? pNext->nParticleSystemNext : INVALID_ID;
				}
				if ( bDestroy )
				{
					c_ParticleSystemDestroy( pParticleSystem->nId, FALSE );
				} else {
					// even if we don't want to kill the system, we should still kill the sounds and lights
					if ( pParticleSystem->nSoundId != INVALID_ID )
					{
						CLT_VERSION_ONLY( c_SoundStop( pParticleSystem->nSoundId ) );
						pParticleSystem->nSoundId = INVALID_ID;
					}
					if ( pParticleSystem->nFootstepId != INVALID_ID )
					{
						CLT_VERSION_ONLY( c_SoundStop( pParticleSystem->nFootstepId ) );
						pParticleSystem->nFootstepId = INVALID_ID;
					}

				}
			}
			pParticleSystem = pParticleSystemNext;
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleSystemsDestroyAll( void )
{
#ifdef INTEL_CHANGES
	// make sure that all of the async systems are done
	for (ASYNC_PARTICLE_SYSTEM* pAsyncParticleSystem = HashGetFirst(sgtAsyncParticleSystems); pAsyncParticleSystem; )
	{
		ASYNC_PARTICLE_SYSTEM* pAsyncParticleSystemNext = HashGetNext(sgtAsyncParticleSystems, pAsyncParticleSystem);
		pAsyncParticleSystem->bMainThreadEnabled = FALSE;
		WaitForSingleObject( pAsyncParticleSystem->hCallbackFinished, INFINITE );
		c_ParticleSystemDestroy( pAsyncParticleSystem->nId, FALSE, TRUE );
		pAsyncParticleSystem = pAsyncParticleSystemNext;
	}
#endif
	for (PARTICLE_SYSTEM* pParticleSystem = HashGetFirst(sgtParticleSystems); pParticleSystem; )
	{
		PARTICLE_SYSTEM* pParticleSystemNext = HashGetNext(sgtParticleSystems, pParticleSystem);
		c_ParticleSystemDestroy( pParticleSystem->nId, FALSE );
		pParticleSystem = pParticleSystemNext;
	}

	sgnDrawListCount = 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#ifdef INTEL_CHANGES
static void sAsyncParticlesDestroy( 
	int nId )
{
	// check to see if the original system is still around
	ASYNC_PARTICLE_SYSTEM *pAsyncParticleSystem = HashGet( sgtAsyncParticleSystems, nId );
	if ( pAsyncParticleSystem )
	{
		pAsyncParticleSystem->bMainThreadEnabled = FALSE;
		WaitForSingleObject( pAsyncParticleSystem->hCallbackFinished, INFINITE );

		// original system is gone, we should shut ourselves down
		// clean up our memory before destroying the system

		// destroy the event
		CloseHandle( pAsyncParticleSystem->hCallbackFinished );

		// make the particle system disappear
		// save some data for after we destroy the system
		int nDataIndex = pAsyncParticleSystem->nAsyncVerticesBufferIndex;
		MEMORYPOOL *pMemoryPool = pAsyncParticleSystem->pMemoryPool;

		// this will remove pAsyncParticleSystem from the hash
		c_ParticleSystemDestroy( nId, FALSE, TRUE );

#if USE_MEMORY_MANAGER
		pAsyncParticleSystem->pMemoryPoolHEAP->Term();
		IMemoryManager::GetInstance().RemoveAllocator(pAsyncParticleSystem->pMemoryPoolHEAP);
		delete pAsyncParticleSystem->pMemoryPoolHEAP;
		pAsyncParticleSystem->pMemoryPoolHEAP = NULL;
		
		pMemoryPool->Term();
		IMemoryManager::GetInstance().RemoveAllocator(pMemoryPool);
		delete pMemoryPool;
#else
		MemoryPoolFree( pMemoryPool);
#endif		

		// note that this index is now available
		sgbParticleSystemAsyncVerticesBufferAvailable[nDataIndex] = TRUE;
		sgnParticleSystemAsyncCount--;
	}

}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemDestroy( 
	 int nId, 
#ifdef INTEL_CHANGES
	 BOOL bDestroyNext,
	 BOOL bIsAsync )
#else
	 BOOL bDestroyNext )
#endif
{
#ifdef INTEL_CHANGES
	PARTICLE_SYSTEM* pParticleSystem = NULL;
	if ( bIsAsync )
	{
		ASYNC_PARTICLE_SYSTEM *pAsyncParticleSystem = HashGet( sgtAsyncParticleSystems, nId );
		if ( !pAsyncParticleSystem )
		{
			return;
		}
		if ( pAsyncParticleSystem->pParticleSortList )
			FREE( pAsyncParticleSystem->pMemoryPool, pAsyncParticleSystem->pParticleSortList );
		pAsyncParticleSystem->pParticleSortList = NULL;
		pAsyncParticleSystem->nParticlesToSortAllocated = 0;

		pParticleSystem = &( pAsyncParticleSystem->tParticleSystem );
	}
	else
	{
		pParticleSystem = HashGet(sgtParticleSystems, nId);
	}
#else
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nId);
#endif
	if ( !pParticleSystem)
	{
		return;
	}
	if ( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DYING ) == 0 ) 
	{
		c_ParticleSystemKill( nId, FALSE, TRUE );
	}

	if ( sParticleSystemUsesHavokFX( pParticleSystem ) )
	{
		e_HavokFXRemoveSystem( pParticleSystem->nId);
		pParticleSystem->nHavokFXParticleCount = 0;
		//pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_USING_HAVOK_FX;
	}

#ifdef INTEL_CHANGES
	// async systems are not in draw list to begin with
	if ( ! bIsAsync )
	{
		BOOL bFoundInDrawList = FALSE;
		for ( int i = 0; i < sgnDrawListCount; i++ )
		{
			if ( pParticleSystem == sgpDrawList[ i ] )
			{
				MemoryMove( sgpDrawList + i, (sgnDrawListAllocated - i) * sizeof(PARTICLE_SYSTEM*), sgpDrawList + i + 1, (sgnDrawListCount - i - 1) * sizeof(PARTICLE_SYSTEM *));
				sgnDrawListCount--;
				bFoundInDrawList = TRUE;
				break;
			}
		}
		ASSERT( bFoundInDrawList || ! pParticleSystem->pDefinition );
		for ( int i = 0; i < sgnDrawListCount; i++ )
		{
			ASSERT ( pParticleSystem != sgpDrawList[ i ] );
		}

		if ( pParticleSystem->pDefinition && pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_ASYNC_SIMULATION )
		{
			sAsyncParticlesDestroy( pParticleSystem->nId );
		}
	}
#else
	BOOL bFoundInDrawList = FALSE;
	for ( int i = 0; i < sgnDrawListCount; i++ )
	{
		if ( pParticleSystem == sgpDrawList[ i ] )
		{
			MemoryMove( sgpDrawList + i, (sgnDrawListAllocated - i) * sizeof(PARTICLE_SYSTEM*), sgpDrawList + i + 1, (sgnDrawListCount - i - 1) * sizeof(PARTICLE_SYSTEM *));
			sgnDrawListCount--;
			bFoundInDrawList = TRUE;
			break;
		}
	}
	ASSERT( bFoundInDrawList || ! pParticleSystem->pDefinition );
	for ( int i = 0; i < sgnDrawListCount; i++ )
	{
		ASSERT ( pParticleSystem != sgpDrawList[ i ] );
	}
#endif

	int nNextId = pParticleSystem->nParticleSystemNext;
	int nRopeEndId = pParticleSystem->nParticleSystemRopeEnd;

	if ( pParticleSystem->nSoundId != INVALID_ID )
	{
		CLT_VERSION_ONLY( c_SoundStop( pParticleSystem->nSoundId ) );
	}
	if ( pParticleSystem->nFootstepId != INVALID_ID )
	{
		CLT_VERSION_ONLY( c_SoundStop( pParticleSystem->nFootstepId ) );
	}

	if ( pParticleSystem->nLightId != INVALID_ID )
		e_LightRemove( pParticleSystem->nLightId );

	//if ( sParticleSystemUsesHavokFX( pParticleSystem ) )
	//{
	//	int nHavokFXWorld = sParticleSystemGetHavokFXWorld( pParticleSystem );
	//	e_HavokFXRemoveRigidBodies( nHavokFXWorld, pParticleSystem->nHavokFXParticleCount );
	//	pParticleSystem->nHavokFXParticleCount = 0;
	//}
	if ( pParticleSystem->pDefinition )
	{
		PARTICLE_SYSTEM_DEFINITION* pDefinition = pParticleSystem->pDefinition;
		sParticleSystemReleaseRef( pDefinition );

		if ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_USES_GPU_SIM )
		{
			V( e_ParticleSystemGPUSimDestroy( pParticleSystem ) );
		}
	}

	pParticleSystem->tParticles.Destroy();
	pParticleSystem->tKnots.Destroy();
#ifdef INTEL_CHANGES
	if ( bIsAsync )
	{
		HashRemove( sgtAsyncParticleSystems, nId );
	}
	else
	{
		HashRemove( sgtParticleSystems, nId );
	}
#else
	HashRemove(sgtParticleSystems, nId);
#endif

	if ( bDestroyNext )
	{
		if ( nNextId != INVALID_ID )
			c_ParticleSystemDestroy( nNextId,	 TRUE );
		if ( nRopeEndId != INVALID_ID )
			c_ParticleSystemDestroy( nRopeEndId, TRUE );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemKill ( 
	int nId,
	BOOL bDestroyNext,
#ifdef INTEL_CHANGES
	BOOL bForce,
	BOOL bIsAsync )
#else
	BOOL bForce )
#endif
{
#ifdef INTEL_CHANGES
	PARTICLE_SYSTEM* pParticleSystem = NULL;
	if ( bIsAsync )
	{
		ASYNC_PARTICLE_SYSTEM *pAsyncParticleSystem = HashGet( sgtAsyncParticleSystems, nId );
		if ( !pAsyncParticleSystem )
		{
			return;
		}

		pParticleSystem = &( pAsyncParticleSystem->tParticleSystem );
	}
	else
	{
		pParticleSystem = HashGet(sgtParticleSystems, nId);
	}
#else
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nId);
#endif
	if (!pParticleSystem)
	{
		return;
	}

	int nSystemNext		= pParticleSystem->nParticleSystemNext;
	int nSystemRopeEnd	= pParticleSystem->nParticleSystemRopeEnd;

	BOOL bKillSystem = TRUE;
	if ( ! bForce &&
		(pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DYING) == 0 &&
		 pParticleSystem->pDefinition &&
		 pParticleSystem->pDefinition->nDyingParticleSystem != INVALID_ID )
	{
		PARTICLE_SYSTEM_DEFINITION * pDyingDef = (PARTICLE_SYSTEM_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_PARTICLE, pParticleSystem->pDefinition->nDyingParticleSystem );
		if ( pDyingDef )
		{
			c_ParticleSystemLoad( pParticleSystem->pDefinition->nDyingParticleSystem );
			if ( pParticleSystem->pDefinition->nSoundGroup != pDyingDef->nSoundGroup && pParticleSystem->nSoundId != INVALID_ID )
			{
				CLT_VERSION_ONLY( c_SoundStop( pParticleSystem->nSoundId ) );
				pParticleSystem->nSoundId = INVALID_ID;
			}
			if ( pParticleSystem->pDefinition->nFootstep != pDyingDef->nFootstep && pParticleSystem->nFootstepId != INVALID_ID )
			{
				CLT_VERSION_ONLY( c_SoundStop( pParticleSystem->nFootstepId ) );
				pParticleSystem->nFootstepId = INVALID_ID;
			}

			sParticleSystemReleaseRef( pParticleSystem->pDefinition );
			sParticleSystemAddRef( pDyingDef );
			pParticleSystem->pDefinition = pDyingDef;
			pParticleSystem->fElapsedTime = 0.0f;
			pParticleSystem->fTimeLastParticleCreated = 0.0f;
			pParticleSystem->fLastParticleBurstTime = 0.0f;
			pParticleSystem->fDuration = pParticleSystem->pDefinition->fDuration;
			float fParticleDuration = c_sGetValueFromPath( 0.0f, pParticleSystem->pDefinition->tParticleDurationPath );

			for ( int nParticleId = pParticleSystem->tParticles.GetFirst(); 
				nParticleId != INVALID_ID;
				nParticleId = pParticleSystem->tParticles.GetNextId( nParticleId ) )
			{
				PARTICLE * pParticle = pParticleSystem->tParticles.Get( nParticleId );
				pParticle->fElapsedTime = 0.0f;
				pParticle->fDuration = fParticleDuration;
			}
	
			bKillSystem = FALSE;
		}
	}

	if ( bKillSystem )
	{
		pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_DYING;

		if ( sParticleSystemUsesHavokFX( pParticleSystem ) )
		{
			if ( pParticleSystem->nSoundId != INVALID_ID )
			{
				CLT_VERSION_ONLY( c_SoundStop( pParticleSystem->nSoundId ) );
				pParticleSystem->nSoundId = INVALID_ID;
			}
			if ( pParticleSystem->nFootstepId != INVALID_ID )
			{
				CLT_VERSION_ONLY( c_SoundStop( pParticleSystem->nFootstepId ) );
				pParticleSystem->nFootstepId = INVALID_ID;
			}
			e_HavokFXRemoveSystem( pParticleSystem->nId);
			pParticleSystem->nHavokFXParticleCount = 0;
			//pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_USING_HAVOK_FX;
		}
	}

	if ( bDestroyNext )
	{
		c_ParticleSystemKill( nSystemNext,		TRUE );
		c_ParticleSystemKill( nSystemRopeEnd,	TRUE );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

float c_ParticleSystemsGetLifePercent( 
	GAME * pGame, 
	int nId )
{
	if ( ! pGame )
		return 0.0f;

	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nId);
	if (!pParticleSystem)
	{
		return 0.0f;
	}

	if ( pParticleSystem->dwFlags & ( PARTICLE_SYSTEM_FLAG_DEAD | PARTICLE_SYSTEM_FLAG_DYING ) )
		return 0.0f;
	if ( ! pParticleSystem->pDefinition )
		return 0.0f;
	if ( pParticleSystem->pDefinition->fDuration == 0.0f )
		return 0.0f;
	float fTime = FMODF ( pParticleSystem->fElapsedTime, pParticleSystem->fDuration );
	return fTime / pParticleSystem->pDefinition->fDuration;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct PARTICLE_TO_DRAW 
{
	PARTICLE * pParticle;
	float fZ;
	VECTOR4 vTransformedPos;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sCompareParticles( const void * p1, const void * p2 )
{
	PARTICLE_TO_DRAW * pFirst  = (PARTICLE_TO_DRAW *) p1;
	PARTICLE_TO_DRAW * pSecond = (PARTICLE_TO_DRAW *) p2;
	if ( pFirst->fZ > pSecond->fZ )
		return -1;
	if ( pFirst->fZ < pSecond->fZ )
		return 1;
	return 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define MIN_SCALE_FOR_SCREEN_SIZE_PREDICTIONS 1.0f
static void sDrawParticles(
	PARTICLE_SYSTEM * pParticleSystem,
	PARTICLE_DRAW_INFO & tDrawInfo,
	const PLANE * tPlanes,
	PARTICLE_TO_DRAW *& pParticleSortList,
	int & nParticlesToSortAllocated,
	MEMORYPOOL * pMemoryPool,
	BOOL bDepthOnly )
{
	ASSERT_RETURN( pParticleSystem->pDefinition );
	BOOL bUsesProj2 = e_ParticleSystemUsesProj2( pParticleSystem );
	MATRIX * pmViewProj = bUsesProj2 ? &tDrawInfo.matViewProjection2 : &tDrawInfo.matViewProjection;
	// Draw particles
	VECTOR vCameraSide;
	VectorCross( vCameraSide, tDrawInfo.vEyeDirection, sgvUpVector );
	VectorNormalize( vCameraSide );

#ifdef PARTIClE_CULL_FOR_SPEED_ENABLED
	if ( !AppIsHammer() &&
		pParticleSystem->fPredictedScreenFill + tDrawInfo.fTotalScreenPredicted > tDrawInfo.fScreensMax &&
		(pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_LOOPS) == 0 &&
		pParticleSystem->pDefinition->nCullPriority != 1 )
	{
		pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_CULLED_FOR_SPEED;
		pParticleSystem->tKnots.Clear();
		pParticleSystem->tParticles.Clear();
		return;
	}
	pParticleSystem->fPredictedScreenFill = 0.0f;
#endif // PARTIClE_CULL_FOR_SPEED_ENABLED

	int nParticlesToSort = 0;
	for ( int nParticleId = pParticleSystem->tParticles.GetFirst(); 
		nParticleId != INVALID_ID; 
		nParticleId = pParticleSystem->tParticles.GetNextId( nParticleId ) )
	{
		PARTICLE * pParticle = pParticleSystem->tParticles.Get( nParticleId );

		if ( nParticlesToSort >= OLD_PARTICLE_DRAW_LIMIT &&
			(pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ALLOW_MORE_PARTICLES ) == 0 )
			continue;

		if( tPlanes && !e_PointInFrustum( &pParticle->vPosition, tPlanes, pParticle->fScale ) )
		{
			continue;
		}

		VECTOR4 vTransformedPosition;
		MatrixMultiply( &vTransformedPosition, &pParticle->vPosition, pmViewProj );
		float fApproxZ = vTransformedPosition.fZ / vTransformedPosition.fW;

		if ( nParticlesToSort == nParticlesToSortAllocated )
		{
			if ( ! nParticlesToSortAllocated )
				nParticlesToSortAllocated = 300;
			else
				nParticlesToSortAllocated *= 2;
			pParticleSortList = (PARTICLE_TO_DRAW *) REALLOC( pMemoryPool, pParticleSortList, sizeof( PARTICLE_TO_DRAW ) * nParticlesToSortAllocated );
		}

		pParticleSortList[ nParticlesToSort ].pParticle = pParticle;
		pParticleSortList[ nParticlesToSort ].fZ = fApproxZ;
		pParticleSortList[ nParticlesToSort ].vTransformedPos = vTransformedPosition;
		nParticlesToSort++;
	}

	if (    ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_TEXTURE_ADDITIVE_BLEND ) == 0 
		 && ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_TEXTURE_MULTIPLY_BLEND ) == 0 )
		qsort( pParticleSortList, nParticlesToSort, sizeof( PARTICLE_TO_DRAW ), sCompareParticles );

	for ( int i = 0; i < nParticlesToSort; i++ )
	{
		PARTICLE * pParticle = pParticleSortList[ i ].pParticle;

		VECTOR vNormalCurrent = tDrawInfo.vNormalCurrent;

		// get OFFSET vectors for this particle
		VECTOR vOffset = tDrawInfo.vUpCurrent; 
		BOOL bAlignWithVelocity = (pParticleSystem->pDefinition->dwFlags & 
			(PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_U_WITH_VELOCITY | PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_V_WITH_VELOCITY)) != 0;

		BOOL bAlignWithSystemNormal = ( pParticleSystem->pDefinition->dwFlags2 & 
			(PARTICLE_SYSTEM_DEFINITION_FLAG2_ALIGN_U_WITH_SYSTEM_NORMAL | PARTICLE_SYSTEM_DEFINITION_FLAG2_ALIGN_V_WITH_SYSTEM_NORMAL )) != 0;

		if ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_TURN_TO_FACE_CAMERA )
		{
			vNormalCurrent = tDrawInfo.vEyePosition - pParticle->vPosition;
			VectorNormalize( vNormalCurrent );
		}

		if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_DONT_FACE_CAMERA )
		{
			vOffset = VECTOR( 0, 0, 1 );
			if ( vOffset == vNormalCurrent || vOffset == -vNormalCurrent ) 
				vOffset = VECTOR( 1, 0, 0 ); 
			else
			{
				VECTOR vTemp;
				VectorCross( vTemp, vOffset, vNormalCurrent );
				vOffset = vTemp;
			}
		}
		else if ( bAlignWithVelocity || bAlignWithSystemNormal )
		{
			VectorNormalize( vOffset, bAlignWithVelocity ? pParticle->vVelocity : pParticleSystem->vNormalToUse );
			VECTOR vPerp;
			if ( vOffset == vNormalCurrent || vOffset == -vNormalCurrent )
			{
				if ( fabsf(vOffset.fZ) != 1.0f )
				{
					VectorCross( vPerp, vOffset, VECTOR( 0.0f, 0.0f, 1.0f ) );
				} else {
					VectorCross( vPerp, vOffset, VECTOR( 0.0f, 1.0f, 0.0f ) );
				}
				VectorNormalize( vPerp );
				vNormalCurrent = vPerp;
			} else {
				VectorCross( vPerp, vOffset, vNormalCurrent );
				VectorCross( vNormalCurrent, vPerp, vOffset );
				VectorNormalize( vNormalCurrent );
			}
			if ( (pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_V_WITH_VELOCITY) != 0 ||
				 (pParticleSystem->pDefinition->dwFlags2& PARTICLE_SYSTEM_DEFINITION_FLAG2_ALIGN_V_WITH_SYSTEM_NORMAL) != 0  )
			{
				vOffset = vPerp;
			}
		} 
		else if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_NORMAL_WITH_VELOCITY )
		{
			VectorNormalize( vNormalCurrent, pParticle->vVelocity );
			VECTOR vUp( 0.0f, 0.0f, 1.0f );
			if ( vNormalCurrent == vUp || vNormalCurrent == -vUp )
			{
				vUp = VECTOR( 1.0f, 0.0f, 0.0f );
			}
			VectorCross( vOffset, vUp, vNormalCurrent );
		}
		if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_FACE_UP )
		{
			vNormalCurrent = VECTOR( 0, 0, 1 );
			VECTOR vPerp;
			VectorCross( vPerp, vOffset, vNormalCurrent );
			VectorNormalize( vPerp );
			VectorCross( vOffset, vPerp, vNormalCurrent );
		}
		if ( pParticle->fRotation )
		{// rotate if necessary
			MATRIX mRotationMatrix;
			MatrixRotationAxis( mRotationMatrix, vNormalCurrent, DegreesToRadians( pParticle->fRotation ) );
			MatrixMultiply( &vOffset, &vOffset, &mRotationMatrix );
		} 
		VectorNormalize( vOffset );
		VECTOR vPerpOffset; 
		VectorCross( vPerpOffset, vOffset, vNormalCurrent );

		VectorNormalize( vPerpOffset, vPerpOffset );

		if ( pParticle->fStretchBox != 0.0f )
		{
			if(  pParticle->fStretchBox > 1 )
			{
				VECTOR vMidPerp = vOffset - vPerpOffset;
			    VectorNormalize( vMidPerp, vMidPerp );
				vMidPerp *= pParticle->fStretchBox;
  				vPerpOffset -= vMidPerp;
				vOffset += vMidPerp;  
			}
			
			else
			{
				VECTOR vMid = vPerpOffset + vOffset;
				VectorNormalize( vMid, vMid );
				vMid *= pParticle->fStretchBox;
				vPerpOffset += vMid;
				vOffset += vMid;
				VectorNormalize( vPerpOffset, vPerpOffset );
				VectorNormalize( vOffset, vOffset );
			}
		}

		VECTOR vPositionShift( 0.0f );
		if ( pParticle->fCenterX != 0.5f || pParticle->fCenterY != 0.5f )
		{
			VECTOR vXShift = vOffset + vPerpOffset;
			VECTOR vYShift = vOffset - vPerpOffset;
			vXShift *= pParticle->fCenterX - 0.5f;
			vYShift *= pParticle->fCenterY - 0.5f;
			vPositionShift = vXShift + vYShift;

			if ( pParticle->fCenterRotation != 0.0f )
			{
				MATRIX mRotationMatrix;
				MatrixRotationAxis( mRotationMatrix, vNormalCurrent, DegreesToRadians( pParticle->fCenterRotation ) );
				MatrixMultiply( &vPositionShift, &vPositionShift, &mRotationMatrix );
			}
		}

#ifdef PARTIClE_CULL_FOR_SPEED_ENABLED
		if ( tDrawInfo.fScreensMax && AppIsHellgate() && pParticleSortList[ i ].vTransformedPos.fW > 0.0f )
		{
			VECTOR vOffsetForScale = pParticle->vPosition + vOffset * pParticle->fScale;
			VECTOR vPerpForScale = pParticle->vPosition + vPerpOffset * pParticle->fScale;
			VECTOR4 vTransformedOffsetForScale;
			MatrixMultiply( &vTransformedOffsetForScale, &vOffsetForScale, pmViewProj );
			VECTOR4 vTransformedPerpForScale;
			MatrixMultiply( &vTransformedPerpForScale, &vPerpForScale, pmViewProj );

			float fMaxDelta = MAX( fabsf( vTransformedOffsetForScale.fX - pParticleSortList[ i ].vTransformedPos.fX ), fabsf( vTransformedOffsetForScale.fY - pParticleSortList[ i ].vTransformedPos.fY ) );
			float fPredictedScreenPercent_Offset = fabsf( fMaxDelta / pParticleSortList[ i ].vTransformedPos.fW );
			fPredictedScreenPercent_Offset = PIN( fPredictedScreenPercent_Offset, 0.0f, 1.0f );

			fMaxDelta = MAX( fabsf( vTransformedPerpForScale.fX - pParticleSortList[ i ].vTransformedPos.fX ), fabsf( vTransformedPerpForScale.fY - pParticleSortList[ i ].vTransformedPos.fY ) );
			float fPredictedScreenPercent_Perp = fabsf( fMaxDelta / pParticleSortList[ i ].vTransformedPos.fW );
			fPredictedScreenPercent_Perp = PIN( fPredictedScreenPercent_Perp, 0.0f, 1.0f );

			float fScreenPercent = fPredictedScreenPercent_Offset * fPredictedScreenPercent_Perp;
			fScreenPercent *= PARTICLE_FILL_TEXTURE_ALPHA_FACTOR;
			pParticleSystem->fPredictedScreenFill += fScreenPercent;
			//if ( fPredictedScreenPercent + tDrawInfo.fTotalScreenPredicted > tDrawInfo.fScreensMax )
			//	continue; // a crude but powerful way to keep particles from slowing down the framerate.
			tDrawInfo.fTotalScreenPredicted += fScreenPercent;
		}
#endif //#ifdef PARTIClE_CULL_FOR_SPEED_ENABLED

		if ( pParticleSystem->pDefinition->nModelDefId == INVALID_ID )
			e_ParticleDrawAddQuad( pParticleSystem, pParticle, tDrawInfo, vOffset, vPerpOffset, vPositionShift, bDepthOnly );
		else
			e_ParticleDrawAddMesh( pParticleSystem, pParticle, tDrawInfo, vOffset, vPerpOffset, vPositionShift, bDepthOnly );
	}
}

#ifdef INTEL_CHANGES
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define MAX_ASYNC_TIME_DELTA (0.05f)
static void sAsyncParticlesJob( JOB_DATA &jobData )
{
#ifdef THREAD_PROFILER
	__itt_event_start( sgtThreadProfilerEventAsyncParticles );
#endif

	int nId = CAST_PTR_TO_INT( jobData.data1 );
	
	// get async system
	ASYNC_PARTICLE_SYSTEM *pAsyncParticleSystem = HashGet( sgtAsyncParticleSystems, nId );
	ASSERT_RETURN( pAsyncParticleSystem != NULL );

	PARTICLE_SYSTEM *pParticleSystem = &( pAsyncParticleSystem->tParticleSystem );

	if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ASYNC_OPTIMIZE )
	{
		pParticleSystem->tParticles.OrderForSpeed();
	}

	float fTimeDelta = pAsyncParticleSystem->fAsyncUpdateTime;
	fTimeDelta = MIN( fTimeDelta, MAX_ASYNC_TIME_DELTA );
	if( fTimeDelta > 0.0f )
	{
		pAsyncParticleSystem->fAsyncUpdateTime = 0.0f;

		// combine the update and the draw together
		
		// time for the update
		ASSERT( !( c_sParticleSystemHasRope ( pParticleSystem ) ) );
		BOOL bIsNew = (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ASYNC_SYSTEM_IS_NEW) != 0;
		pAsyncParticleSystem->nParticleCount = c_sParticleSystemUpdate( pParticleSystem, bIsNew, pParticleSystem->vAsyncPosDelta, fTimeDelta );

		// now prep the draw
		// get a local PARTICLE_DRAW_INFO structure
		PARTICLE_DRAW_INFO tDrawInfo;
		ZeroMemory( &tDrawInfo, sizeof( PARTICLE_DRAW_INFO ) );

		tDrawInfo.pParticleSystemCurr = pParticleSystem;
		tDrawInfo.nParticleSystemAsyncIndex = ( pAsyncParticleSystem->nAsyncVerticesBufferIndex + 1 );
		tDrawInfo.vEyeDirection = pAsyncParticleSystem->vAsyncEyeDirection;
		// this tDrawInfo now needs some vectors set
		// this code is copied from c_ParticlesDraw()

		// Prepare the OFFSET vectors for building particle geometry
		VECTOR vRightVector;
		VECTOR vUpVector(0, 0, 1.0);
		VectorCross    ( vRightVector, pAsyncParticleSystem->vAsyncEyeDirection, vUpVector );
		VectorNormalize( vRightVector );
		VECTOR vBillboardUpVector;
		VectorCross    ( vBillboardUpVector, vRightVector, pAsyncParticleSystem->vAsyncEyeDirection );
		VectorNormalize( vBillboardUpVector );

		if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_NORMAL_WITH_SYSTEM )
		{
			VECTOR vSystemRightVector;
			VectorCross    ( vSystemRightVector, pParticleSystem->vNormalToUse, vUpVector );
			VectorNormalize( vSystemRightVector, vSystemRightVector );
			VectorCross    ( tDrawInfo.vUpCurrent, vRightVector, pParticleSystem->vNormalToUse );
			if ( VectorIsZero( tDrawInfo.vUpCurrent ) )
			{
				tDrawInfo.vUpCurrent.fZ = 1.0f;
			}
			else
			{
				VectorNormalize( tDrawInfo.vUpCurrent, tDrawInfo.vUpCurrent );
			}
			tDrawInfo.vNormalCurrent = pParticleSystem->vNormalToUse;
		} else {
			tDrawInfo.vUpCurrent = vBillboardUpVector;
			tDrawInfo.vNormalCurrent = tDrawInfo.vEyeDirection;
		}

		// place all the vertices (doesn't make any draw calls)
		sDrawParticles( pParticleSystem, tDrawInfo, pAsyncParticleSystem->pAsyncPlanes,	pAsyncParticleSystem->pParticleSortList,
			pAsyncParticleSystem->nParticlesToSortAllocated, pAsyncParticleSystem->pMemoryPool, FALSE );

		// need to return some of the tDrawInfo data back like nVerticesInBuffer
		pAsyncParticleSystem->nVerticesInBuffer = tDrawInfo.nVerticesInBuffer;
		pAsyncParticleSystem->nIndexBufferCurr = tDrawInfo.nIndexBufferCurr;
	}

#ifdef THREAD_PROFILER
	__itt_event_end( sgtThreadProfilerEventAsyncParticles );
#endif

	// hey, we're done with the update, set the event
	SetEvent( pAsyncParticleSystem->hCallbackFinished );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static
void sCreateAsyncParticleSystemJob(
								   PARTICLE_SYSTEM * pParticleSystem,
								   ASYNC_PARTICLE_SYSTEM * pAsyncParticleSystem )
{
	if ( ! pAsyncParticleSystem->bMainThreadEnabled )
		return;

	// finished that frame, let's do it again
	// copy in all the data we've been saving
	if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ASYNC_ROLLBACK_TIMES )
	{
		pAsyncParticleSystem->tParticleSystem.fTimeLastParticleCreated	-= pAsyncParticleSystem->tParticleSystem.fDuration;
		pAsyncParticleSystem->tParticleSystem.fPositionPrevTime			-= pAsyncParticleSystem->tParticleSystem.fDuration;
		pAsyncParticleSystem->tParticleSystem.fLastParticleBurstTime	-= 1.0f;
		pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_ASYNC_ROLLBACK_TIMES;
	}
	pAsyncParticleSystem->tParticleSystem.fElapsedTime = pParticleSystem->fElapsedTime;
	pAsyncParticleSystem->tParticleSystem.fPathTime = pParticleSystem->fPathTime;
	if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ASYNC_SYSTEM_IS_NEW )
	{
		pAsyncParticleSystem->tParticleSystem.dwFlags |= PARTICLE_SYSTEM_FLAG_ASYNC_SYSTEM_IS_NEW;
		pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_ASYNC_SYSTEM_IS_NEW;
	} else {
		pAsyncParticleSystem->tParticleSystem.dwFlags &= ~PARTICLE_SYSTEM_FLAG_ASYNC_SYSTEM_IS_NEW;
	}
	pAsyncParticleSystem->tParticleSystem.vAsyncPosDelta = pParticleSystem->vAsyncPosDelta; 
	pParticleSystem->vAsyncPosDelta = VECTOR(0);
	pAsyncParticleSystem->tParticleSystem.vPosition = pParticleSystem->vPosition;
	pAsyncParticleSystem->tParticleSystem.vNormalToUse = pParticleSystem->vNormalToUse;
	pAsyncParticleSystem->tParticleSystem.idRoom = pParticleSystem->idRoom;
	if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ASYNC_OPTIMIZE )
	{
		pAsyncParticleSystem->tParticleSystem.dwFlags |= PARTICLE_SYSTEM_FLAG_ASYNC_OPTIMIZE;
		pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_ASYNC_OPTIMIZE;
	} else {
		pAsyncParticleSystem->tParticleSystem.dwFlags &= ~PARTICLE_SYSTEM_FLAG_ASYNC_OPTIMIZE;
	}

	pAsyncParticleSystem->fAsyncUpdateTime = pParticleSystem->fAsyncUpdateTime;
	pAsyncParticleSystem->vAsyncEyeDirection = pAsyncParticleSystem->vMainThreadEyeDirection;

	{
		int nSize = sizeof(PLANE) * NUM_FRUSTUM_PLANES;
		MemoryCopy( pAsyncParticleSystem->pAsyncPlanes, nSize, pAsyncParticleSystem->pMainThreadAsyncPlanes, nSize );
	}

	pParticleSystem->fAsyncUpdateTime = 0.0f;

	ResetEvent( pAsyncParticleSystem->hCallbackFinished );

	// try to send the job off
	// now actually send the job
	c_JobSubmit(
		JOB_TYPE_BIG,
		sAsyncParticlesJob,
		JOB_DATA( (DWORD_PTR)CAST_TO_VOIDPTR( pParticleSystem->nId ) ),
		sAsyncParticlesComplete
		);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sAsyncParticlesComplete( JOB_DATA &jobData )
{
	int nId = CAST_PTR_TO_INT( jobData.data1 );
	
	// make sure that async systems haven't already been cleaned up
	if ( ! sgbAsyncParticleSystemsValid )
	{
		return;
	}

	// get async system
	ASYNC_PARTICLE_SYSTEM *pAsyncParticleSystem = HashGet( sgtAsyncParticleSystems, nId );
	if ( pAsyncParticleSystem == NULL )
	{
		// must have already been forcibly killed
		return;
	}

	// check to see if the original system is still around
	PARTICLE_SYSTEM *pOriginalParticleSystem = c_ParticleSystemGet( nId );
	if( pOriginalParticleSystem == NULL )
	{
		sAsyncParticlesDestroy( nId );
		return;
	}

	// swap the buffers
	ASSERT_RETURN( pAsyncParticleSystem->nAsyncVerticesBufferIndex >= 0 );
	ASSERT_RETURN( pAsyncParticleSystem->nAsyncVerticesBufferIndex < PARTICLE_SYSTEM_ASYNC_MAX );
	PARTICLE_QUAD_VERTEX * pSwapTemp = sgpParticleSystemAsyncMainThreadVerticesBuffer[ pAsyncParticleSystem->nAsyncVerticesBufferIndex ];
	sgpParticleSystemAsyncMainThreadVerticesBuffer[ pAsyncParticleSystem->nAsyncVerticesBufferIndex ] = sgpParticleSystemAsyncVerticesBuffer[ pAsyncParticleSystem->nAsyncVerticesBufferIndex ];
	sgpParticleSystemAsyncVerticesBuffer[ pAsyncParticleSystem->nAsyncVerticesBufferIndex ] = pSwapTemp;

	pOriginalParticleSystem->nAyncVertexCount = pAsyncParticleSystem->nVerticesInBuffer;

	sCreateAsyncParticleSystemJob( pOriginalParticleSystem, pAsyncParticleSystem );
}

#endif
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sDrawParticlesLightmap(
	PARTICLE_SYSTEM * pParticleSystem,
	PARTICLE_DRAW_INFO & tDrawInfo ,
	const PLANE * tPlanes )
{
	ASSERT_RETURN( pParticleSystem->pDefinition );
	// Draw particles

	for ( int nParticleId = pParticleSystem->tParticles.GetFirst(); 
		nParticleId != INVALID_ID; 
		nParticleId = pParticleSystem->tParticles.GetNextId( nParticleId ) )
	{
		PARTICLE * pParticle = pParticleSystem->tParticles.Get( nParticleId );
		if( tPlanes && !e_PointInFrustumCheap( &pParticle->vPosition, tPlanes, 2 ) )
		{
			continue;
		}
		
		VECTOR vNormalCurrent = tDrawInfo.vNormalCurrent;

		// get OFFSET vectors for this particle
		VECTOR vOffset = tDrawInfo.vUpCurrent;
		BOOL bAlignWithVelocity = (pParticleSystem->pDefinition->dwFlags & 
			(PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_U_WITH_VELOCITY | PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_V_WITH_VELOCITY)) != 0;
		BOOL bAlignWithSystemNormal = ( pParticleSystem->pDefinition->dwFlags2 & 
			(PARTICLE_SYSTEM_DEFINITION_FLAG2_ALIGN_U_WITH_SYSTEM_NORMAL | PARTICLE_SYSTEM_DEFINITION_FLAG2_ALIGN_V_WITH_SYSTEM_NORMAL )) != 0;
		if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_DONT_FACE_CAMERA )
		{
			vOffset = VECTOR( 0, 1, 0 );
			if ( vOffset == vNormalCurrent || vOffset == -vNormalCurrent ) 
				vOffset = VECTOR( 1, 0, 0 ); 
			else
			{
				VECTOR vTemp;
				VectorCross( vTemp, vOffset, vNormalCurrent );
				vOffset = vTemp;
			}
		}
		else if ( bAlignWithVelocity || bAlignWithSystemNormal )
		{
			VectorNormalize( vOffset, bAlignWithVelocity ? pParticle->vVelocity : pParticleSystem->vNormalToUse );
			VECTOR vPerp;
			if ( vOffset == vNormalCurrent || vOffset == -vNormalCurrent )
			{
				if ( fabsf(vOffset.fY) != 1.0f )
				{
					VectorCross( vPerp, vOffset, VECTOR( 0.0f, 1.0f, 0.0f ) );
				} else {
					VectorCross( vPerp, vOffset, VECTOR( 1.0f, 0.0f, 0.0f ) );
				}
				VectorNormalize( vPerp );
				vNormalCurrent = vPerp;
			} else {
				VectorCross( vPerp, vOffset, vNormalCurrent );
				VectorCross( vNormalCurrent, vPerp, vOffset );
				VectorNormalize( vNormalCurrent );
			}
			if ( (pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_V_WITH_VELOCITY) != 0 ||
				 (pParticleSystem->pDefinition->dwFlags2& PARTICLE_SYSTEM_DEFINITION_FLAG2_ALIGN_V_WITH_SYSTEM_NORMAL) != 0  )
			{
				vOffset = vPerp;
			}
		} 
		else if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_NORMAL_WITH_VELOCITY )
		{
			VectorNormalize( vNormalCurrent, pParticle->vVelocity );
			VECTOR vUp( 0.0f, 1.0f, 0.0f );
			if ( vNormalCurrent == vUp || vNormalCurrent == -vUp )
			{
				vUp = VECTOR( 1.0f, 0.0f, 0.0f );
			}
			VectorCross( vOffset, vUp, vNormalCurrent );
		}
		if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_FACE_UP )
		{
			vNormalCurrent = VECTOR( 0, 1, 0 );
			VECTOR vPerp;
			VectorCross( vPerp, vOffset, vNormalCurrent );
			VectorNormalize( vPerp );
			VectorCross( vOffset, vPerp, vNormalCurrent );
		}
		if ( pParticle->fRotation )
		{// rotate if necessary
			MATRIX mRotationMatrix;
			MatrixRotationAxis( mRotationMatrix, vNormalCurrent, DegreesToRadians( pParticle->fRotation ) );
			MatrixMultiply( &vOffset, &vOffset, &mRotationMatrix );
		} 
		VectorNormalize( vOffset );
		VECTOR vPerpOffset; 
		VectorCross( vPerpOffset, vOffset, vNormalCurrent );

		VectorNormalize( vPerpOffset, vPerpOffset );

		if ( pParticle->fStretchBox != 0.0f )
		{
			VECTOR vMid = vPerpOffset + vOffset;
			VectorNormalize( vMid, vMid );
			vMid *= pParticle->fStretchBox;
			vPerpOffset += vMid;
			vOffset += vMid;
			VectorNormalize( vPerpOffset, vPerpOffset );
			VectorNormalize( vOffset, vOffset );
		}

		VECTOR vPositionShift( 0.0f );
		if ( pParticle->fCenterX != 0.5f || pParticle->fCenterY != 0.5f )
		{
			VECTOR vXShift = vOffset + vPerpOffset;
			VECTOR vYShift = vOffset - vPerpOffset;
			vXShift *= pParticle->fCenterX - 0.5f;
			vYShift *= pParticle->fCenterY - 0.5f;
			vPositionShift = vXShift + vYShift;

			if ( pParticle->fCenterRotation != 0.0f )
			{
				MATRIX mRotationMatrix;
				MatrixRotationAxis( mRotationMatrix, vNormalCurrent, DegreesToRadians( pParticle->fCenterRotation ) );
				MatrixMultiply( &vPositionShift, &vPositionShift, &mRotationMatrix );
			}
		}

		if ( pParticleSystem->pDefinition->nModelDefId == INVALID_ID )
			e_ParticleDrawAddQuad( pParticleSystem, pParticle, tDrawInfo, vOffset, vPerpOffset, vPositionShift, FALSE );
		else
			e_ParticleDrawAddMesh( pParticleSystem, pParticle, tDrawInfo, vOffset, vPerpOffset, vPositionShift, FALSE );
	}
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sShouldSubmitCurrSystemForDraw(
	PARTICLE_SYSTEM * pNext,
	PARTICLE_DRAW_INFO & tDrawInfo )
{
	if ( ! pNext || ! tDrawInfo.pParticleSystemCurr )
		return FALSE;

	if ( tDrawInfo.pParticleSystemCurr->pDefinition->nTextureId != 
		pNext->pDefinition->nTextureId )
		return TRUE;

	if ( tDrawInfo.pParticleSystemCurr->pDefinition->nModelDefId !=
		pNext->pDefinition->nModelDefId )
		return TRUE;

	if ( e_ParticleSystemUsesProj2( tDrawInfo.pParticleSystemCurr ) != e_ParticleSystemUsesProj2( pNext ) )
		return TRUE;

	if ( tDrawInfo.pParticleSystemCurr->fAlphaRef != pNext->fAlphaRef )
		return TRUE;

	if ( ! e_ParticleDefinitionsCanDrawTogether( tDrawInfo.pParticleSystemCurr->pDefinition, pNext->pDefinition ) )
		return TRUE;

	EFFECT_DEFINITION * pEffectDef = e_ParticleSystemGetEffectDef( tDrawInfo.pParticleSystemCurr->pDefinition );
	if ( pEffectDef && TESTBIT( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_ONE_PARTICLE_SYSTEM ) )
		return TRUE;

	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sUpdateHellriftConnection( PARTICLE_SYSTEM * pParticleSystem )
{
	EFFECT_DEFINITION * pEffectDef = e_ParticleSystemGetEffectDef( pParticleSystem->pDefinition );
	if ( !pEffectDef || !TESTBIT( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_USES_PORTALS ) )
		return;

	if ( pParticleSystem->nPortalID == INVALID_ID )
	{
		// search for this portal
		// look in the room and grab the first one without a particle (VP_FLAG_HAS_PARTICLE)

		ROOM * pRoom = RoomGetByID( AppGetCltGame(), pParticleSystem->idRoom );
		if ( pRoom )
		{
			int i;
			for ( i = 0; i < pRoom->nNumVisualPortals; i++ )
			{
				if ( 0 == ( pRoom->pVisualPortals[ i ].dwFlags & VP_FLAG_HAS_PARTICLE ) )
				{
					// found one!
					UNIT * pUnit = UnitGetById( AppGetCltGame(), pRoom->pVisualPortals[ i ].idVisualPortal );
					if ( pUnit )
					{
						VECTOR vUnitPos = UnitGetPosition( pUnit );
						VECTOR vPartPos = pParticleSystem->vPosition;
						// get a 2D distance
						vUnitPos.z = 0.f;
						vPartPos.z = 0.f;
						float fDistSqr = VectorDistanceSquared( vPartPos, vUnitPos );
						if ( fDistSqr < 0.01f )  // .1 meter max distance allowed
							break;
					}
				}
			}
			ASSERTX( i < pRoom->nNumVisualPortals, "Found a portal without a particle attachment, but it's not within a meter!" );
			if ( i < pRoom->nNumVisualPortals )
			{
				// found it, so attach (woo!)
				pRoom->pVisualPortals[ i ].dwFlags |= VP_FLAG_HAS_PARTICLE;
				pParticleSystem->nPortalID = pRoom->pVisualPortals[ i ].nEnginePortalID;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT c_ParticlesDraw(	
	BYTE bDrawLayer,
	const MATRIX * pmatView, 
	const MATRIX * pmatProj,
	const MATRIX * pmatProj2, 
	const VECTOR & vEyePosition,
	const VECTOR & vEyeDirection,
	BOOL bPixelShader,
	BOOL bDepthOnly )
{
#if !ISVERSION(SERVER_VERSION)
	if ( ! AppGetCltGame() )
		return S_FALSE;

#if defined(INTEL_CHANGES) && defined(THREAD_PROFILER)
	__itt_event_start( sgtThreadProfilerEventParticles );
#endif

	sgnParticlesUpdateAsync[ bDrawLayer ] = 0;

	// It's calling a constructor which is showing up on the profile.
	static PARTICLE_DRAW_INFO tDrawInfo;
	ZeroMemory( &tDrawInfo, sizeof( PARTICLE_DRAW_INFO ) );
	tDrawInfo.vEyePosition  = vEyePosition;
	tDrawInfo.vEyeDirection = vEyeDirection;
	tDrawInfo.bPixelShader = bPixelShader;
	{
		tDrawInfo.fScreensMax = sgfMaxPredictedParticleScreens;
	}

	UNIT * pControlUnit = GameGetControlUnit( AppGetCltGame() );
	if ( pControlUnit )
	{
		tDrawInfo.tFocusUnit.fHeight = UnitGetCollisionHeight( pControlUnit );
		tDrawInfo.tFocusUnit.fRadius = UnitGetCollisionRadius( pControlUnit );
		tDrawInfo.tFocusUnit.vPosition = UnitGetPosition( pControlUnit );
	}

	e_ParticleDrawInitFrame( (PARTICLE_DRAW_SECTION)(PARTICLE_DRAW_SECTION_NORMAL + bDrawLayer), tDrawInfo );

	// The particles are already in world coords.  So just set the ViewProjection matrix
	MatrixMultiply ( &tDrawInfo.matViewProjection,  pmatView, pmatProj );
	MatrixMultiply ( &tDrawInfo.matViewProjection2, pmatView, pmatProj2 );
	
	// prepare for our file format and lack of indices
	//HRVERIFY( GetDevice()->SetIndices( NULL ) );

	// Prepare the OFFSET vectors for building particle geometry
	VECTOR vRightVector;
	VECTOR vUpVector(0, 0, 1.0);
	VectorCross    ( vRightVector, vEyeDirection, vUpVector );
	VectorNormalize( vRightVector, vRightVector );
	VECTOR vBillboardUpVector;
	VectorCross    ( vBillboardUpVector, vRightVector, vEyeDirection );
	VectorNormalize( vBillboardUpVector, vBillboardUpVector );

	ASSERT( bDrawLayer < (1 << PARTICLE_SYSTEM_BITS_DRAW_LAYER) );
	DWORD dwDrawLayerMask = bDrawLayer << PARTICLE_SYSTEM_SHIFT_DRAW_LAYER;

	PARTICLE_SYSTEM * pParticleSystem = NULL;
	e_GetCurrentViewFrustum()->Update( pmatView, pmatProj );
	const PLANE * tPlanes = e_GetCurrentViewFrustum()->GetPlanes();

	REGION * pRegion = e_GetCurrentRegion();

	// draw each particle system
	for ( int i = 0; i < sgnDrawListCount; i++ )
	{
		pParticleSystem = sgpDrawList[ i ];

		// CHB 2007.09.12
		if (sShouldSkipProcessing(pParticleSystem))
		{
			continue;
		}

		if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_PARTICLE_IS_LIGHT ) != 0 &&
			 ( !AppIsHammer() || !e_GetDrawAllLights() ) ) 
			continue;

		BOOL bIsHavokFXSystem = sParticleSystemUsesHavokFX( pParticleSystem );

#ifdef INTEL_CHANGES
		BOOL bParticleSystemHasAsyncSimulation = ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_ASYNC_SIMULATION ) != 0 );
		ASYNC_PARTICLE_SYSTEM *pAsyncParticleSystem = HashGet( sgtAsyncParticleSystems, pParticleSystem->nId );
		if ( pAsyncParticleSystem == NULL )
		{
			bParticleSystemHasAsyncSimulation = FALSE;
		}

		// if we have an async system related to this one, don't short-circuit for an empty particle system
		if ( ! bParticleSystemHasAsyncSimulation )
		{
			if ( ! bIsHavokFXSystem )
			{
				if ( pParticleSystem->tParticles.GetFirst() == INVALID_ID &&
					( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_DRAW_ROPE ) == 0 )
					continue;
			} else {
				if ( pParticleSystem->nHavokFXParticleCount == 0 && 
					(pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_USE_HAVOKFX_PARTICLES ) == 0 )
					continue;
			}
		}
#else
		if ( ! bIsHavokFXSystem )
		{
			if ( pParticleSystem->tParticles.GetFirst() == INVALID_ID &&
				( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_DRAW_ROPE ) == 0 )
				continue;
		} else {
			if ( pParticleSystem->nHavokFXParticleCount == 0 && 
				(pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_USE_HAVOKFX_PARTICLES ) == 0 )
				continue;
		}
#endif

		if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DEAD )
			continue;

		if ( (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_VISIBLE) == 0 &&
			 (pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_IGNORE_MODEL_VISIBILITY) == 0 &&
			 (pParticleSystem->pDefinition->dwFlags  & PARTICLE_SYSTEM_DEFINITION_FLAG_DRAW_ROPE) == 0 ) // ropes have trouble figuring out visibility
			continue;

		if ( (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_NODRAW) != 0 &&
			(pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_IGNORE_MODEL_NODRAW) == 0 )
			continue;

		if ( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_MASK_DRAW_LAYER ) != dwDrawLayerMask )
			continue;

		if ( ! e_TextureIsLoaded( pParticleSystem->pDefinition->nTextureId, TRUE ) )
			continue;

		EFFECT_DEFINITION * pEffectDef = e_ParticleSystemGetEffectDef( pParticleSystem->pDefinition );
		// check the region
		if ( pRegion && pParticleSystem->nRegionID != REGION_INVALID && pParticleSystem->nRegionID != pRegion->nId && 
			(! pEffectDef || !TESTBIT( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_USES_PORTALS ) ) )
		{
			continue;
		}

//GPUDRAW

		gtBatchInfo.nParticleSystemsVisible++;

		// sometimes we need to draw whatever we have saved up...
		if ( (bIsHavokFXSystem || sShouldSubmitCurrSystemForDraw( pParticleSystem, tDrawInfo )) &&
			 tDrawInfo.pParticleSystemCurr )
		{
			sUpdateHellriftConnection( tDrawInfo.pParticleSystemCurr );

			if ( tDrawInfo.pParticleSystemCurr->pDefinition->nModelDefId == INVALID_ID )
			{
				V( e_ParticlesDrawBasic( tDrawInfo, FALSE, bDepthOnly ) );
			}
			else
			{
				V( e_ParticlesDrawMeshes( tDrawInfo, bDepthOnly ) );
			}
		}

#ifdef HAVOKFX_ENABLED
		if ( bIsHavokFXSystem )
		{
			V( e_ParticlesSystemDrawWithHavokFX( tDrawInfo, pParticleSystem ) );
			continue;
		}
#endif

		if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_USES_GPU_SIM ) 
		  && ! bDepthOnly )
		{
			

			//NOTE TO CC@HAVOK...  CONSIDER MOVING THIS FURTHER DOWN
         	tDrawInfo.vWindDirection = sgvWindDirection;
	        tDrawInfo.fWindMagnitude = sgfWindTimeCurrent;
   		    //seperated transforms needed for GPU sim
    	    tDrawInfo.matView = *pmatView;
	        tDrawInfo.matProjection = *pmatProj;

		    //set rendering parameters for the smoke
         	float SmokeRenderThickness = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tFluidSmokeThickness );
	        float SmokeAmbientLight = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tFluidSmokeAmbientLight );
			float SmokeDensityModifier = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tFluidSmokeDensityModifier );
			float SmokeVelocityModifier = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tFluidSmokeVelocityModifier );
        	DWORD diffuseColor = e_GetColorFromPath( pParticleSystem->fPathTime, & pParticleSystem->pDefinition->tParticleColor );
            float windInfluence = c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tParticleWindInfluence );
			VECTOR vOffset( c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tLaunchOffsetX), 
			               c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tLaunchOffsetY),
					       c_sGetValueFromPath( pParticleSystem->fPathTime, pParticleSystem->pDefinition->tLaunchOffsetZ)); 
			float glowMinDensity = c_sGetValueFromPath( pParticleSystem->fPathTime,pParticleSystem->pDefinition->tGlowMinDensity);

			V( e_ParticlesDrawGPUSim( tDrawInfo, pParticleSystem, SmokeRenderThickness, SmokeAmbientLight, diffuseColor, windInfluence, SmokeDensityModifier, SmokeVelocityModifier, vOffset, glowMinDensity ) );
			continue;
		}

		tDrawInfo.pParticleSystemCurr = pParticleSystem;

		if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_NORMAL_WITH_SYSTEM )
		{
			VECTOR vSystemRightVector;
			VectorCross    ( vSystemRightVector, pParticleSystem->vNormalToUse, vUpVector );
			VectorNormalize( vSystemRightVector, vSystemRightVector );
			VectorCross    ( tDrawInfo.vUpCurrent, vRightVector, pParticleSystem->vNormalToUse );
			if ( VectorIsZero( tDrawInfo.vUpCurrent ) )
				tDrawInfo.vUpCurrent.fZ = 1.0f;
			else
				VectorNormalize( tDrawInfo.vUpCurrent, tDrawInfo.vUpCurrent );
			tDrawInfo.vNormalCurrent = pParticleSystem->vNormalToUse;
		} else {
			tDrawInfo.vUpCurrent = vBillboardUpVector;
			tDrawInfo.vNormalCurrent = vEyeDirection;
		}

		/// Draw Ropes
		if ( ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_DRAW_ROPE ) != 0 &&
			c_sParticleSystemHasRope ( pParticleSystem ) )
		{
			e_ParticleDrawAddRope( pParticleSystem, tDrawInfo, bDepthOnly );
		} 

#ifdef INTEL_CHANGES
		if ( !bParticleSystemHasAsyncSimulation )
		{
			sDrawParticles( pParticleSystem, tDrawInfo, tPlanes, sgpParticleSortList, sgnParticleSortListAllocated, NULL, bDepthOnly );
		}
		else
		{
			// now draw all of the async particles
			// wait for the callback to be done
			// INTEL_NOTE: should do a short-circuit test here before waiting forever
			//WaitForSingleObject( pAsyncParticleSystem->hCallbackFinished, INFINITE );
			//ResetEvent( pAsyncParticleSystem->hCallbackFinished );

			sgnParticlesUpdateAsync[ bDrawLayer ] += pAsyncParticleSystem->nParticleCount;

			// time to draw
			// we've got to draw the vertices out in smaller chunks, so we don't overload the vertex buffers
			// we can massage tDrawInfo.nVerticesInBuffer and tDrawInfo.nFirstVertexOfBatch to make this work
			// nVerticesMultiplier will tell e_ParticlesDrawBasic which section of the array to use
			tDrawInfo.nParticleSystemAsyncIndex = ( pAsyncParticleSystem->nAsyncVerticesBufferIndex + 1 );
			tDrawInfo.nVerticesMultiplier = 0;
			tDrawInfo.nVerticesInBuffer = pAsyncParticleSystem->nVerticesInBuffer;
			tDrawInfo.nIndexBufferCurr = pAsyncParticleSystem->nIndexBufferCurr;
			
			// INTEL_NOTE: this should really be sgnParticleSystemVertexBufferSizes[ PARTICLE_VERTEX_BUFFER_BASIC ]
			// but we can't see that constant from here
			int nBufferSize = 6000;
			int nBufferTotal = tDrawInfo.nVerticesInBuffer;
			int nBatchSize = 0;
			
			tDrawInfo.nFirstVertexOfBatch = 0;
			while ( nBufferTotal > 0 )
			{
				nBatchSize = min( nBufferSize, nBufferTotal );
				tDrawInfo.nVerticesInBuffer = nBatchSize;

				// INTEL_NOTE: converting nVerticesInBuffer to nTrianglesInBatchCurr by arbitrary division
				// since we can't determine which number of triangles are responsible for this subset of vertices.
				// This will have to change if particles are handled by meshes instead of quads

				tDrawInfo.nTrianglesInBatchCurr = (tDrawInfo.nVerticesInBuffer / 3);

				if ( tDrawInfo.pParticleSystemCurr->pDefinition->nModelDefId == INVALID_ID )
				{
					V( e_ParticlesDrawBasic( tDrawInfo, FALSE, bDepthOnly ) );
				}
				else
				{
					V( e_ParticlesDrawMeshes( tDrawInfo, bDepthOnly ) );
				}

				nBufferTotal -= nBufferSize;
				tDrawInfo.nVerticesMultiplier++;
			}

			// clean things up for the next system
			tDrawInfo.pParticleSystemCurr = NULL;
			tDrawInfo.nParticleSystemAsyncIndex = 0;
			tDrawInfo.nVerticesInBuffer = 0;
			tDrawInfo.nIndexBufferCurr = 0;
			tDrawInfo.nVerticesMultiplier = 0;
			tDrawInfo.nTrianglesInBatchCurr = 0;
			tDrawInfo.nFirstVertexOfBatch = 0;

			pAsyncParticleSystem->vMainThreadEyeDirection = vEyeDirection;
			{
				int nSize = sizeof(PLANE) * NUM_FRUSTUM_PLANES;
				MemoryCopy( pAsyncParticleSystem->pMainThreadAsyncPlanes, nSize, tPlanes, nSize );
			}
		}
#else
		sDrawParticles		( pParticleSystem, tDrawInfo, tPlanes, sgpParticleSortList, sgnParticleSortListAllocated, NULL );
#endif
	}

	if ( tDrawInfo.pParticleSystemCurr && (tDrawInfo.nVerticesInBuffer || tDrawInfo.nMeshesInBatchCurr) )
	{
		if ( tDrawInfo.pParticleSystemCurr->pDefinition->nModelDefId == INVALID_ID )
		{
			V( e_ParticlesDrawBasic( tDrawInfo, FALSE, bDepthOnly ) );
		}
		else
		{
			V( e_ParticlesDrawMeshes( tDrawInfo, bDepthOnly ) );
		}
	}

	e_ParticlesSetPredictedScreenArea( tDrawInfo.fTotalScreenPredicted );

	e_ParticleDrawEndFrame();

#if defined(INTEL_CHANGES) && defined(THREAD_PROFILER)
	__itt_event_end( sgtThreadProfilerEventParticles );
#endif
#endif //#if !ISVERSION(SERVER_VERSION)

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT c_ParticlesDrawLightmap(	
	BYTE bDrawLayer,
	const MATRIX * pmatView, 
	const MATRIX * pmatProj,
	const MATRIX * pmatProj2, 
	const VECTOR & vEyePosition,
	const VECTOR & vEyeDirection,
	BOOL bPixelShader,
	BOOL bDepthOnly )
{
#if !ISVERSION(SERVER_VERSION)
	// It's calling a constructor which is showing up on the profile.
	static PARTICLE_DRAW_INFO tDrawInfo;
	ZeroMemory( &tDrawInfo, sizeof( PARTICLE_DRAW_INFO ) );
	tDrawInfo.vEyePosition  = vEyePosition;
	tDrawInfo.vEyeDirection = vEyeDirection;
	tDrawInfo.bPixelShader = bPixelShader;

	e_ParticleDrawInitFrameLightmap( PARTICLE_DRAW_SECTION_LIGHTMAP, tDrawInfo );
	

	// The particles are already in world coords.  So just set the ViewProjection matrix
	MatrixMultiply ( &tDrawInfo.matViewProjection,  pmatView, pmatProj );
	MatrixMultiply ( &tDrawInfo.matViewProjection2, pmatView, pmatProj2 );

	// prepare for our file format and lack of indices
	//HRVERIFY( GetDevice()->SetIndices( NULL ) );

	// Prepare the OFFSET vectors for building particle geometry
	VECTOR vRightVector;
	VECTOR vUpVector(0, 1.0, 0.0);
	VectorCross    ( vRightVector, vEyeDirection, vUpVector );
	VectorNormalize( vRightVector, vRightVector );
	VECTOR vBillboardUpVector;
	VectorCross    ( vBillboardUpVector, vRightVector, vEyeDirection );
	VectorNormalize( vBillboardUpVector, vBillboardUpVector );

	ASSERT( bDrawLayer < (1 << PARTICLE_SYSTEM_BITS_DRAW_LAYER) );
	DWORD dwDrawLayerMask = bDrawLayer << PARTICLE_SYSTEM_SHIFT_DRAW_LAYER;

	PARTICLE_SYSTEM * pParticleSystem = NULL;
	e_GetCurrentViewFrustum()->Update( pmatView, pmatProj );
	const PLANE * tPlanes = e_GetCurrentViewFrustum()->GetPlanes();
	//ASSERT_RETURN( tPlanes );
	// draw each particle system
	for ( int i = 0; i < sgnDrawListCount; i++ )
	{
		pParticleSystem = sgpDrawList[ i ];

		// CHB 2007.09.12
		if (sShouldSkipProcessing(pParticleSystem))
		{
			continue;
		}

		if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_PARTICLE_IS_LIGHT ) == 0 ||
			( pParticleSystem->pDefinition->dwFlags3 &  PARTICLE_SYSTEM_DEFINITION_FLAG3_LIGHT_PARTICLE_RENDER_FIRST ) != 0 ) 
			continue;
		if ( pParticleSystem->tParticles.GetFirst() == INVALID_ID &&
			( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_DRAW_ROPE ) == 0 )
			continue;

		if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DEAD )
			continue;

		if ( (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_VISIBLE) == 0 )
			continue;

		if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_NODRAW )
			continue;


		if ( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_MASK_DRAW_LAYER ) != dwDrawLayerMask )
			continue;

		if ( ! e_TextureIsLoaded( pParticleSystem->pDefinition->nTextureId, TRUE ) )
			continue;

		gtBatchInfo.nParticleSystemsVisible++;

		if ( sShouldSubmitCurrSystemForDraw( pParticleSystem, tDrawInfo ) &&
			 tDrawInfo.pParticleSystemCurr )
		{
			if ( tDrawInfo.pParticleSystemCurr->pDefinition->nModelDefId == INVALID_ID )
			{
				V( e_ParticlesDrawBasic( tDrawInfo, FALSE, FALSE ) );
			}
			//else
			//	e_ParticlesDrawMeshes( tDrawInfo );
		}

		tDrawInfo.pParticleSystemCurr = pParticleSystem;

		if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_NORMAL_WITH_SYSTEM )
		{
			VECTOR vSystemRightVector;
			VectorCross    ( vSystemRightVector, pParticleSystem->vNormalToUse, vUpVector );
			VectorNormalize( vSystemRightVector, vSystemRightVector );
			VectorCross    ( tDrawInfo.vUpCurrent, vRightVector, pParticleSystem->vNormalToUse );
			if ( VectorIsZero( tDrawInfo.vUpCurrent ) )
				tDrawInfo.vUpCurrent.fY = 1.0f;
			else
				VectorNormalize( tDrawInfo.vUpCurrent, tDrawInfo.vUpCurrent );
			tDrawInfo.vNormalCurrent = pParticleSystem->vNormalToUse;
		} else {
			tDrawInfo.vUpCurrent = vBillboardUpVector;
			tDrawInfo.vNormalCurrent = vEyeDirection;
		}
/*
		/// Draw Ropes
		if ( ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_DRAW_ROPE ) != 0 &&
			c_sParticleSystemHasRope ( pParticleSystem ) )
		{
			e_ParticleDrawAddRope( pParticleSystem, tDrawInfo );
		} 
*/
		sDrawParticlesLightmap		( pParticleSystem, tDrawInfo, tPlanes );
	}

	if ( tDrawInfo.pParticleSystemCurr && (tDrawInfo.nVerticesInBuffer || tDrawInfo.nMeshesInBatchCurr) )
	{
		if ( tDrawInfo.pParticleSystemCurr->pDefinition->nModelDefId == INVALID_ID )
		{
			V( e_ParticlesDrawBasic( tDrawInfo, FALSE, FALSE ) );
		}
		else
		{
			V( e_ParticlesDrawMeshes( tDrawInfo, FALSE ) );
		}
	}

	e_ParticleDrawEndFrameLightmap();

#endif //!SERVER_VERSION

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT c_ParticlesDrawPreLightmap(	
	BYTE bDrawLayer,
	const MATRIX * pmatView, 
	const MATRIX * pmatProj,
	const MATRIX * pmatProj2, 
	const VECTOR & vEyePosition,
	const VECTOR & vEyeDirection,
	BOOL bPixelShader,
	BOOL bDepthOnly )
{
#if !ISVERSION(SERVER_VERSION)
	// It's calling a constructor which is showing up on the profile.
	static PARTICLE_DRAW_INFO tDrawInfo;
	ZeroMemory( &tDrawInfo, sizeof( PARTICLE_DRAW_INFO ) );
	tDrawInfo.vEyePosition  = vEyePosition;
	tDrawInfo.vEyeDirection = vEyeDirection;
	tDrawInfo.bPixelShader = bPixelShader;

	e_ParticleDrawInitFrameLightmap( PARTICLE_DRAW_SECTION_PRELIGHTMAP, tDrawInfo );

	// The particles are already in world coords.  So just set the ViewProjection matrix
	MatrixMultiply ( &tDrawInfo.matViewProjection,  pmatView, pmatProj );
	MatrixMultiply ( &tDrawInfo.matViewProjection2, pmatView, pmatProj2 );

	// prepare for our file format and lack of indices
	//HRVERIFY( GetDevice()->SetIndices( NULL ) );

	// Prepare the OFFSET vectors for building particle geometry
	VECTOR vRightVector;
	VECTOR vUpVector(0, 1.0, 0.0);
	VectorCross    ( vRightVector, vEyeDirection, vUpVector );
	VectorNormalize( vRightVector, vRightVector );
	VECTOR vBillboardUpVector;
	VectorCross    ( vBillboardUpVector, vRightVector, vEyeDirection );
	VectorNormalize( vBillboardUpVector, vBillboardUpVector );

	ASSERT( bDrawLayer < (1 << PARTICLE_SYSTEM_BITS_DRAW_LAYER) );
	DWORD dwDrawLayerMask = bDrawLayer << PARTICLE_SYSTEM_SHIFT_DRAW_LAYER;

	PARTICLE_SYSTEM * pParticleSystem = NULL;
	e_GetCurrentViewFrustum()->Update( pmatView, pmatProj );
	const PLANE * tPlanes = e_GetCurrentViewFrustum()->GetPlanes();
	//ASSERT_RETURN( tPlanes );
	// draw each particle system
	for ( int i = 0; i < sgnDrawListCount; i++ )
	{
		pParticleSystem = sgpDrawList[ i ];

		// CHB 2007.09.12
		if (sShouldSkipProcessing(pParticleSystem))
		{
			continue;
		}

		if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_PARTICLE_IS_LIGHT ) == 0 ||
			( pParticleSystem->pDefinition->dwFlags3 &  PARTICLE_SYSTEM_DEFINITION_FLAG3_LIGHT_PARTICLE_RENDER_FIRST ) == 0 ) 
			continue;
		if ( pParticleSystem->tParticles.GetFirst() == INVALID_ID &&
			( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_DRAW_ROPE ) == 0 )
			continue;

		if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_DEAD )
			continue;

		if ( (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_VISIBLE) == 0 )
			continue;

		if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_NODRAW )
			continue;


		if ( ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_MASK_DRAW_LAYER ) != dwDrawLayerMask )
			continue;

		if ( ! e_TextureIsLoaded( pParticleSystem->pDefinition->nTextureId, TRUE ) )
			continue;

		gtBatchInfo.nParticleSystemsVisible++;

		if ( sShouldSubmitCurrSystemForDraw( pParticleSystem, tDrawInfo ) &&
			 tDrawInfo.pParticleSystemCurr )
		{
			if ( tDrawInfo.pParticleSystemCurr->pDefinition->nModelDefId == INVALID_ID )
			{
				V( e_ParticlesDrawBasic( tDrawInfo, FALSE, FALSE ) );
			}
			//else
			//	e_ParticlesDrawMeshes( tDrawInfo );
		}

		tDrawInfo.pParticleSystemCurr = pParticleSystem;

		if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_NORMAL_WITH_SYSTEM )
		{
			VECTOR vSystemRightVector;
			VectorCross    ( vSystemRightVector, pParticleSystem->vNormalToUse, vUpVector );
			VectorNormalize( vSystemRightVector, vSystemRightVector );
			VectorCross    ( tDrawInfo.vUpCurrent, vRightVector, pParticleSystem->vNormalToUse );
			if ( VectorIsZero( tDrawInfo.vUpCurrent ) )
				tDrawInfo.vUpCurrent.fY = 1.0f;
			else
				VectorNormalize( tDrawInfo.vUpCurrent, tDrawInfo.vUpCurrent );
			tDrawInfo.vNormalCurrent = pParticleSystem->vNormalToUse;
		} else {
			tDrawInfo.vUpCurrent = vBillboardUpVector;
			tDrawInfo.vNormalCurrent = vEyeDirection;
		}
/*
		/// Draw Ropes
		if ( ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_DRAW_ROPE ) != 0 &&
			c_sParticleSystemHasRope ( pParticleSystem ) )
		{
			e_ParticleDrawAddRope( pParticleSystem, tDrawInfo );
		} 
*/
		sDrawParticlesLightmap		( pParticleSystem, tDrawInfo, tPlanes );
	}

	if ( tDrawInfo.pParticleSystemCurr && (tDrawInfo.nVerticesInBuffer || tDrawInfo.nMeshesInBatchCurr) )
	{
		if ( tDrawInfo.pParticleSystemCurr->pDefinition->nModelDefId == INVALID_ID )
		{
			V( e_ParticlesDrawBasic( tDrawInfo, FALSE, FALSE ) );
		}
		//else
		//	e_ParticlesDrawMeshes( tDrawInfo );
	}

	e_ParticleDrawEndFrameLightmap();
#endif //!SERVER_VERSION

	return S_OK;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int c_ParticlesGetUpdateTotal()
{
	int nTotal = sgnParticlesUpdateSync;
	for ( int i = 0; i < NUM_PARTICLE_DRAW_SECTIONS; i++ )
		nTotal += sgnParticlesUpdateAsync[ i ];
	return nTotal;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sParticleSystemSetFlag( 
	int nParticleSystemId,
	DWORD dwFlag,
	BOOL bSet,
	BOOL bIncludeEnds )
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pParticleSystem)
	{
		return;
	}

	if ( ! bSet && 
		( dwFlag & PARTICLE_SYSTEM_FLAG_NODRAW ) != 0 &&
		( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_NODRAW ) != 0 )
	{ // this way, things don't streak particles when they suddenly become drawn
		pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_WAS_NODRAW;
	}

	if ( bSet )
	{
		pParticleSystem->dwFlags |= dwFlag;
	}
	else
	{
		pParticleSystem->dwFlags &= ~dwFlag;
	}

	if ( (dwFlag & ( PARTICLE_SYSTEM_FLAG_NODRAW | PARTICLE_SYSTEM_FLAG_NOLIGHTS )) != 0 && 
		pParticleSystem->nLightId != INVALID_ID )
	{
		BOOL bLightNoDraw = ( pParticleSystem->dwFlags & (PARTICLE_SYSTEM_FLAG_NOLIGHTS | PARTICLE_SYSTEM_FLAG_NODRAW ) ) != 0;
		e_LightSetFlags( pParticleSystem->nLightId, LIGHT_FLAG_NODRAW, bLightNoDraw );
	}

	if ( (dwFlag & ( PARTICLE_SYSTEM_FLAG_NODRAW | PARTICLE_SYSTEM_FLAG_NOSOUND )) != 0 &&
		pParticleSystem->nSoundId != INVALID_ID )
	{
		BOOL bMuteSound = ( pParticleSystem->dwFlags & (PARTICLE_SYSTEM_FLAG_NOSOUND | PARTICLE_SYSTEM_FLAG_NODRAW ) ) != 0;

		if ( bMuteSound )
		{
			c_SoundSetMute(pParticleSystem->nSoundId, TRUE);
		}
		else if ( pParticleSystem->nSoundId != INVALID_ID )
		{
			c_SoundSetMute(pParticleSystem->nSoundId, FALSE);
		}
	}

	if ( pParticleSystem->nParticleSystemNext != INVALID_ID )
	{
		c_sParticleSystemSetFlag( pParticleSystem->nParticleSystemNext,	  dwFlag, bSet, bIncludeEnds );
	}
	if ( bIncludeEnds && pParticleSystem->nParticleSystemRopeEnd != INVALID_ID )
	{
		c_sParticleSystemSetFlag( pParticleSystem->nParticleSystemRopeEnd, dwFlag, bSet, bIncludeEnds );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemSetVisible( 
	int nParticleSystemId, 
	BOOL bSet )
{
	c_sParticleSystemSetFlag( nParticleSystemId, PARTICLE_SYSTEM_FLAG_VISIBLE, bSet, TRUE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemSetDraw( 
	int nParticleSystemId, 
	BOOL bDraw )
{
	c_sParticleSystemSetFlag( nParticleSystemId, PARTICLE_SYSTEM_FLAG_NODRAW, !bDraw, TRUE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemSetNoLights( 
	int nParticleSystemId, 
	BOOL bSet )
{
	c_sParticleSystemSetFlag( nParticleSystemId, PARTICLE_SYSTEM_FLAG_NOLIGHTS, bSet, TRUE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemSetNoSound( 
	int nParticleSystemId, 
	BOOL bSet )
{
	c_sParticleSystemSetFlag( nParticleSystemId, PARTICLE_SYSTEM_FLAG_NOSOUND, bSet, TRUE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemSetDrawLayer( 
	int nParticleSystemId, 
	BYTE bDrawLayer,
	BOOL bLockDrawLayer )
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pParticleSystem)
	{
		return;
	}
	if ( bLockDrawLayer || (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_LOCK_DRAW_LAYER) == 0 )
	{
		ASSERT( bDrawLayer < (1 << PARTICLE_SYSTEM_BITS_DRAW_LAYER) );
		DWORD dwFlag = bDrawLayer << PARTICLE_SYSTEM_SHIFT_DRAW_LAYER;

		pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_MASK_DRAW_LAYER;

		pParticleSystem->dwFlags |= dwFlag;
	}

	if ( pParticleSystem->nParticleSystemNext != INVALID_ID )
	{
		c_ParticleSystemSetDrawLayer( pParticleSystem->nParticleSystemNext,	   bDrawLayer, bLockDrawLayer );
	}
	if ( pParticleSystem->nParticleSystemRopeEnd != INVALID_ID )
	{
		c_ParticleSystemSetDrawLayer( pParticleSystem->nParticleSystemRopeEnd, bDrawLayer, bLockDrawLayer );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemSetRegion( 
	int nParticleSystemId, 
	int nRegionID )
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pParticleSystem)
	{
		return;
	}

	pParticleSystem->nRegionID = nRegionID;

	if ( pParticleSystem->nParticleSystemNext != INVALID_ID )
	{
		c_ParticleSystemSetRegion( pParticleSystem->nParticleSystemNext,	nRegionID );
	}
	if ( pParticleSystem->nParticleSystemRopeEnd != INVALID_ID )
	{
		c_ParticleSystemSetRegion( pParticleSystem->nParticleSystemRopeEnd, nRegionID );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleSystemSetFirstPerson( 
	int nParticleSystemId, 
	BOOL bSet )
{
	c_sParticleSystemSetFlag( nParticleSystemId, PARTICLE_SYSTEM_FLAG_FIRST_PERSON, bSet, FALSE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemForceMoveParticles( 
	int nParticleSystemId,
	BOOL bForceMove )
{
	c_sParticleSystemSetFlag( nParticleSystemId, PARTICLE_SYSTEM_FLAG_FORCE_MOVE_PARTICLES, bForceMove, TRUE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int c_ParticleSystemGetDefinition( 
	int nParticleSystemId )
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pParticleSystem)
	{
		return INVALID_ID;
	}
	return pParticleSystem->nDefinitionId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL c_ParticleSystemExists( 
	int nParticleSystemId )
{
	return (HashGet(sgtParticleSystems, nParticleSystemId) != NULL);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL c_ParticleSystemCanDestroy( 
	int nParticleSystemId )
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if ( !pParticleSystem || !pParticleSystem->pDefinition )
	{
		return TRUE;
	}
	return ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_DONT_DESTROY_SYSTEM ) == 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemSetRopeEnd( 
	int nParticleSystemId,
	const VECTOR & vPosition,
	const VECTOR & vNormal,
	BOOL bLockEnd,
	BOOL bSetIdealPos,
	BOOL bIsFromParent )
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pParticleSystem || !pParticleSystem->pDefinition)
	{
		return;
	}

	if ( ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_END_IS_INDEPENDENT ) == 0 ||
		! bIsFromParent )
	{
		if ( bSetIdealPos )
			pParticleSystem->vRopeEndIdeal	= vPosition;
		if ( !pParticleSystem->pDefinition || pParticleSystem->pDefinition->fRopeEndSpeed == 1.0f || 
			VectorIsZero( pParticleSystem->vRopeEnd ) )
			pParticleSystem->vRopeEnd	= vPosition;
		pParticleSystem->vRopeEndNormal = vNormal;
		if ( bLockEnd )
			pParticleSystem->dwFlags |=  PARTICLE_SYSTEM_FLAG_ROPE_END_ATTACHED;
		else
			pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_ROPE_END_ATTACHED;

		pParticleSystem->dwFlags |= PARTICLE_SYSTEM_FLAG_ROPE_END_MOVED;
	}

	if ( pParticleSystem->nParticleSystemNext != INVALID_ID )
	{
		c_ParticleSystemSetRopeEnd( pParticleSystem->nParticleSystemNext, vPosition, vNormal, TRUE, TRUE, TRUE );
	}
	if ( pParticleSystem->nParticleSystemRopeEnd != INVALID_ID )
	{
		c_ParticleSystemMove( pParticleSystem->nParticleSystemRopeEnd,
							  pParticleSystem->vRopeEnd,
							  &pParticleSystem->vRopeEndNormal,
							  pParticleSystem->idRoom,
							  NULL );
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemReleaseRopeEnd( 
	int nParticleSystemId)
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pParticleSystem)
	{
		return;
	}
	pParticleSystem->dwFlags &= ~PARTICLE_SYSTEM_FLAG_ROPE_END_ATTACHED;

	if ( pParticleSystem->nParticleSystemNext != INVALID_ID )
	{
		c_ParticleSystemReleaseRopeEnd( pParticleSystem->nParticleSystemNext );
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleSystemSetParam( 
	int nParticleSystemId,
	PARTICLE_SYSTEM_PARAM eParam,
	float fValue)
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pParticleSystem)
	{
		return;
	}

	const int pnParamChart[] = 
	{
		OFFSET( PARTICLE_SYSTEM, fHosePressure 			), //PARTICLE_SYSTEM_PARAM_HOSE_PRESSURE,
		OFFSET( PARTICLE_SYSTEM, fCircleRadius 			), //PARTICLE_SYSTEM_PARAM_CIRCLE_RADIUS,
		OFFSET( PARTICLE_SYSTEM, fNovaStart    			), //PARTICLE_SYSTEM_PARAM_NOVA_START,
		OFFSET( PARTICLE_SYSTEM, fNovaAngle    			), //PARTICLE_SYSTEM_PARAM_NOVA_ANGLE,
		OFFSET( PARTICLE_SYSTEM, fRangePercent 			), //PARTICLE_SYSTEM_PARAM_RANGE_PERCENT,
		OFFSET( PARTICLE_SYSTEM, fDuration	   			), //PARTICLE_SYSTEM_PARAM_DURATION,
		OFFSET( PARTICLE_SYSTEM, fRopeBendMaxXY			), //PARTICLE_SYSTEM_PARAM_ROPE_MAX_BENDXY,
		OFFSET( PARTICLE_SYSTEM, fRopeBendMaxZ 			), //PARTICLE_SYSTEM_PARAM_ROPE_MAX_BENDZ,
		OFFSET( PARTICLE_SYSTEM, fParticleSpawnThrottle ), //PARTICLE_SYSTEM_PARAM_PARTICLE_SPAWN_THROTTLE,
	};

	if ( pParticleSystem->pDefinition && 
		(eParam != PARTICLE_SYSTEM_PARAM_DURATION ||
		 (pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_LOCK_DURATION) == 0) )
	{
		float * pfParam = (float *)((BYTE *)pParticleSystem + pnParamChart[ eParam ]);
		*pfParam = fValue;
	}

	if ( pParticleSystem->nParticleSystemNext != INVALID_ID )
	{
		c_ParticleSystemSetParam( pParticleSystem->nParticleSystemNext, eParam, fValue );
	}

	if ( pParticleSystem->nParticleSystemRopeEnd != INVALID_ID )
	{
		c_ParticleSystemSetParam( pParticleSystem->nParticleSystemRopeEnd, eParam, fValue );
	}
}

#if !ISVERSION(SERVER_VERSION)
//-------------------------------------------------------------------------------------------------
// forward declaration
//-------------------------------------------------------------------------------------------------
static void c_sParticleSystemLoadSounds(void * pDef, EVENT_DATA * pEventData);

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sParticleSystemDoLoadSounds(PARTICLE_SYSTEM_DEFINITION * pDefinition)
{
	ASSERT_RETURN(pDefinition);
	c_SoundLoad(pDefinition->nSoundGroup);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sParticleSystemDoLoadSounds(int nParticleSystemDefId)
{
	if ( nParticleSystemDefId == INVALID_ID )
		return;

	PARTICLE_SYSTEM_DEFINITION * pDefinition = 
		(PARTICLE_SYSTEM_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_PARTICLE, nParticleSystemDefId );

	if ( ! pDefinition )
	{
		EVENT_DATA eventData;
		DefinitionAddProcessCallback(DEFINITION_GROUP_PARTICLE, nParticleSystemDefId, c_sParticleSystemLoadSounds, &eventData);
		return;
	}

	c_sParticleSystemDoLoadSounds(pDefinition);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sParticleSystemLoadSounds(void * pDef, EVENT_DATA * pEventData)
{
	ASSERT_RETURN(pDef);

	PARTICLE_SYSTEM_DEFINITION * pDefinition = (PARTICLE_SYSTEM_DEFINITION *)pDef;
	c_sParticleSystemDoLoadSounds(pDefinition);

	sGetOtherSystemDef ( pDefinition, pDefinition->pszNextParticleSystem,	pDefinition->nNextParticleSystem );
	if ( pDefinition->nNextParticleSystem != INVALID_ID )
		c_sParticleSystemDoLoadSounds( pDefinition->nNextParticleSystem );

	sGetOtherSystemDef ( pDefinition, pDefinition->pszRopeEndParticleSystem, pDefinition->nRopeEndParticleSystem );
	if ( pDefinition->nRopeEndParticleSystem != INVALID_ID )
		c_sParticleSystemDoLoadSounds( pDefinition->nRopeEndParticleSystem );

	sGetOtherSystemDef ( pDefinition, pDefinition->pszDyingParticleSystem, pDefinition->nDyingParticleSystem );
	if ( pDefinition->nDyingParticleSystem != INVALID_ID )
		c_sParticleSystemDoLoadSounds( pDefinition->nDyingParticleSystem );
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleSystemLoad( 
	int nParticleSystemDefId )
{
#if !ISVERSION(SERVER_VERSION)
	if ( nParticleSystemDefId == INVALID_ID )
		return;

	PARTICLE_SYSTEM_DEFINITION * pDefinition = 
		(PARTICLE_SYSTEM_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_PARTICLE, nParticleSystemDefId );

	if ( ! pDefinition )
	{
		EVENT_DATA eventData;
		DefinitionAddProcessCallback(DEFINITION_GROUP_PARTICLE, nParticleSystemDefId, c_sParticleSystemLoadSounds, &eventData);
		return;
	}

	if ( pDefinition->dwRuntimeFlags & PARTICLE_SYSTEM_DEFINITION_RUNTIME_FLAG_LOADED )
		return;

	if ( pDefinition->nLightDefId == INVALID_ID )
	{
		if ( pDefinition->pszLightName[ 0 ] != 0 && 
			PStrCmp( pDefinition->pszLightName, BLANK_STRING ) != 0 )
		{
			pDefinition->nLightDefId = DefinitionGetIdByName( DEFINITION_GROUP_LIGHT, pDefinition->pszLightName );
		} else {
			pDefinition->nLightDefId = INVALID_ID;
		}
	}

	c_SoundLoad(pDefinition->nSoundGroup);

	c_sParticleSystemLoadTextures( pDefinition );
	e_ParticleSystemLoadModel( pDefinition );

	if ( pDefinition->pszRopePath[ 0 ] != 0 )
	{
		pDefinition->nRopePathId = sRopePathGet( pDefinition->pszRopePath );		
	}

	sGetOtherSystemDef ( pDefinition, pDefinition->pszNextParticleSystem,	pDefinition->nNextParticleSystem );
	if ( pDefinition->nNextParticleSystem != INVALID_ID )
		c_ParticleSystemLoad( pDefinition->nNextParticleSystem );

	sGetOtherSystemDef ( pDefinition, pDefinition->pszRopeEndParticleSystem, pDefinition->nRopeEndParticleSystem );
	if ( pDefinition->nRopeEndParticleSystem != INVALID_ID )
		c_ParticleSystemLoad( pDefinition->nRopeEndParticleSystem );

	sGetOtherSystemDef ( pDefinition, pDefinition->pszDyingParticleSystem, pDefinition->nDyingParticleSystem );
	if ( pDefinition->nDyingParticleSystem != INVALID_ID )
		c_ParticleSystemLoad( pDefinition->nDyingParticleSystem );

	pDefinition->dwRuntimeFlags |= PARTICLE_SYSTEM_DEFINITION_RUNTIME_FLAG_LOADED;
#endif //!ISVERSION(SERVER_VERSION)
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleSystemFlagSoundsForLoad( 
	int nParticleSystemDefId,
	BOOL bLoadNow)
{
#if !ISVERSION(SERVER_VERSION)
	if ( nParticleSystemDefId == INVALID_ID )
		return;

	PARTICLE_SYSTEM_DEFINITION * pDefinition = 
		(PARTICLE_SYSTEM_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_PARTICLE, nParticleSystemDefId );

	if ( ! pDefinition )
	{
		EVENT_DATA eventData;
		DefinitionAddProcessCallback(DEFINITION_GROUP_PARTICLE, nParticleSystemDefId, c_sParticleSystemLoadSounds, &eventData);
		return;
	}

	c_SoundFlagForLoad(pDefinition->nSoundGroup, bLoadNow);

	sGetOtherSystemDef ( pDefinition, pDefinition->pszNextParticleSystem,	pDefinition->nNextParticleSystem );
	if ( pDefinition->nNextParticleSystem != INVALID_ID )
		c_ParticleSystemFlagSoundsForLoad( pDefinition->nNextParticleSystem, bLoadNow );

	sGetOtherSystemDef ( pDefinition, pDefinition->pszRopeEndParticleSystem, pDefinition->nRopeEndParticleSystem );
	if ( pDefinition->nRopeEndParticleSystem != INVALID_ID )
		c_ParticleSystemFlagSoundsForLoad( pDefinition->nRopeEndParticleSystem, bLoadNow );

	sGetOtherSystemDef ( pDefinition, pDefinition->pszDyingParticleSystem, pDefinition->nDyingParticleSystem );
	if ( pDefinition->nDyingParticleSystem != INVALID_ID )
		c_ParticleSystemFlagSoundsForLoad( pDefinition->nDyingParticleSystem, bLoadNow );
#endif //!ISVERSION(SERVER_VERSION)
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleSetWind(
	float fMin,
	float fMax,
	const struct VECTOR & vWindDirection )
{
#if !ISVERSION(SERVER_VERSION)
	sgfWindMin = fMin;
	sgfWindMax = fMax;
	sgfWindCurrent = ( fMin + fMax ) / 2.0f;
	sgvWindDirection = vWindDirection;
#endif// !ISVERSION(SERVER_VERSION)
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleGetWindFromEngine()
{
#if !ISVERSION(SERVER_VERSION)
	float fMin, fMax;
	VECTOR vDir;
	e_GetWind( e_GetCurrentEnvironmentDef(), fMin, fMax, vDir );
	c_ParticleSetWind( fMin, fMax, vDir );
#endif // !ISVERSION(SERVER_VERSION)
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleSetCullPriority( 
	int nCullPriority )
{
	ASSERT_RETURN( nCullPriority >= PARTICLE_SYSTEM_CULL_PRIORITY_ALWAYS_DRAW );
	ASSERT_RETURN( nCullPriority <= MAX_PARTICLE_SYSTEM_CULL_PRIORITY );
	sgnCullPriority = nCullPriority;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int c_ParticleGetCullPriority()
{
	return sgnCullPriority;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL c_ParticleTestGlobalBit(
	int nBit )
{
	return TESTBIT( sgdwGlobalFlags, nBit );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleSetGlobalBit(
	int nBit,
	BOOL bValue)
{
	switch ( nBit )
	{
	case PARTICLE_GLOBAL_BIT_DX10_ENABLED:
		if ( !bValue || e_DX10IsEnabled() )
			SETBIT( sgdwGlobalFlags, nBit, bValue );
		break;

	case PARTICLE_GLOBAL_BIT_ASYNC_SYSTEMS_ENABLED:
		if ( !bValue || c_sCanDoAsyncSystems() )
			SETBIT( sgdwGlobalFlags, nBit, bValue );
		break;

	default:
		SETBIT( sgdwGlobalFlags, nBit, bValue );
		break;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleToggleDebugMode()
{
	sgbUpdateParticleCounts = ! sgbUpdateParticleCounts;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ParticleSystemSetVelocity( 
	int nParticleSystemId,
	const VECTOR & vVelocity)
{
	PARTICLE_SYSTEM* pParticleSystem = HashGet(sgtParticleSystems, nParticleSystemId);
	if (!pParticleSystem)
	{
		return;
	}

	pParticleSystem->vVelocity = vVelocity;

	if ( pParticleSystem->nParticleSystemNext != INVALID_ID )
	{
		c_ParticleSystemSetVelocity( pParticleSystem->nParticleSystemNext, vVelocity );
	}

	if ( pParticleSystem->nParticleSystemRopeEnd != INVALID_ID )
	{
		c_ParticleSystemSetVelocity( pParticleSystem->nParticleSystemRopeEnd, vVelocity );
	}

}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct UPDATE_FLAG_CHART 
{
	int nOffset;
	BOOL bTriple;
	DWORD dwFlag;
};

static const UPDATE_FLAG_CHART sgtUpdateFlagChart[] =
{
	// nOffset														triple		flag
	{ OFFSET(PARTICLE_SYSTEM_DEFINITION, tParticleGlow),			FALSE,		PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_GLOW },
	{ OFFSET(PARTICLE_SYSTEM_DEFINITION, tParticleScale),			FALSE,		PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_SCALE },
	{ OFFSET(PARTICLE_SYSTEM_DEFINITION, tParticleCenterX),			FALSE,		PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_CENTER_X },
	{ OFFSET(PARTICLE_SYSTEM_DEFINITION, tParticleCenterY),			FALSE,		PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_CENTER_Y },
	{ OFFSET(PARTICLE_SYSTEM_DEFINITION, tParticleStretchBox),		FALSE,		PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_STRETCH_BOX },
	{ OFFSET(PARTICLE_SYSTEM_DEFINITION, tParticleStretchDiamond),	FALSE,		PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_STRETCH_DIAMOND },
	{ OFFSET(PARTICLE_SYSTEM_DEFINITION, tParticleColor),			TRUE,		PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_COLOR },
	{ OFFSET(PARTICLE_SYSTEM_DEFINITION, tParticleAlpha),			FALSE,		PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_ALPHA },
	{ OFFSET(PARTICLE_SYSTEM_DEFINITION, tParticleDistortionStrength),FALSE,	PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_DISTORTION },
	{ -1,															FALSE,		0 }, // keep this last
};

BOOL ParticleSystemPostXMLLoad(
	void * pDef,
	BOOL bForceSync )
{
	PARTICLE_SYSTEM_DEFINITION * pParticleSystemDef = (PARTICLE_SYSTEM_DEFINITION *) pDef;
	pParticleSystemDef->dwUpdateFlags = 0;
	RefCountInit(pParticleSystemDef->tRefCount);

	const UPDATE_FLAG_CHART * pChart = &sgtUpdateFlagChart[ 0 ];
	for ( int i = 0; pChart->nOffset != -1; i++, pChart++ )
	{
		BOOL bSetFlag = FALSE;
		if ( pChart->bTriple )
		{
			CInterpolationPath<CFloatPair> * pPair = (CInterpolationPath<CFloatPair> *) ((BYTE *)pDef + pChart->nOffset);
			if ( pPair->GetPointCount() > 1 )
				bSetFlag = TRUE;
		} else {
			CInterpolationPath<CFloatTriple> * pPair = (CInterpolationPath<CFloatTriple> *) ((BYTE *)pDef + pChart->nOffset);
			if ( pPair->GetPointCount() > 1 )
				bSetFlag = TRUE;
		}

		if ( bSetFlag )
			pParticleSystemDef->dwUpdateFlags |= pChart->dwFlag;
	}
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ParticleEffectSetLoad(
	PARTICLE_EFFECT_SET * particle_effect_set,
	const char * pszFolder )
{
	char szParticlePath[MAX_PATH];

	particle_effect_set->nDefaultId = INVALID_ID;
	if (particle_effect_set->szDefault && particle_effect_set->szDefault[0])
	{
		if (particle_effect_set->szDefault[0] == '.')	// sometimes we want something outside of the folder
		{	
			particle_effect_set->nDefaultId = c_sGetParticleDefId(particle_effect_set->szDefault + 1);
		} 
		{
			if ( pszFolder[ 0 ] != 0 )
				PStrPrintf(szParticlePath, MAX_PATH, "%s\\%s", pszFolder, particle_effect_set->szDefault);
			else
				PStrCopy( szParticlePath, MAX_PATH, particle_effect_set->szDefault, MAX_PATH );
			particle_effect_set->nDefaultId = c_sGetParticleDefId(szParticlePath);
		}
	}

	for (unsigned int jj = 0; jj < NUM_DAMAGE_TYPES; ++jj)
	{
		PARTICLE_ELEMENT_EFFECT * particle_element_effect = &particle_effect_set->tElementEffect[jj];
		particle_element_effect->nPerElementId = INVALID_ID;

		if (particle_element_effect->szPerElement && particle_element_effect->szPerElement[0])
		{
			if (particle_element_effect->szPerElement[0] == '.')		// sometimes we want something outside of the folder
			{
				particle_element_effect->nPerElementId = c_sGetParticleDefId((const char *)particle_element_effect->szPerElement + 1);
			} 
			else
			{
				if ( pszFolder[ 0 ] != 0 )
					PStrPrintf(szParticlePath, MAX_PATH, "%s\\%s", pszFolder, particle_element_effect->szPerElement);
				else
					PStrCopy( szParticlePath, MAX_PATH, particle_element_effect->szPerElement, MAX_PATH );
				particle_element_effect->nPerElementId = c_sGetParticleDefId(szParticlePath);
			}
		}
	}			

}

//#endif //!SERVER_VERSION
