//----------------------------------------------------------------------------
// debugbars_priv.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "stdafx.h"

#include "uix_debug.h"
#include "debugbars.h"
#include "excel.h"
#include "excel_private.h"
#include "e_profile.h"
#include "c_particles.h"
#include "e_particle.h"
#include "c_sound_util.h"
#include "appcommontimer.h"
#include "room.h"
#include "e_texture.h"
#include "e_debug.h"
#include "config.h"
#if !ISVERSION(SERVER_VERSION)
#include "debugbars_priv.h"

#define FPS_BAR(nBar)	SET_BAR( nBar,	"FPS",	(int)AppCommonGetDrawFrameRate(),	120,	45,		10 );



// TO ADD NEW BARS SET:
//   Make a callback (see sDebugBarUpdateGeneral for example)
//   Make it accessible from right about here (prototype or include)
//   Add it to debugbars_def.h (should be obvious)
//   All done -- /bars <stringname> to show



// PROTOTYPES FOR RENDER FUNCTIONS
static void sDebugBarUpdateGeneral();
static void sDebugBarUpdateParticle();
static void sDebugBarUpdateBackground();
static void sDebugBarUpdateTiming();

DEBUG_BAR_SET_DEFINITION gtDebugBarsSets[ NUM_DEBUG_BARS_SETS ] =
{
#include "debugbars_def.h"
};

DEBUG_BARS_SET gnCurrentBarsSet = DEBUG_BARS_NONE;


//----------------------------------------------------------------------------
// debugbars.txt
EXCEL_STRUCT_DEF(DEBUG_BAR_SET_DEFINITION)
	EF_STRING(	"Name",										szName																				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_DEBUG_BARS, DEBUG_BAR_SET_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "debugbars") 
	EXCEL_TABLE_POST_CREATE(gtDebugBarsSets)
EXCEL_TABLE_END

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_SetUIBarsSet( int nBarsSet )
{
	ASSERT_RETURN( nBarsSet >= 0 && nBarsSet < NUM_DEBUG_BARS_SETS );
	gnCurrentBarsSet = (DEBUG_BARS_SET)nBarsSet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
	
void c_UpdateUIBars()
{
	if ( gnCurrentBarsSet == DEBUG_BARS_NONE )
		return;
	ASSERT_RETURN( gnCurrentBarsSet >= 0 && gnCurrentBarsSet < NUM_DEBUG_BARS_SETS );
	ASSERT_RETURN( gtDebugBarsSets[ gnCurrentBarsSet ].pfnUpdate );

	gtDebugBarsSets[ gnCurrentBarsSet ].pfnUpdate();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void SetUIBarCallback(
	int nBarIndex,
	const char * szName,
	int nValue,
	int nMax,
	int nGood,
	int nBad )
{
#if !ISVERSION(SERVER_VERSION)
	e_DebugSetBar( nBarIndex, szName, nValue, nMax, nGood, nBad );
#endif// !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugBarUpdateGeneral()
{
	int nRoomsDrawn = c_RoomGetDrawCount();
	int nBatches = e_MetricsGetTotalBatchCount();
	int nPolygons = e_MetricsGetTotalPolygonCount();
	int nParticlesDrawn   = e_ParticlesGetDrawnTotal();
	int nNumSounds = c_SoundGetNumActive();
	int nTextureCount = 0, nTextureMB = 0;
	V( e_AddTextureUsedStats( -1, -1, nTextureCount, nTextureMB ) );
	nTextureMB /= 1024 * 1024;	// convert bytes to MB

	//		 bar	name				value					max			good	bad
	FPS_BAR( 0 );
	SET_BAR( 1,		"Batches",			nBatches,				800,		150,	450 );
	SET_BAR( 2,		"Rooms Draw",		nRoomsDrawn,			150,		10,		40 );
	SET_BAR( 3,		"Particles Draw",	nParticlesDrawn,		5000,		1500,	4000 );
	SET_BAR( 4,		"Sounds",			nNumSounds,				1000,		30,		200 );
	SET_BAR( 5,		"Textures Used",	nTextureCount,			2000,		30,		800 );
	SET_BAR( 6,		"Texture Used MB",	nTextureMB,				512,		50,		180	);
	SET_BAR( 7,		"Polygons",			nPolygons,				2000000,	500000,	1000000 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugBarUpdateParticle()
{
	int nParticlesDrawn   = e_ParticlesGetDrawnTotal();
	int nParticlesUpdated = c_ParticlesGetUpdateTotal();
	float fParticleWorldArea  = e_ParticlesGetTotalWorldArea();
	float fParticleScreenArea = e_ParticlesGetTotalScreenArea();
	int nTextureCount = 0, nTextureKB = 0;
	V( e_AddTextureUsedStats( TEXTURE_GROUP_PARTICLE, -1, nTextureCount, nTextureKB ) );
	nTextureKB /= 1024;	// convert bytes to KB
	int nParticleBatches, nSystemsVisible;
	e_GetBatchStats(
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		&nParticleBatches,
		&nSystemsVisible );


	//		 bar	name						value						max		good	bad
	//FPS_BAR( 0 );
	SET_BAR( 0,		"Particles Drawn",			nParticlesDrawn,			5000,	1500,	4000 );
	SET_BAR( 1,		"Particles Updated",		nParticlesUpdated,			2000,	50,		250 );
	SET_BAR( 2,		"Particle Batches",			nParticleBatches,			100,	5,		40 );
	SET_BAR( 3,		"Systems Visible",			nSystemsVisible,			300,	20,		150 );
	SET_BAR( 4,		"World Area",				(int)fParticleWorldArea,	2000,	50,		500 );
	SET_BAR( 5,		"Screen Area",				(int)fParticleScreenArea,	50,		2,		10 );
	SET_BAR( 6,		"Texture Count",			nTextureCount,				200,	10,		50 );
	SET_BAR( 7,		"Texture KB",				nTextureKB,					10240,	500,	4096 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugBarUpdateBackground()
{
	int nRoomsDrawn = c_RoomGetDrawCount();
	int nRoomsVisible, nRoomBatches, nRoomsInFrustum, nShadowBatches;
	e_GetBatchStats(
		NULL,
		NULL,
		NULL,
		&nRoomsVisible,
		//&nRoomBatches,
		NULL,
		NULL,
		NULL,
		NULL,
		&nShadowBatches );
	nRoomBatches = gtBatchInfo.nBatches[ METRICS_GROUP_BACKGROUND ];
	nRoomsInFrustum = gtBatchInfo.nRoomsInFrustum;
	int nTextureCount = 0, nTextureMB = 0;
	V( e_AddTextureUsedStats( TEXTURE_GROUP_BACKGROUND, -1, nTextureCount, nTextureMB ) );
	nTextureMB /= 1024 * 1024;	// convert bytes to MB
	int nPolygons = e_MetricsGetPolygonCount( METRICS_GROUP_BACKGROUND );

	//		 bar	name					value					max			good	bad
	FPS_BAR( 0 );
	SET_BAR( 1,		"Room Batches",			nRoomBatches,			800,		50,		350 );
	SET_BAR( 2,		"Rooms Drawn",			nRoomsDrawn,			150,		10,		40 );
	SET_BAR( 3,		"Rooms Visible",		nRoomsVisible,			500,		50,		150 );
	SET_BAR( 4,		"Rooms In Frustum",		nRoomsInFrustum,		1000,		1000,	1001 );
	SET_BAR( 5,		"BG Texture Count",		nTextureCount,			400,		40,		200 );
	SET_BAR( 6,		"BG Texture Used MB",	nTextureMB,				200,		40,		100 );
	SET_BAR( 7,		"Polygons",				nPolygons,				2000000,	250000,	500000);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#include "chb_timing.h"
static void sDebugBarUpdateTiming()
{
	int nCPUTimeMS = static_cast<int>(CHB_TimingGetInstantaneous(CHB_TIMING_CPU) * 1000 + 0.5f);
	int nSyncTimeMS = static_cast<int>(CHB_TimingGetInstantaneous(CHB_TIMING_SYNC) * 1000 + 0.5f);

	//		 bar	name					value					max			good	bad
//	FPS_BAR( 0 );
	SET_BAR( 1,		"CPU",					nCPUTimeMS,				100,		20,		50 );
	SET_BAR( 2,		"GPU",					0 /*!!!*/,				100,		20,		50 );
	SET_BAR( 3,		"Sync",					nSyncTimeMS,			100,		20,		50 );
}

#endif //!ISVERSION(SERVER_VERSION)
