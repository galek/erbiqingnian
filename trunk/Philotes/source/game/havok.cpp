
//-------------------------------------------------------------------------------------------------
// Prime 
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// INCLUDE
//-------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "appcommon.h"
#ifdef HAVOK_ENABLED
#include "prime.h"
#include "dungeon.h"
#include "units.h" // also includes game.h
#include "havok.h"
#include "boundingbox.h"
#include "e_havokfx.h"
#include "c_ragdoll.h"
#include "globalindex.h"
#include "excel.h"
#include "level.h"
#include "filepaths.h"
#include "unit_priv.h"

#include "e_main.h"
#include "e_model.h"

#include <hkbase/stream/hkOstream.h>

// AE: If we decide to use our own memory manager, then we must used 16-byte
// aligned allocations. Havok documentation says that this is required.
#define HAVOK_USES_OUR_MEMORY_MANAGER
//#define FROZEN
#define POSITION_LIMIT 100000.0f

//#define HAVOK_KEYCODE(code, value)					\
//	extern const char*		  HK_KEYCODE  = code;	\
//	extern const unsigned int HK_KEYVALUE = value;	

#if defined(HK_PLATFORM_GC)
#	define KEYCODE_ATTRIBUES __attribute__((section(".sdata")))
#else
#	define KEYCODE_ATTRIBUES
#endif
#define HAVOK_KEYCODE(code, value)					\
	extern const char         HK_KEYCODE[] KEYCODE_ATTRIBUES = code;	\
	extern const unsigned int HK_KEYVALUE  KEYCODE_ATTRIBUES = value

//HAVOK_KEYCODE("340704-PC-BJLAKOBEFLAG", 0x0)
//HAVOK_KEYCODE("FLAGSHIP_ONE", 0x43e8bd5)
//HAVOK_KEYCODE("31MAR05.PhAn.Flagship_MonsterMash", 0xac09cd3a)
//HAVOK_KEYCODE("02APR05.PhAn.Flagship_HellgateLondon", 0xac0a903a)
HAVOK_KEYCODE("CLIENT.PhAn.Flagship_Namco_HellgateLondon", 0x7603e00b);

#define HK_CLASSES_FILE <hkserialize/classlist/hkCompleteClasses.h>
#include <hkserialize/util/hkBuiltinTypeRegistry.cxx>

//#define COLFILTER_RAGDOLL_LAYER		1 Definition the same moved to c_ragdoll.h
//#define COLFILTER_RAGDOLL_SYSTEM	1
#define COLFILTER_UNIT_LAYER		3
#define COLFILTER_UNIT_SYSTEM		1
#define COLFILTER_BACKGROUND_LAYER  2
#define COLFILTER_BACKGROUND_SYSTEM 2
#define COLFILTER_RAYCAST_LAYER		4
#define COLFILTER_RAYCAST_SYSTEM	4
//#define COLFILTER_POSE_PHANTOM_LAYER	5 // moved to havok.h where c_appearance.cpp can see it
//#define COLFILTER_POSE_PHANTOM_SYSTEM	5

//#define COLFILTER_RAGDOLL_PENETRATION_RECOVERY_LAYER	6 //lives in c_ragdoll.h
//#define COLFILTER_RAGDOLL_PENETRATION_DETECTION_LAYER	7 //lives in c_ragdoll.h

#define HAVOK_BIN_EXTENSION "hbin"

BOOL sgbHavokOpen = FALSE;
RAND sgHavokRand;

//#ifdef HAMMER
static hkWorld * sgpHammerWorld = NULL;
static int sgnHammerFxWorld = INVALID_ID;
static hkRigidBody * pFloor = NULL;
//#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if USE_HAVOK_4
// Generate a custom list to trim memory requirements
#define HK_COMPAT_FILE <hkcompat/hkCompatVersions.h>
#include <hkcompat/hkCompat_All.cxx>
#else
extern const hkVersionRegistry::Updater hkVersionUpdater_Havok300_Havok310;
extern const hkVersionRegistry::Updater hkVersionUpdater_Havok310_Havok320;
extern const hkVersionRegistry::Updater hkVersionUpdater_Havok320_Havok330a2;
extern const hkVersionRegistry::Updater hkVersionUpdater_Havok330a2_Havok330b1;
extern const hkVersionRegistry::Updater hkVersionUpdater_Havok330b1_Havok330b2;

const hkVersionRegistry::Updater* hkVersionRegistry::StaticLinkedUpdaters[] =
{
	&hkVersionUpdater_Havok300_Havok310,
	&hkVersionUpdater_Havok310_Havok320,
	&hkVersionUpdater_Havok320_Havok330a2,
	&hkVersionUpdater_Havok330a2_Havok330b1,
	&hkVersionUpdater_Havok330b1_Havok330b2,
	HK_NULL
};
#endif

//-------------------------------------------------------------------------------------------------
// Visualdebugger
//-------------------------------------------------------------------------------------------------
hkVisualDebugger* sgVisualDebugger = HK_NULL;
hkPhysicsContext* sgVisualDebuggerContext = HK_NULL;

//-----------------------------------------------------------------------------
// Name: HavokStepVisualDebugger()
// Desc: Initializes The Visual Debugger - Not vital, but allows user to "view"
//			this application running through the Visual Debugger Client.
// Only turn this on when you are using the Debugger - it sets the app up as a server and we can't ship that.
//-----------------------------------------------------------------------------
//#define USE_VISUAL_DEBUGGER
void c_HavokSetupVisualDebugger(
	hkWorld* physicsWorld )
{
#ifdef USE_VISUAL_DEBUGGER
	if ( sgVisualDebugger )
		return;
	// Setup the visual debugger      
	sgVisualDebuggerContext = new hkPhysicsContext;
	hkPhysicsContext::registerAllPhysicsProcesses(); // all the physics viewers
	sgVisualDebuggerContext->addWorld(physicsWorld); // add the physics world so the viewers can see it

	hkArray<hkProcessContext*> contexts;
	contexts.pushBack(sgVisualDebuggerContext);

	sgVisualDebugger = new hkVisualDebugger(contexts); 
	sgVisualDebugger->serve(); 

/*	// Allocate memory for internal profiling information
	// You can discard this if you do not want Havok profiling information
	hkMonitorStream::getInstance().resize( 500 * 1024 );	// 500K for timer info
	hkMonitorStream::getInstance().reset();

	// Calibrate the monitors 
	hkMonitorBank::getInstance().examineMonitorCapture( 0, 1.0f / hkStopwatch::getTicksPerSecond());

//	hkFinishLoadedObjectRegistry registry;
//	hkRegisterHavokClasses::registerAll(registry);
//	sgVisualDebugger = hkVisualDebugger::create(physicsWorld, HK_NULL, registry);
//
//	sgVisualDebugger->serve(); 
//
//	// Allocate memory for internal profiling information
//	// You can discard this if you do not want Havok profiling information
//	hkMonitorStream::getInstance().resize( 2000000 );	// 2 meg for timer info
//	hkMonitorStream::getInstance().reset();
//
//	// Calibrate the monitors 
//	hkMonitorBank::getInstance().examineMonitorCapture( 0, 1.0f / hkStopwatch::getTicksPerSecond());*/
#endif
}

//-----------------------------------------------------------------------------
// Name: StepVisualDebugger()
// Desc: Steps the Visual Debugger. This allows you to visually confirm that
//			Havok is "in sync" with the DirectX display.
//-----------------------------------------------------------------------------

void c_HavokStepVisualDebugger()
{
#if !ISVERSION(SERVER_VERSION)
#ifdef USE_VISUAL_DEBUGGER
	if (!sgVisualDebugger)
		return;

	// Update camera with game view
	{
		VECTOR vCamPos, vCamTo;
		e_GetViewPosition( &vCamPos );
		e_GetViewFocusPosition( &vCamTo );
		HK_UPDATE_CAMERA( hkVector4( vCamPos.fX, vCamPos.fY, vCamPos.fZ ), hkVector4( vCamTo.fX, vCamTo.fY, vCamTo.fZ ), hkVector4(0,0,1), 0.1f, 1000.0f, 60.0f, "Player" );
	}
	
	// Step the debugger
	sgVisualDebugger->step();

	// Reset internal profiling info for next frame
////	hkMonitorStream::getInstance().reset();
//	hkMonitorBank::getInstance().beginNewFrameAllMonitors();

	// On PS2 we must restart the performance counters every 7 seconds
	// otherwise we will get an exception which cannot be caught by the user
//	// Step the debugger
//	sgVisualDebugger->step();
//
//	// Reset internal profiling info for next frame
//	hkMonitorStream::getInstance().reset();
//	hkMonitorBank::getInstance().beginNewFrameAllMonitors();
#endif
#endif //!SERVER_VERSION
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_PrintHavokMemoryStats()
{
//#if !ISVERSION(SERVER_VERSION)
//	OS_PATH_CHAR szLogOs[ MAX_PATH ];
//	PStrPrintf( szLogOs, _countof(szLogOs), OS_PATH_TEXT("%s%s"), LogGetRootPath(), OS_PATH_TEXT("havokmem.txt") );
//	char szLog[ MAX_PATH ];
//	PStrCvt(szLog, szLogOs, _countof(szLog));
//	hkOstream tFile( szLog );
//	hkMemory::getInstance().printStatistics( &tFile );
//#endif //!SERVER_VERSION
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void myPrintf( const char* str, void* args )
{
	OutputDebugString( str );
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BYTE * sgpHavokStackBuffer = NULL;
// CHB 2007.03.14
//static hkThreadMemory* threadMemoryManager = NULL;		// BSP: apparently this pointer needs to be kept around because we need to free it later

//class hkMyThreadMemory : public hkThreadMemory
//{
//public:
//	hkMyThreadMemory( hkMemory* mainMemoryManager )
//		: hkThreadMemory( mainMemoryManager )
//	{}
//
//protected:
//	virtual void* onStackOverflow(int nbytes)
//	{
//		return MALLOC( NULL, nbytes );
//	}
//
//	/// Override this function to hook into the frame based allocation.
//	/// Called when the current stack becomes empty. See also onStackOverflow.
//	virtual void onStackUnderflow(void* p)
//	{
//		return FREE( NULL, p );
//	}
//};
#define HAVOK_REQUIRED_ALIGNMENT			16

class hkMyMemory : public hkMemory
{
public:
	hkMyMemory() :
		hkMemory()
	{ 
	}
	
	virtual void * allocate(int nbytes, HK_MEMORY_CLASS cl)
	{
		MEMORYPOOL * pool = MemoryGetThreadPool();
		void * p = ALIGNED_MALLOC(pool, nbytes, HAVOK_REQUIRED_ALIGNMENT);
		return p;
	}
	virtual void deallocate(void* p)
	{
		if (p)
		{
			MEMORYPOOL * pool = MemoryGetThreadPool();
			FREE(pool, p);
		}
	}
	virtual void * allocateChunk(int nbytes, HK_MEMORY_CLASS cl)
	{
		MEMORYPOOL * pool = MemoryGetThreadPool();
		void * p = ALIGNED_MALLOC(pool, nbytes, HAVOK_REQUIRED_ALIGNMENT);
		return p;
	}
	virtual void deallocateChunk(void* p, int nbytes, HK_MEMORY_CLASS )
	{
		if (p)
		{
			MEMORYPOOL * pool = MemoryGetThreadPool();
			FREE(pool, p);
		}
	}
	virtual void * alignedAllocate(int alignment, int nbytes, HK_MEMORY_CLASS cl)
	{
		MEMORYPOOL * pool = MemoryGetThreadPool();
		void * p = ALIGNED_MALLOC(pool, nbytes, MAX<int>(HAVOK_REQUIRED_ALIGNMENT, alignment));
		return p;
	}
	virtual void alignedDeallocate(void* p)
	{
		if (p)
		{
			MEMORYPOOL * pool = MemoryGetThreadPool();
			FREE(pool, p);
		}
	}

	/// Allocates a block of size nbytes of fast runtime memory
	/// If a matching block is available this function will return it.
	/// If no matching block could be found this function will allocate a new block and issue a warning,
	/// add it to its managed array of blocks and return a pointer to it.
	///
	/// General info on fast runtime blocks:
	/// Regular stack memory is not working properly in a multi-threaded environment. To cater to the need for
	/// fast memory allocations during simulation, the concept of dedicated runtime memory blocks (working
	/// as a stack memory replacement) has been devised.
	/// Runtime block functions provide access to very fast memory allocation by internally
	/// storing an array of preallocated memory blocks that are re-used on the fly as soon
	/// as they have been declared available again by explicitly deallocating them. New
	/// allocations are only performed if no available memory block of matching size could
	/// be found in the internal array of managed block.
	/// Internally these runtime blocks are only used inside the simulation (i.e. during a call to
	/// stepDeltaTime() or stepProcessMt()) and can therefore be used freely outside by the user.
	virtual void * allocateRuntimeBlock(int nbytes, HK_MEMORY_CLASS cl)
	{
		MEMORYPOOL * pool = MemoryGetThreadPool();
		void * p = ALIGNED_MALLOC(pool, nbytes, HAVOK_REQUIRED_ALIGNMENT);
		return p;
	}

	/// Deallocates a block of fast runtime memory that has been allocated by allocateRuntimeBlock()
	/// For more information on runtime blocks see allocateRuntimeBlock()
	virtual void deallocateRuntimeBlock(void* p, int nbytes, HK_MEMORY_CLASS cl)
	{
		if (p)
		{
			MEMORYPOOL * pool = MemoryGetThreadPool();
			FREE(pool, p);
		}
	}

	/// Preallocates a fast runtime memory block of size nbytes
	/// For more information on runtime blocks see allocateRuntimeBlock()
	virtual void preAllocateRuntimeBlock(int nbytes, HK_MEMORY_CLASS cl)
	{
	}

	/// This allows the user to provide his own block(s) of memory for fast runtime memory
	/// Note that memory blocks that have been provided by the user will not be deallocated by a call to
	/// freeRuntimeBlocks(). The user has to take care of deallocation himself.
	/// For more information on runtime blocks see allocateRuntimeBlock()
	virtual void provideRuntimeBlock(void*, int nbytes, HK_MEMORY_CLASS cl)
	{}

	/// Deallocate all fast runtime memory blocks that are still allocated and that have NOT been provided
	/// by the user (by using provideRuntimeBlock())
	/// For more information on runtime blocks see allocateRuntimeBlock()
	virtual void freeRuntimeBlocks()
	{}



	/// Prints some statistics to the specified console.
	virtual void printStatistics(hkOstream* c) {}

	/// Get a simple synopsis of memory usage.
	virtual void getStatSynopsis(hkMemoryStatistics* reportOut) {}

};

class hkMyThreadMemory : public hkThreadMemory
{

	public:
	
		hkMyThreadMemory(hkMemory* mainMemoryManager, int maxNumElemsOnFreeList = 16) :
			hkThreadMemory(mainMemoryManager, maxNumElemsOnFreeList)
		{
		
		}
		
	public:
	
		virtual void* onStackOverflow(int nbytesin)
		{
			HK_ASSERT(0x3fffb5df,  (nbytesin & 0xf) == 0);

			// Assert if the stack was empty
			HK_ASSERT2(0x2bebba62, ( m_stack.m_current != HK_NULL ),
								"The system needs to use a local stack for simulation.\n" \
								"You need to call hkThreadMemory::setStackArea() to set up a buffer "\
								"for this stack. Failure to do this results in lots of heap "\
								"allocations which can lead to large slowdowns." );

			ASSERTV(nbytesin == 0, "havoc stack size needs tuning.  increase size by: ~%d - Didier", nbytesin);

			const int smallestalloc = 4096;
			const int extrasize     = 1024;

			const int increasedSize = nbytesin + extrasize;
			const int size = (increasedSize > smallestalloc ) ? increasedSize : smallestalloc;
			
			//
			// NOTE: If you are overriding this function, copy and paste this implementation,
			// and simply replace the NEXT line with your own stack allocation function
			//
			char* newmem = static_cast<char*>(MALLOC(g_ScratchAllocator, size+hkSizeOf(Stack)));

			// save old stack
			*reinterpret_cast<Stack*>(newmem) = m_stack;
			// update current
			m_stack.m_base = newmem + hkSizeOf(Stack);
			m_stack.m_current = m_stack.m_base + nbytesin;
			m_stack.m_end = m_stack.m_base + size;
			m_stack.m_prev = reinterpret_cast<Stack*>(newmem);
			return m_stack.m_base;
		}

		virtual void onStackUnderflow(void* ptr)
		{
			char* chunkAddr = m_stack.m_base - hkSizeOf(Stack);
			//int chunkSize = int(m_stack.m_end - m_stack.m_base) + hkSizeOf(Stack);
			HK_ASSERT(0x4398d3a1,  (hkUlong(chunkAddr) & 0xf) == 0);
			// restore previous stack
			m_stack = *reinterpret_cast<Stack*>(chunkAddr);
			//
			// NOTE: If you are overriding this function, copy and paste this implementation,
			// and simply replace the NEXT line with your own stack deallocation function.
			//
			FREE(g_ScratchAllocator, chunkAddr);
		}	

};


class hkMyError : public hkDefaultError
{
public:
	hkMyError( hkErrorReportFunction errorReportFunction, void* errorReportObject = HK_NULL )
		: hkDefaultError(errorReportFunction, errorReportObject)
	{
	}

	int message(hkError::Message msg, int id, const char* description, const char* file, int line)
	{
		if( id == -1 && m_sectionIds.getSize() )
		{
			id = m_sectionIds.back();
		}

		if (!isEnabled(id))
		{
			return 0;
		}

		const char* what = "";

		hkBool stackTrace = false;
		switch( msg )
		{
		case MESSAGE_REPORT:
			what = "Report";
			break;
		case MESSAGE_WARNING:
			what = "Warning";
			break;
		case MESSAGE_ASSERT:
			what = "Assert";
			stackTrace = true;
			break;
		case MESSAGE_ERROR:
			what = "Error";
			stackTrace = true;
			break;
		}
		//showMessage(what, id, description, file, line, stackTrace);
#if !ISVERSION(RETAIL_VERSION)
		bool stopInDebugger = (msg == MESSAGE_ASSERT || msg == MESSAGE_ERROR);
		if( stopInDebugger )
		{
			DoAssert( what, description, file, line, __FUNCSIG__);
		}
		return FALSE;
#else
		return FALSE;
#endif
	}
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static hkMemory * sgMemoryManager = NULL;

void HavokSystemInit()
{
	// TODO: add our 
	//hkMemory *memoryManager;
	ASSERT_RETURN( sgMemoryManager == NULL );
#ifdef HAVOK_USES_OUR_MEMORY_MANAGER
	sgMemoryManager = new hkMyMemory();
	//threadMemoryManager = new hkMyThreadMemory( memoryManager );
	//hkThreadMemory* threadMemoryManager = MEMORYPOOL_NEW(NULL)hkThreadMemory( sgMemoryManager );
#else
	sgMemoryManager = new hkPoolMemory();
#endif
	hkMyThreadMemory* threadMemoryManager = new hkMyThreadMemory( sgMemoryManager );

	hkBaseSystem::init( sgMemoryManager, threadMemoryManager, (hkErrorReportFunction)(myPrintf) );
	threadMemoryManager->removeReference();	// CHB 2007.03.14

	int stackSize = 512 * KILO + 2 * MEGA;
	
#if USE_MEMORY_MANAGER
	sgpHavokStackBuffer = (BYTE *) MALLOCZ( g_StaticAllocator, stackSize );
#else	
	sgpHavokStackBuffer = (BYTE *) MALLOCZ( NULL, stackSize );
#endif
	
	threadMemoryManager->setStackArea( sgpHavokStackBuffer, stackSize );

	hkError::replaceInstance( new hkMyError((hkErrorReportFunction)(myPrintf)) );

	c_RagdollSystemInit();

	RandInit(sgHavokRand);

	hkError::getInstance().setEnabled(0xf3768206, false);

	e_HavokFXSystemInit();

	sgbHavokOpen = TRUE;

	/*hkThreadMemory::*///threadMemoryManager->setStackArea(NULL, 16777216); //Multimopps overflowing the stack.
	//This doesn't work.  I believe I have to allocate it somewhere first.
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokSystemClose()
{
	c_RagdollSystemClose();

	if ( sgVisualDebugger )
		sgVisualDebugger->removeReference();
	if ( sgVisualDebuggerContext )
		sgVisualDebuggerContext->removeReference();
	
#if USE_MEMORY_MANAGER
	FREE( g_StaticAllocator, sgpHavokStackBuffer );
#else		
	FREE( NULL, sgpHavokStackBuffer );
#endif
	
	hkBaseSystem::quit();

	e_HavokFXSystemClose();

	// CHB 2007.03.14
	//if (threadMemoryManager)
	//	delete threadMemoryManager;

	sgbHavokOpen = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokThreadInit(
	HAVOK_THREAD_DATA & tThreadData )
{
	ASSERT_RETURN( tThreadData.pMemoryPool );
	tThreadData.pThreadMemory = new hkThreadMemory( sgMemoryManager );

	hkBaseSystem::initThread( tThreadData.pThreadMemory );

	int stackSize = 0x1800000;
	tThreadData.pStackBuffer = (BYTE *) MALLOCZ( tThreadData.pMemoryPool, stackSize );
	tThreadData.pThreadMemory->setStackArea( tThreadData.pStackBuffer, stackSize );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokThreadClose(
	HAVOK_THREAD_DATA & tThreadData )
{
	ASSERT_RETURN( tThreadData.pMemoryPool );

	tThreadData.pThreadMemory->setStackArea(0, 0);
	FREE( tThreadData.pMemoryPool, tThreadData.pStackBuffer );
	tThreadData.pThreadMemory->removeReference();

	hkBaseSystem::clearThreadResources();

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokInitWorld(
	hkWorld** ppHavokWorld,
	BOOL bIsClient,
	BOOL bUseHavokFX,
	const BOUNDING_BOX * pBoundingBox,
	BOOL bSetUpDebugger )
{
	hkWorldCinfo info;
#ifndef FROZEN
	info.m_gravity = hkVector4( 0,0,-9.800723f);
#else
	info.m_gravity = hkVector4( 0,0,0);
#endif
	//info.m_collisionTolerance = 0.0f; 
	if (pBoundingBox)
	{
		// This is how the documentation says to do it:
		//info.m_broadPhaseWorldMin.set(vMin->fX, vMin->fY, vMin->fZ);
		//info.m_broadPhaseWorldMax.set(vMax->fX, vMax->fY, vMax->fZ);

		// However, the documentation is WRONG
		info.m_broadPhaseWorldAabb.m_min.set(pBoundingBox->vMin.fX, pBoundingBox->vMin.fY, pBoundingBox->vMin.fZ);
		info.m_broadPhaseWorldAabb.m_max.set(pBoundingBox->vMax.fX, pBoundingBox->vMax.fY, pBoundingBox->vMax.fZ);

		info.m_broadPhaseWorldAabb.m_min.sub4( hkVector4( 1.0f, 1.0f, 20.0f, 0.0f ) );
		info.m_broadPhaseWorldAabb.m_max.add4( hkVector4( 1.0f, 1.0f, 30.0f, 0.0f ) );
	}
	else
	{
		info.setBroadPhaseWorldSize( 1000.0f );
	}
	info.m_broadPhaseBorderBehaviour = hkWorldCinfo::BROADPHASE_BORDER_DO_NOTHING;
	info.setupSolverInfo( hkWorldCinfo::SOLVER_TYPE_4ITERS_MEDIUM );
	//info.m_iterativeLinearCastEarlyOutDistance = info.m_collisionTolerance / 10.0f;

#if USE_HAVOK_4
	// TODO: fixme
	hkError::getInstance().setEnabled( 0x38f1276d, false ); // ray is inside broadphase
	hkError::getInstance().setEnabled( 0x28487ddd, false ); // parameter aliasing - should be disabled anyway
#endif

	*ppHavokWorld = new hkWorld( info );
	hkAgentRegisterUtil::registerAllAgents( (*ppHavokWorld)->getCollisionDispatcher() );

	////////////
	//setup collision filtering
	{
		/////////
		//setup group filter & layers

		hkGroupFilter * pGroupFilter = new hkGroupFilter();
		(*ppHavokWorld)->setCollisionFilter( pGroupFilter );
		
		
		
		pGroupFilter->disableCollisionsBetween( COLFILTER_UNIT_LAYER, COLFILTER_RAGDOLL_LAYER );


		
		//CC@HAVOK: turn off collisions between recovering ragdoll bodies and the world.
		pGroupFilter->disableCollisionsBetween( COLFILTER_BACKGROUND_LAYER, COLFILTER_RAGDOLL_PENETRATION_RECOVERY_LAYER );
		//CC@HAVOK: disables collisions between the penetration test check and the ragdoll bodies
		pGroupFilter->disableCollisionsBetween( COLFILTER_RAGDOLL_PENETRATION_DETECTION_LAYER, COLFILTER_RAGDOLL_LAYER );
		//CC@HAVOK: disables collisions between the penetration test check and covering ragdoll bodies
		pGroupFilter->disableCollisionsBetween( COLFILTER_RAGDOLL_PENETRATION_DETECTION_LAYER, COLFILTER_RAGDOLL_PENETRATION_RECOVERY_LAYER );


#ifdef FROZEN
		pGroupFilter->disableCollisionsBetween( COLFILTER_BACKGROUND_LAYER, COLFILTER_RAGDOLL_LAYER );
#endif	
		pGroupFilter->disableCollisionsBetween( COLFILTER_RAYCAST_LAYER, COLFILTER_UNIT_LAYER );
		pGroupFilter->disableCollisionsBetween( COLFILTER_RAYCAST_LAYER, COLFILTER_RAGDOLL_LAYER );
		pGroupFilter->disableCollisionsBetween( COLFILTER_POSE_PHANTOM_LAYER, COLFILTER_BACKGROUND_LAYER );
		pGroupFilter->disableCollisionsBetween( COLFILTER_POSE_PHANTOM_LAYER, COLFILTER_RAYCAST_LAYER );
		pGroupFilter->disableCollisionsBetween( COLFILTER_POSE_PHANTOM_LAYER, COLFILTER_UNIT_LAYER );
		pGroupFilter->disableCollisionsBetween( COLFILTER_POSE_PHANTOM_LAYER, COLFILTER_RAGDOLL_LAYER );


		////////
		// setup filtering between adjacent ragdoll bones,
		// using the group filter as the first check.
		hkConstrainedSystemFilter* pConstrainedSystemFilter = new hkConstrainedSystemFilter(pGroupFilter);
		pGroupFilter->removeReference();

		(*ppHavokWorld)->setCollisionFilter( pConstrainedSystemFilter, true, HK_UPDATE_FILTER_ON_WORLD_FULL_CHECK, HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS);
		pConstrainedSystemFilter->removeReference();
	}


	if ( bSetUpDebugger )
		c_HavokSetupVisualDebugger( *ppHavokWorld );

	if ( bIsClient && bUseHavokFX )
	{
		BOUNDING_BOX tHavokFXBox;
		float fFloorBottom;
		BOOL bAddFloor = FALSE;
		if ( ! pBoundingBox )
		{
			if ( AppIsHammer() )
			{
				tHavokFXBox.vMin = VECTOR( -100.0f, -100.0f, -2.0f );
				tHavokFXBox.vMax = VECTOR(  100.0f,  100.0f, 100.0f );
				bAddFloor = TRUE;
			} else {
				tHavokFXBox.vMin = VECTOR( -500.0f, -500.0f, -100.0f );
				tHavokFXBox.vMax = VECTOR(  500.0f,  500.0f,  100.0f );
			}
			fFloorBottom = 0.0f;
		} else {
			tHavokFXBox.vMin = pBoundingBox->vMin;
			tHavokFXBox.vMax = pBoundingBox->vMax;

			fFloorBottom = tHavokFXBox.vMin.fZ;
			tHavokFXBox.vMin -= 100.0f;
			tHavokFXBox.vMax += 100.0f;
		}

		e_HavokFXInitWorld( *ppHavokWorld, tHavokFXBox, bAddFloor, fFloorBottom );
	}

	if ( AppIsHammer() )
	{
		HavokAddFloor( AppGetCltGame(), *ppHavokWorld );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokInit( 
	GAME * pGame )
{
//	ASSERT( sgbHavokOpen == FALSE );

//#ifdef HAMMER
	if (AppIsHammer() && !IS_SERVER(pGame))
	{
		HavokInitWorld( &sgpHammerWorld, IS_CLIENT(pGame), TRUE, NULL, TRUE );
	}
//#endif

	if (IS_CLIENT(pGame))
	{
		c_RagdollGameInit( pGame );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
hkWorld* HavokGetWorld( int nModelId, BOOL* pbUseHavokFX )
{
	hkWorld* pWorld = HK_NULL;
	if( !AppIsHammer() )
	{
		// the roomID is stored as userdata in the model, now
		int nRoomID = (int)e_ModelGetRoomID( nModelId );
		if ( nRoomID != INVALID_ID )
		{
			ROOM* pRoom = RoomGetByID( AppGetCltGame(), nRoomID );
			if ( pRoom )
			{
				LEVEL* pLevel = RoomGetLevel( pRoom );
				if( pLevel )
				{
					pWorld = pLevel->m_pHavokWorld;
					if( pbUseHavokFX )
					{
						const DRLG_DEFINITION* pDRLGDef = RoomGetDRLGDef( pRoom );
						if ( pDRLGDef && pDRLGDef->bCanUseHavokFX )
							*pbUseHavokFX = TRUE;
						else
							*pbUseHavokFX = FALSE;
					}
				}
			}
		}
	}
	else
	{
		pWorld = HavokGetHammerWorld();
		if ( pbUseHavokFX )
			*pbUseHavokFX = TRUE;
	}

	return pWorld;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokCloseWorld(
	hkWorld** pHavokWorld )
{
	if ( *pHavokWorld )
	{
		// CHB 2007.03.14
		if ( AppIsHammer() )
		{
			HavokRemoveFloor( AppGetCltGame(), *pHavokWorld );
		}

		(*pHavokWorld)->removeReference();
		*pHavokWorld = NULL;
	}

	e_HavokFXCloseWorld();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokClose( 
	GAME * pGame )
{
	if ( IS_CLIENT( pGame ) )
		c_RagdollGameClose( pGame );

// #ifdef HAMMER
	if (AppIsHammer() && !IS_SERVER(pGame))
	{
		HavokCloseWorld( &sgpHammerWorld );
		sgpHammerWorld = NULL;
		sgnHammerFxWorld = INVALID_ID;
	}
// #endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void s_HavokUpdate( 
	GAME * pGame,
	float fDelta )
{
	ASSERT_RETURN( IS_SERVER(pGame) );

	pGame->fHavokUpdateTime += fDelta;

	int nUpdateCount = 0;
	while ( pGame->fHavokUpdateTime > 0.0f )
	{
		pGame->fHavokUpdateTime -= MAYHEM_UPDATE_STEP;
		nUpdateCount++;
	}

	if ( nUpdateCount )
	{
		int nLevelCount = DungeonGetNumLevels( pGame );
		for ( int i = 0; i < nLevelCount; i++ )
		{
			LEVEL * pLevel = LevelGetByID( pGame, (LEVELID)i );
			if ( ! pLevel )
			{
				continue;
			}

			if (pLevel->m_pHavokWorld)
			{
				for ( int j = 0; j < nUpdateCount; j++ )
				{
					pLevel->m_pHavokWorld->stepDeltaTime( MAYHEM_UPDATE_STEP );
				}
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static hkStreamReader* sOpenStreamReader( 
	const char* name )
{
	hkStreamReader* sb = hkStreambufFactory::getInstance().openReader(name);
	if ( sb->isOk() )
	{
		return sb;
	}
	sb->removeReference();
	return HK_NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static hkStreamWriter* sOpenStreamWriter( 
	const char* name )
{
	hkStreamWriter * sb = hkStreambufFactory::getInstance().openWriter(name);
	if ( sb->isOk() )
	{
		return sb;
	}
	sb->removeReference();
	return HK_NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static
void sHavokShapeHolderInit( 
	HAVOK_SHAPE_HOLDER & tHavokShapeHolder,
	hkShape * pShape )
{
	tHavokShapeHolder.pHavokShape = pShape;
	if ( pShape )
		pShape->addReference();
	tHavokShapeHolder.dwMemSizeAndFlags = pShape ? pShape->m_memSizeAndFlags : 0;
	pShape->m_memSizeAndFlags = 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static
hkStorageMeshShape * sCreateStorageMeshShape( 
	int nVertexStructSize, 
	int nVertexBufferSize, 
	int nVertexCount, 
	void * pVertices, 
	int nIndexBufferSize, 
	int nTriangleCount, 
	WORD * pwIndexBuffer )
{
	hkStorageMeshShape * pStorage = new hkStorageMeshShape( 0.05f ); // for some reason they want a radius - not sure if they need it

	hkMeshShape* mesh = new hkMeshShape();
	REF(mesh);
	hkMeshShape::Subpart part;
	part.m_indexBase = pwIndexBuffer;
	part.m_indexStriding = sizeof( WORD ) * 3;
	part.m_numTriangles = nTriangleCount;
	part.m_numVertices = nVertexCount;
	part.m_stridingType = hkMeshShape::INDICES_INT16;
	part.m_vertexBase = (float *) pVertices;
	part.m_vertexStriding = nVertexStructSize;

	pStorage->addSubpart( part );

	return pStorage;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//Removing redundant extension change (already handled in c_granny_rigid)
//so that I can use this function repeatedly in HavokSaveMultiMoppCode

void HavokSaveMoppCode( 
	const char * pszFilename, 
	int nVertexStructSize, 
	int nVertexBufferSize, 
	int nVertexCount, 
	void * pVertices, 
	int nIndexBufferSize, 
	int nTriangleCount, 
	WORD * pwIndexBuffer )
{
//	char pszMoppFileName[MAX_XML_STRING_LENGTH];
//	PStrReplaceExtension(pszMoppFileName, MAX_XML_STRING_LENGTH, pszFilename, "mop");

	//Debug code: Create a vertex out in the wild blue yonder and see if it yanks the angle.
/*	nVertexCount++;
	nVertexBufferSize+= nVertexStructSize;
	VECTOR *pNewVertices = (VECTOR *) MALLOC(NULL, nVertexBufferSize);
	for(int i = 0; i < nVertexCount; i++) pNewVertices[i].fX = ((VECTOR *)(pVertices))[i].fX,
					pNewVertices[i].fY= ((VECTOR *)(pVertices))[i].fY,
					pNewVertices[i].fZ= ((VECTOR *)(pVertices))[i].fZ;
	//pVertices = (VECTOR *) REALLOC(NULL, ((VECTOR*)pVertices), nVertexCount*sizeof(VECTOR));
	pNewVertices[nVertexCount-1] = VECTOR(-100.0f, 100.0f, 100.0f);
	nTriangleCount++;
	nIndexBufferSize += 3*sizeof(WORD);
	WORD * pwNewIndexBuffer = (WORD *) MALLOC(NULL, nIndexBufferSize);
	int j;
	for(j = 0; j < 3*(nTriangleCount-1); j++) pwNewIndexBuffer[j] = pwIndexBuffer[j];
	pwNewIndexBuffer[j] = nVertexCount-1;
	pwNewIndexBuffer[j+1] = 0;
	pwNewIndexBuffer[j+2] = 1;*/
	
	//end debug code.

	hkStorageMeshShape * pStorageMeshShape = sCreateStorageMeshShape( nVertexStructSize, nVertexBufferSize, nVertexCount, pVertices, //pNewVertices, //
																	  nIndexBufferSize, nTriangleCount, pwIndexBuffer );

	hkMoppFitToleranceRequirements mft;
	hkMoppCode* pMoppCode = hkMoppUtility::buildCode( pStorageMeshShape, mft );

	// Now write out to file
	hkStreamWriter * sb = sOpenStreamWriter( pszFilename /* pszMoppFileName*/ );
	if ( sb )
	{
		// Create an archive 
		hkOArchive outputArchive(sb);

		// Write mopp data out. 
		hkMoppCodeStreamer::writeMoppCodeToArchive(pMoppCode, outputArchive);
		sb->removeReference();
	}

	pMoppCode->removeReference();

	pStorageMeshShape->removeReference();

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

enum ExclusionState{ INCLUDED, OCCLUDED, CUTOFF };

struct orthogonalProjection
{
	VECTOR original;
	VECTOR projection;

	bool bMarked;
	ExclusionState eExclusion; 

	orthogonalProjection(const VECTOR & orig, const VECTOR &dir1, const VECTOR &dir2, const VECTOR &dir3)
		:original(orig), bMarked(false), eExclusion(INCLUDED)
	{
		projection.fX = VectorDot(original, dir1);
		projection.fY = VectorDot(original, dir2);
		projection.fZ = VectorDot(original, dir3);
	}

	void makeProjection(const VECTOR &dir1, const VECTOR &dir2, const VECTOR &dir3)
	{
		projection.fX = VectorDot(original, dir1);
		projection.fY = VectorDot(original, dir2);
		projection.fZ = VectorDot(original, dir3);
	}
};
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

VECTOR invertProjection(VECTOR input, const VECTOR &dir1, const VECTOR &dir2, const VECTOR &dir3 )
{//assumes orthogonal unit vectors.
	MATRIX mTransform;
	MatrixMultiply(&mTransform, 0.0f);


	mTransform.m[0][0] = dir1.fX, mTransform.m[0][1] = dir1.fY, mTransform.m[0][2] = dir1.fZ;
	mTransform.m[1][0] = dir2.fX, mTransform.m[1][1] = dir2.fY, mTransform.m[1][2] = dir2.fZ;
	mTransform.m[2][0] = dir3.fX, mTransform.m[2][1] = dir3.fY, mTransform.m[2][2] = dir3.fZ;

	//might have this transposed.

	//MATRIX mInverse;

	//MatrixTranspose(&mInverse, &mTransform);

	VECTOR output;
	MatrixMultiply(&output, &input, &mTransform);

	return output;
}

bool operator <(const orthogonalProjection &a, const orthogonalProjection&b)
{
	return a.projection.fX < b.projection.fX;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

VECTOR VectorTriangleNormal(const VECTOR &v1, const VECTOR &v2, const VECTOR &v3)
{
	VECTOR vDiff1 = v2 - v1;
	VECTOR vDiff2 = v3 - v2;
	VECTOR vNormal;
	VectorCross(vNormal, vDiff1, vDiff2);

	float fMagnitude = VectorLength(vNormal);

	if(fMagnitude < 0.001)// EPSILON, but it's dangerous to use low epsilons for square roots
		return VECTOR(0.0f, 0.0f, 0.0f);
	else return vNormal/fMagnitude;
}
//Perhaps I should move this to VECTOR.h.  I'll think about it.
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct MeshParameters
{
	//Note: struct contains two pointers, neither of which it will initialize or destruct
	//I'll add a Finalize function to FREE these two pointers.
	int nVertexStructSize; 
	int nVertexBufferSize; 
	int nVertexCount;
	VECTOR * pVertices; 
	int nIndexBufferSize; 
	int nTriangleCount; 
	WORD * pwIndexBuffer; 
	
	MeshParameters(const int& nVSS, const int & nVBS, const int & nVC, void * pV,
					const int & nIBS, const int &nTC, WORD * pwIB)
					:nVertexStructSize(nVSS), nVertexBufferSize(nVBS), nVertexCount(nVC),
					pVertices( (VECTOR *) pV), nIndexBufferSize(nIBS),
					nTriangleCount(nTC), pwIndexBuffer(pwIB)
	{}

	MeshParameters() {} //Default constructor does nothing.

	void Finalize() //Only call if we didn't steal the pVertices or indexbuffer...
	{
		FREE(NULL, pVertices);
		FREE(NULL, pwIndexBuffer);
	}


};
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

MeshParameters SeamExtender(const MeshParameters & mpOriginal, float fSeamWidth = 0.0f )
{
	//Extends the edges of a room shape so that there is no HavokFX dropoff at the edges.
	//Defaults to Seam Width = roomwidth/100

	//Note: Theoretically, anything that is a ceiling should produce a ceiling triangle.
	//Thus, ceiling extensions should get deleted by ceiling deleter.  Nevertheless,
	//I don't want vertical walls having extensions.  So I'll put in a vectortrianglenormal
	//check similar to DeleteCelings.
	//Note that a vertical triangle on an edge produces far too many triangles and vertices.

	MeshParameters mpNew(mpOriginal);
	mpNew.pVertices = (VECTOR *)MALLOC(NULL, 
		(mpOriginal.nVertexCount+4*mpOriginal.nTriangleCount)*sizeof(VECTOR) );
	MemoryCopy(mpNew.pVertices, (mpOriginal.nVertexCount+2*mpOriginal.nTriangleCount)*sizeof(VECTOR),
		mpOriginal.pVertices, mpOriginal.nVertexBufferSize);

	mpNew.pwIndexBuffer = (WORD *)MALLOC(NULL,
		mpOriginal.nTriangleCount*5*3*sizeof(WORD)); //up to two extra triangles per triangle.
	MemoryCopy(mpNew.pwIndexBuffer, mpOriginal.nTriangleCount*3*3*sizeof(WORD),
		mpOriginal.pwIndexBuffer, mpOriginal.nIndexBufferSize);

	float fXMax =-999999.0f, fYMax =-999999.0f, fZMax =-999999.0f;
	float fXMin = 999999.0f, fYMin = 999999.0f, fZMin = 999999.0f;

	for(int i = 0; i < mpOriginal.nVertexCount; i++)
	{
		if(fYMax < ((VECTOR *)(mpOriginal.pVertices))[i].fY) fYMax = ((VECTOR *)(mpOriginal.pVertices))[i].fY;
		if(fYMin > ((VECTOR *)(mpOriginal.pVertices))[i].fY) fYMin = ((VECTOR *)(mpOriginal.pVertices))[i].fY;
		if(fZMax < ((VECTOR *)(mpOriginal.pVertices))[i].fZ) fZMax = ((VECTOR *)(mpOriginal.pVertices))[i].fZ;
		if(fZMin > ((VECTOR *)(mpOriginal.pVertices))[i].fZ) fZMin = ((VECTOR *)(mpOriginal.pVertices))[i].fZ;
		if(fXMax < ((VECTOR *)(mpOriginal.pVertices))[i].fX) fXMax = ((VECTOR *)(mpOriginal.pVertices))[i].fX;
		if(fXMin > ((VECTOR *)(mpOriginal.pVertices))[i].fX) fXMin = ((VECTOR *)(mpOriginal.pVertices))[i].fX;
	}
	float fXRange = fXMax - fXMin;
	float fYRange = fYMax - fYMin;
	float fZRange = fZMax - fZMin;
	REF(fZRange);

	float fXPositions[4] = { fXMin, fXMax, 99.0f, 99.0f };
	float fYPositions[4] = { 99.0f, 99.0f, fYMin, fYMax };

	VECTOR vSeamDirections[4]; 

	if(fSeamWidth == 0.0f) //make seams proportional to ranges
	{
		vSeamDirections[0] = VECTOR(-fXRange/100.0f, 0.0f, 0.0f);
		vSeamDirections[1] = VECTOR( fXRange/100.0f, 0.0f, 0.0f);
		vSeamDirections[2] = VECTOR(0.0f, -fYRange/100.0f, 0.0f);
		vSeamDirections[3] = VECTOR(0.0f,  fYRange/100.0f, 0.0f);
	}
	else
	{
		vSeamDirections[0] = VECTOR(-fSeamWidth, 0.0f, 0.0f);
		vSeamDirections[1] = VECTOR( fSeamWidth, 0.0f, 0.0f);
		vSeamDirections[2] = VECTOR(0.0f, -fSeamWidth, 0.0f);
		vSeamDirections[3] = VECTOR(0.0f,  fSeamWidth, 0.0f);
	}

	const VECTOR vDir = VECTOR(0.0f, 0.0f, 1.0f);

	for(int nDirection = 0; nDirection < 4; nDirection++)
		for(int i = 0; i < mpOriginal.nTriangleCount; i++)
	{	//for each triangle, check if each of the 3 edges is on the edge of the map,
		//and extend seams if so.

		//Counterexample to this code: A mesh consisting of a degenerate triangle
		//with all points lying on a line.  This would create no less than 12
		//triangles and 12 vertices, which we have not allocated memory for.
		//I am making the assumption that no one does this to me, but if it
		//turns out false, I can simply add a triangle degeneracy test.
		VECTOR vNormal = VectorTriangleNormal
			(mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i]],
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i+1]],
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i+2]]);

		if(VectorDot(vDir, vNormal) > EPSILON && VectorLength(vNormal) > EPSILON ) for(int k = 0; k < 3; k++)
		{ //there, I put in a degeneracy check: vectorlength//don't process triangle unless it's a floor.
			int nVertexIndex1 = mpOriginal.pwIndexBuffer[3*i + k];
			int nVertexIndex2 = mpOriginal.pwIndexBuffer[3*i + (k+1)%3 ];

			if( (ABS(mpOriginal.pVertices[nVertexIndex1].fX - fXPositions[nDirection] ) < EPSILON &&
				 ABS(mpOriginal.pVertices[nVertexIndex2].fX - fXPositions[nDirection] ) < EPSILON) ||
				(ABS(mpOriginal.pVertices[nVertexIndex1].fY - fYPositions[nDirection] ) < EPSILON &&
				 ABS(mpOriginal.pVertices[nVertexIndex2].fY - fYPositions[nDirection] ) < EPSILON) )
			{
				//create two new vertices and two triangles.  Note that the triangles should be
				//stacked in "reverse order" (though havokFX treats triangles as twosided, so it's no big deal)
				int nVertexIndex3 = mpNew.nVertexCount;
				int nVertexIndex4 = mpNew.nVertexCount+1;

				mpNew.pVertices[nVertexIndex3] = vSeamDirections[nDirection] + mpOriginal.pVertices[nVertexIndex1];
				mpNew.pVertices[nVertexIndex4] = vSeamDirections[nDirection] + mpOriginal.pVertices[nVertexIndex2];

				mpNew.pwIndexBuffer[3*mpNew.nTriangleCount    ] = (WORD)nVertexIndex1;
				mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 1] = (WORD)nVertexIndex3;
				mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 2] = (WORD)nVertexIndex4;
				mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 3] = (WORD)nVertexIndex2;
				mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 4] = (WORD)nVertexIndex1;
				mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 5] = (WORD)nVertexIndex4;

				mpNew.nVertexCount += 2;
				mpNew.nTriangleCount += 2;
				mpNew.nIndexBufferSize += 6*sizeof(WORD); //note: can be moved down and calculated all at once
			}
		}
	}

	mpNew.nVertexBufferSize = mpNew.nVertexCount*mpNew.nVertexStructSize;

	mpNew.pVertices = (VECTOR *)REALLOC(NULL, mpNew.pVertices, mpNew.nVertexBufferSize);
	mpNew.pwIndexBuffer = (WORD *)REALLOC(NULL, mpNew.pwIndexBuffer, mpNew.nIndexBufferSize);

	ASSERT(mpNew.nVertexCount <= (mpOriginal.nVertexCount+4*mpOriginal.nTriangleCount) );
	ASSERT(mpNew.nTriangleCount <= mpOriginal.nTriangleCount*5);

	return mpNew;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

MeshParameters LedgeMaker(const MeshParameters & mpOriginal, float fLedgeWidth = 1.0f,const VECTOR & vDir = VECTOR(0.0f, 0.0f, 1.0f))
{//Takes a mesh and detects walls parallel to vDir.  Adds feet at the bottom and ledges at the top.
	//Should give walls some thickness, and make it possible to represent low buildings correctly
	//with just a single heightmap.

	//1 May 2006, intentionally reversing ledge direction so bugs don't tunnel into walls.
	//float fLedgeWidth = 1.0f; //May change this to be dynamic based upon the largest X, Y, or Z dimension.
	//in that case it will be fLedgeWidth = fMaxDimension/100.0;
	float fLedgeSlope = 1.0f;//0.2f; //Making non-flat ledges to prevent invisible ledge clustering.

	MeshParameters mpNew(mpOriginal);
	mpNew.pVertices = (VECTOR *)MALLOC(NULL, 
		(mpOriginal.nVertexCount+2*mpOriginal.nTriangleCount)*sizeof(VECTOR) );
	MemoryCopy(mpNew.pVertices, (mpOriginal.nVertexCount+2*mpOriginal.nTriangleCount)*sizeof(VECTOR),
		mpOriginal.pVertices, mpOriginal.nVertexBufferSize);

	mpNew.pwIndexBuffer = (WORD *)MALLOC(NULL,
		mpOriginal.nTriangleCount*3*3*sizeof(WORD)); //up to two extra triangles per triangle.
	MemoryCopy(mpNew.pwIndexBuffer, mpOriginal.nTriangleCount*3*3*sizeof(WORD),
		mpOriginal.pwIndexBuffer, mpOriginal.nIndexBufferSize);

	for(int i = 0; i < mpOriginal.nTriangleCount; i++)
	{
		VECTOR vNormal = VectorTriangleNormal( 
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i]],
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i+1]],
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i+2]]);

		if( ABS(VectorDot(vNormal, vDir) ) < EPSILON && VectorLength(vNormal) > EPSILON) //Normal perpendicular to vdir
			for(int k = 0; k < 3; k++)
		{
			int nVertexIndex1 = mpOriginal.pwIndexBuffer[3*i + k];
			int nVertexIndex2 = mpOriginal.pwIndexBuffer[3*i + (k+1)%3 ];

			if(ABS(VectorDot(vDir, mpOriginal.pVertices[nVertexIndex1] - mpOriginal.pVertices[nVertexIndex2]))
				< EPSILON) //Flat edge of the triangle
			{
				int nVertexIndex3 = mpNew.nVertexCount;
				int nVertexIndex4 = mpNew.nVertexCount+1;
				VECTOR vDisplacement;
				VectorCross(vDisplacement, vDir, 
					mpOriginal.pVertices[nVertexIndex1] - mpOriginal.pVertices[nVertexIndex2]);
				VectorNormalize(vDisplacement);
				vDisplacement *= fLedgeWidth;
				float fTriangleDirection = VectorDot(vDir, 
					mpOriginal.pVertices[nVertexIndex1] - mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i + (k+2)%3 ]]);
				fTriangleDirection /= ABS(fTriangleDirection);
				vDisplacement += fTriangleDirection*vDir*fLedgeSlope;


				//Create ledge triangles.
				mpNew.pVertices[nVertexIndex3] = mpNew.pVertices[nVertexIndex1] + vDisplacement; //changed from + to -
				mpNew.pVertices[nVertexIndex4] = mpNew.pVertices[nVertexIndex2] + vDisplacement;

				mpNew.pwIndexBuffer[3*mpNew.nTriangleCount    ] = (WORD)nVertexIndex1;
				mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 1] = (WORD)nVertexIndex3;
				mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 2] = (WORD)nVertexIndex4;
				mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 3] = (WORD)nVertexIndex2;
				mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 4] = (WORD)nVertexIndex1;
				mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 5] = (WORD)nVertexIndex4;

				mpNew.nVertexCount += 2;
				mpNew.nTriangleCount += 2;
				mpNew.nIndexBufferSize += 6*sizeof(WORD); //note: can be moved down and calculated all at once


			}
		}
	}
	mpNew.nVertexBufferSize = mpNew.nVertexCount*mpNew.nVertexStructSize;

	mpNew.pVertices = (VECTOR *)REALLOC(NULL, mpNew.pVertices, mpNew.nVertexBufferSize);
	mpNew.pwIndexBuffer = (WORD *)REALLOC(NULL, mpNew.pwIndexBuffer, mpNew.nIndexBufferSize);

	return mpNew;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

MeshParameters ReverseLedgeMaker(const MeshParameters & mpOriginal, float fLedgeWidth = 0.5f, const VECTOR & vDir = VECTOR(0.0f, 0.0f, 1.0f))
{//Takes a mesh and detects walls parallel to vDir.  Adds feet at the bottom and ledges at the top.
	//Should give walls some thickness, and make it possible to represent low buildings correctly
	//with just a single heightmap.

	//1 May 2006, intentionally reversing ledge direction so bugs don't tunnel into walls.

	//Note that currently, reverseledgemaker is exactly the same as ledgemaker with 
	//a -1.0f fLedgeWidth.  The two branches have converged.  They may diverge in future,
	//especially if I need to extend vertical walls for multi-directional FX projection.
	//So I'm keeping this function around. 

	//Diverge: default value for fLedgeWidth now differs.

	//float fLedgeWidth = 1.0f; //May change this to be dynamic based upon the largest X, Y, or Z dimension.
	//in that case it will be fLedgeWidth = fMaxDimension/100.0;
	float fLedgeSlope = 1.0f;//0.2f; //Making non-flat ledges to prevent invisible ledge clustering.

	MeshParameters mpNew(mpOriginal);
	mpNew.pVertices = (VECTOR *)MALLOC(NULL, 
		(mpOriginal.nVertexCount+2*mpOriginal.nTriangleCount)*sizeof(VECTOR) );
	MemoryCopy(mpNew.pVertices, (mpOriginal.nVertexCount+2*mpOriginal.nTriangleCount)*sizeof(VECTOR),
		mpOriginal.pVertices, mpOriginal.nVertexBufferSize);

	mpNew.pwIndexBuffer = (WORD *)MALLOC(NULL,
		mpOriginal.nTriangleCount*3*3*sizeof(WORD)); //up to two extra triangles per triangle.
	MemoryCopy(mpNew.pwIndexBuffer, mpOriginal.nTriangleCount*3*3*sizeof(WORD),
		mpOriginal.pwIndexBuffer, mpOriginal.nIndexBufferSize);

	for(int i = 0; i < mpOriginal.nTriangleCount; i++)
	{
		VECTOR vNormal = VectorTriangleNormal( 
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i]],
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i+1]],
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i+2]]);

		if( ABS(VectorDot(vNormal, vDir) ) < EPSILON && VectorLength(vNormal) > EPSILON) //Normal perpendicular to vdir
			for(int k = 0; k < 3; k++)
			{
				int nVertexIndex1 = mpOriginal.pwIndexBuffer[3*i + k];
				int nVertexIndex2 = mpOriginal.pwIndexBuffer[3*i + (k+1)%3 ];

				if(ABS(VectorDot(vDir, mpOriginal.pVertices[nVertexIndex1] - mpOriginal.pVertices[nVertexIndex2]))
					< EPSILON) //Flat edge of the triangle
				{
					int nVertexIndex3 = mpNew.nVertexCount;
					int nVertexIndex4 = mpNew.nVertexCount+1;
					VECTOR vDisplacement;
					VectorCross(vDisplacement, vDir, 
						mpOriginal.pVertices[nVertexIndex1] - mpOriginal.pVertices[nVertexIndex2]);
					VectorNormalize(vDisplacement);
					vDisplacement *= fLedgeWidth;
					float fTriangleDirection = VectorDot(vDir, 
						mpOriginal.pVertices[nVertexIndex1] - mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i + (k+2)%3 ]]);
					fTriangleDirection /= ABS(fTriangleDirection);
					vDisplacement += fTriangleDirection*vDir*fLedgeSlope;


					//Create ledge triangles.
					mpNew.pVertices[nVertexIndex3] = mpNew.pVertices[nVertexIndex1] - vDisplacement; //changed from + to -
					mpNew.pVertices[nVertexIndex4] = mpNew.pVertices[nVertexIndex2] - vDisplacement;

					mpNew.pwIndexBuffer[3*mpNew.nTriangleCount    ] = (WORD)nVertexIndex1;
					mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 1] = (WORD)nVertexIndex3;
					mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 2] = (WORD)nVertexIndex4;
					mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 3] = (WORD)nVertexIndex2;
					mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 4] = (WORD)nVertexIndex1;
					mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 5] = (WORD)nVertexIndex4;

					mpNew.nVertexCount += 2;
					mpNew.nTriangleCount += 2;
					mpNew.nIndexBufferSize += 6*sizeof(WORD); //note: can be moved down and calculated all at once


				}
			}
	}
	mpNew.nVertexBufferSize = mpNew.nVertexCount*mpNew.nVertexStructSize;

	mpNew.pVertices = (VECTOR *)REALLOC(NULL, mpNew.pVertices, mpNew.nVertexBufferSize);
	mpNew.pwIndexBuffer = (WORD *)REALLOC(NULL, mpNew.pwIndexBuffer, mpNew.nIndexBufferSize);

	return mpNew;
}

MeshParameters DoubleDownwardLedgeMaker(const MeshParameters & mpOriginal, float fLedgeWidth = 0.0f,const VECTOR & vDir = VECTOR(0.0f, 0.0f, 1.0f))
{//Takes a mesh and detects walls parallel to vDir.  At both the bottom and top, adds two
	//downward sloping ledges.  One can see it as adding a tiny sloped roof to every wall.

	//decided two is overkill, and going back to singles.  both downward, though.

	//If the default value of 0.0f is used, fLedgeWidth depends will be fMaxDimension/50.0f
	
	float fLedgeSlope = 1.0f;//0.2f; //Making non-flat ledges to prevent invisible ledge clustering.

	if(fLedgeWidth == 0.0f)
	{
		float fXMax =-999999.0f, fYMax =-999999.0f, fZMax =-999999.0f;
		float fXMin = 999999.0f, fYMin = 999999.0f, fZMin = 999999.0f;

		for(int i = 0; i < mpOriginal.nVertexCount; i++)
		{
			if(fYMax < ((VECTOR *)(mpOriginal.pVertices))[i].fY) fYMax = ((VECTOR *)(mpOriginal.pVertices))[i].fY;
			if(fYMin > ((VECTOR *)(mpOriginal.pVertices))[i].fY) fYMin = ((VECTOR *)(mpOriginal.pVertices))[i].fY;
			//if(fZMax < ((VECTOR *)(mpOriginal.pVertices))[i].fZ) fZMax = ((VECTOR *)(mpOriginal.pVertices))[i].fZ;
			//if(fZMin > ((VECTOR *)(mpOriginal.pVertices))[i].fZ) fZMin = ((VECTOR *)(mpOriginal.pVertices))[i].fZ;
			if(fXMax < ((VECTOR *)(mpOriginal.pVertices))[i].fX) fXMax = ((VECTOR *)(mpOriginal.pVertices))[i].fX;
			if(fXMin > ((VECTOR *)(mpOriginal.pVertices))[i].fX) fXMin = ((VECTOR *)(mpOriginal.pVertices))[i].fX;
		}
		float fXRange = fXMax - fXMin;
		float fYRange = fYMax - fYMin;
		float fZRange = fZMax - fZMin;
		REF(fZRange);

		fLedgeWidth = MIN(fXRange, fYRange)/50.0f;
	}
	const VECTOR vDownward = VECTOR(0.0f, 0.0f, -fLedgeWidth*fLedgeSlope);

	MeshParameters mpNew(mpOriginal);
	mpNew.pVertices = (VECTOR *)MALLOC(NULL, 
		(mpOriginal.nVertexCount+2*mpOriginal.nTriangleCount)*sizeof(VECTOR) );
	MemoryCopy(mpNew.pVertices, (mpOriginal.nVertexCount+2*2*mpOriginal.nTriangleCount)*sizeof(VECTOR),
		mpOriginal.pVertices, mpOriginal.nVertexBufferSize);

	mpNew.pwIndexBuffer = (WORD *)MALLOC(NULL,
		mpOriginal.nTriangleCount*3*3*sizeof(WORD)); //up to two extra triangles per triangle.
	MemoryCopy(mpNew.pwIndexBuffer, mpOriginal.nTriangleCount*2*3*3*sizeof(WORD),//up to four now, since it's double.
		mpOriginal.pwIndexBuffer, mpOriginal.nIndexBufferSize);

	for(int i = 0; i < mpOriginal.nTriangleCount; i++)
	{
		VECTOR vNormal = VectorTriangleNormal( 
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i]],
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i+1]],
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i+2]]);

		if( ABS(VectorDot(vNormal, vDir) ) < EPSILON && VectorLength(vNormal) > EPSILON) //Normal perpendicular to vdir
			for(int k = 0; k < 3; k++)
		{
			int nVertexIndex1 = mpOriginal.pwIndexBuffer[3*i + k];
			int nVertexIndex2 = mpOriginal.pwIndexBuffer[3*i + (k+1)%3 ];

			if(ABS(VectorDot(vDir, mpOriginal.pVertices[nVertexIndex1] - mpOriginal.pVertices[nVertexIndex2]))
				< EPSILON) //Flat edge of the triangle
			{
				
				VECTOR vDisplacement;
				VectorCross(vDisplacement, vDir, 
					mpOriginal.pVertices[nVertexIndex1] - mpOriginal.pVertices[nVertexIndex2]);
				VectorNormalize(vDisplacement);
				vDisplacement *= fLedgeWidth;
				//float fTriangleDirection = VectorDot(vDir, 
				//	mpOriginal.pVertices[nVertexIndex1] - mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i + (k+2)%3 ]]);
				//fTriangleDirection /= ABS(fTriangleDirection);
				//vDisplacement += fTriangleDirection*vDir*fLedgeSlope;
				{//create front downward ledge.
					int nVertexIndex3 = mpNew.nVertexCount;
					int nVertexIndex4 = mpNew.nVertexCount+1;
					//Create ledge triangles.
					mpNew.pVertices[nVertexIndex3] = mpNew.pVertices[nVertexIndex1] + vDisplacement + vDownward; //changed from + to -
					mpNew.pVertices[nVertexIndex4] = mpNew.pVertices[nVertexIndex2] + vDisplacement + vDownward;

					mpNew.pwIndexBuffer[3*mpNew.nTriangleCount    ] = (WORD)nVertexIndex1;
					mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 1] = (WORD)nVertexIndex3;
					mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 2] = (WORD)nVertexIndex4;
					mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 3] = (WORD)nVertexIndex2;
					mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 4] = (WORD)nVertexIndex1;
					mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 5] = (WORD)nVertexIndex4;

					mpNew.nVertexCount += 2;
					mpNew.nTriangleCount += 2;
					mpNew.nIndexBufferSize += 6*sizeof(WORD); //note: can be moved down and calculated all at once
				}
				//{//create back downward ledge.  Note triangles need to be stacked backward.
				//	int nVertexIndex3 = mpNew.nVertexCount;
				//	int nVertexIndex4 = mpNew.nVertexCount+1;
				//	//Create ledge triangles.
				//	mpNew.pVertices[nVertexIndex3] = mpNew.pVertices[nVertexIndex1] - vDisplacement + vDownward; //changed from + to -
				//	mpNew.pVertices[nVertexIndex4] = mpNew.pVertices[nVertexIndex2] - vDisplacement + vDownward;

				//	mpNew.pwIndexBuffer[3*mpNew.nTriangleCount    ] = nVertexIndex1;
				//	mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 1] = nVertexIndex4;
				//	mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 2] = nVertexIndex3;
				//	mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 3] = nVertexIndex1;
				//	mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 4] = nVertexIndex2;
				//	mpNew.pwIndexBuffer[3*mpNew.nTriangleCount + 5] = nVertexIndex4;

				//	mpNew.nVertexCount += 2;
				//	mpNew.nTriangleCount += 2;
				//	mpNew.nIndexBufferSize += 6*sizeof(WORD); //note: can be moved down and calculated all at once
				//}
			}
		}
	}
	mpNew.nVertexBufferSize = mpNew.nVertexCount*mpNew.nVertexStructSize;

	mpNew.pVertices = (VECTOR *)REALLOC(NULL, mpNew.pVertices, mpNew.nVertexBufferSize);
	mpNew.pwIndexBuffer = (WORD *)REALLOC(NULL, mpNew.pwIndexBuffer, mpNew.nIndexBufferSize);

	return mpNew;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
MeshParameters NoProcessing(const MeshParameters & mpOriginal)
{//Placeholder function for these others, so I don't have to change Finalizing code.
	MeshParameters mpNew(mpOriginal);
	mpNew.pVertices = (VECTOR *)MALLOC(NULL, mpOriginal.nVertexBufferSize);
	MemoryCopy(mpNew.pVertices, mpNew.nVertexBufferSize,
		mpOriginal.pVertices, mpOriginal.nVertexBufferSize);
	mpNew.pwIndexBuffer = (WORD *)MALLOC(NULL, mpOriginal.nIndexBufferSize);
	MemoryCopy(mpNew.pwIndexBuffer, mpNew.nIndexBufferSize,
		mpOriginal.pwIndexBuffer, mpOriginal.nIndexBufferSize);

	return mpNew;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
MeshParameters DeleteCeilings(const MeshParameters & mpOriginal, const VECTOR & vDir = VECTOR(0.0f, 0.0f, 1.0f))
{//To be used in indoor levels.
	MeshParameters mpNew(mpOriginal);
	mpNew.pVertices = (VECTOR *)MALLOC(NULL, 
		(mpOriginal.nVertexCount+2*mpOriginal.nTriangleCount)*sizeof(VECTOR) );
	//MemoryCopy(mpNew.pVertices, (mpOriginal.nVertexCount+2*mpOriginal.nTriangleCount)*sizeof(VECTOR),
	//	mpOriginal.pVertices, mpOriginal.nVertexBufferSize);

	mpNew.pwIndexBuffer = (WORD *)MALLOC(NULL,
		mpOriginal.nTriangleCount*3*3*sizeof(WORD)); //up to two extra triangles per triangle.
	//MemoryCopy(mpNew.pwIndexBuffer, mpOriginal.nTriangleCount*3*3*sizeof(WORD),
	//	mpOriginal.pwIndexBuffer, mpOriginal.nIndexBufferSize);

	//Run through the list of triangles
	//throw out all triangles with negative VectorDot(Vnormal, Vdir)

	//Hell, throw out all meaningless vertices while we're at it so the slicer isn't fooled.

	int *VertexMapping = (int *)MALLOC(NULL, mpOriginal.nVertexCount*sizeof(int));
	memset(VertexMapping, 0xff, mpOriginal.nVertexCount*sizeof(int));  //Set everything to -1.


	
	mpNew.nTriangleCount = 0;
	mpNew.nVertexCount = 0;

	int i = 0;

	for(i = 0; i < mpOriginal.nTriangleCount; i++)
	{
		VECTOR vNormal = VectorTriangleNormal
			(mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i]],
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i+1]],
			mpOriginal.pVertices[mpOriginal.pwIndexBuffer[3*i+2]]);

		if(VectorDot(vDir, vNormal)
			< -EPSILON)
		{//Delete all indices by simply not copying them.
			/*VertexMapping[mpNew.pVertices[mpOriginal.pwIndexBuffer[3*i]]] =
				mpNew.pVertices[mpOriginal.pwIndexBuffer[3*i+1]] =
				mpNew.pVertices[mpOriginal.pwIndexBuffer[3*i+2]] = false;*/
			//Unnecessary since it's MallocZ, but let's do it anyway.
			//On second thought, they may be involved in useful triangles.
		}
		else
		{//Transfer all indices to new index buffer, 
			//and transfer vertices while updating the vertex mapping for untransferred vertices
			for(int k = 0; k < 3; k++)
			{
				int nIndex = mpOriginal.pwIndexBuffer[3*i+k];
				if(VertexMapping[nIndex] == -1)
				{
					VertexMapping[nIndex] = mpNew.nVertexCount;
					mpNew.nVertexCount++;
				}
				mpNew.pwIndexBuffer[mpNew.nTriangleCount*3+k] = (WORD)VertexMapping[nIndex];
			}
			mpNew.nTriangleCount++;
		}
	}

	for(i = 0; i < mpOriginal.nVertexCount; i++)
	{
		if(VertexMapping[i] != -1)
			mpNew.pVertices[VertexMapping[i]] = mpOriginal.pVertices[i];
	}

	mpNew.nVertexBufferSize = mpNew.nVertexCount*mpNew.nVertexStructSize;
	mpNew.nIndexBufferSize = mpNew.nTriangleCount*3*sizeof(WORD);


	FREE(NULL, VertexMapping);

	if(mpNew.nVertexCount > 0 && mpNew.nTriangleCount > 0) return mpNew;
	else
	{
		//Do nothing if deleting ceilings would leave nothing.
		mpNew.Finalize();
		mpNew = NoProcessing(mpOriginal);
		return mpNew;
	}
}




float AspectRatio(const MeshParameters & mpBuilding) //Finds the ratio of the smallest x,y to the z.
{//technically, aspect ratio is width over height, so you'll have to invert the result
	float fXMax =-999999.0f, fYMax =-999999.0f, fZMax =-999999.0f;
	float fXMin = 999999.0f, fYMin = 999999.0f, fZMin = 999999.0f;

	for(int i = 0; i < mpBuilding.nVertexCount; i++)
	{
		if(fYMax < ((VECTOR *)(mpBuilding.pVertices))[i].fY) fYMax = ((VECTOR *)(mpBuilding.pVertices))[i].fY;
		if(fYMin > ((VECTOR *)(mpBuilding.pVertices))[i].fY) fYMin = ((VECTOR *)(mpBuilding.pVertices))[i].fY;
		if(fZMax < ((VECTOR *)(mpBuilding.pVertices))[i].fZ) fZMax = ((VECTOR *)(mpBuilding.pVertices))[i].fZ;
		if(fZMin > ((VECTOR *)(mpBuilding.pVertices))[i].fZ) fZMin = ((VECTOR *)(mpBuilding.pVertices))[i].fZ;
		if(fXMax < ((VECTOR *)(mpBuilding.pVertices))[i].fX) fXMax = ((VECTOR *)(mpBuilding.pVertices))[i].fX;
		if(fXMin > ((VECTOR *)(mpBuilding.pVertices))[i].fX) fXMin = ((VECTOR *)(mpBuilding.pVertices))[i].fX;
	}
	float fXRange = fXMax - fXMin;
	float fYRange = fYMax - fYMin;
	float fZRange = fZMax - fZMin;

	return min(fXRange,fYRange)/fZRange;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

MeshParameters TotalHorizontalSlicer(const MeshParameters & mpBuilding, int nSlice, int nSliceCount)
{
	//Like HorizontalSlicer, but instead of collapsing the top, it slices it as well.  This code
	//will be a truly shameless copy-paste job with one or two variables changed.

	//For use in levels spiral staircases and such.  I'll call ledgemaker after this, as opposed
	//to before.

	float fXMax =-999999.0f, fYMax =-999999.0f, fZMax =-999999.0f;
	float fXMin = 999999.0f, fYMin = 999999.0f, fZMin = 999999.0f;

	int i;

	for(i = 0; i < mpBuilding.nVertexCount; i++)
	{
		if(fYMax < ((VECTOR *)(mpBuilding.pVertices))[i].fY) fYMax = ((VECTOR *)(mpBuilding.pVertices))[i].fY;
		if(fYMin > ((VECTOR *)(mpBuilding.pVertices))[i].fY) fYMin = ((VECTOR *)(mpBuilding.pVertices))[i].fY;
		if(fZMax < ((VECTOR *)(mpBuilding.pVertices))[i].fZ) fZMax = ((VECTOR *)(mpBuilding.pVertices))[i].fZ;
		if(fZMin > ((VECTOR *)(mpBuilding.pVertices))[i].fZ) fZMin = ((VECTOR *)(mpBuilding.pVertices))[i].fZ;
		if(fXMax < ((VECTOR *)(mpBuilding.pVertices))[i].fX) fXMax = ((VECTOR *)(mpBuilding.pVertices))[i].fX;
		if(fXMin > ((VECTOR *)(mpBuilding.pVertices))[i].fX) fXMin = ((VECTOR *)(mpBuilding.pVertices))[i].fX;
	}
	float fXRange = fXMax - fXMin;
	REF(fXRange);
	float fYRange = fYMax - fYMin;
	REF(fYRange);
	float fZRange = fZMax - fZMin;

	float fZTop = fZMin + (fZRange/nSliceCount)*(1+nSlice); //top of the slice
	float fZBottom = fZMin + (fZRange/nSliceCount)*nSlice; //bottom of the slice

	//Two passes.  The first will truncate triangles below the bottom. 
	//The second will move down vertices above the top.

	//First pass has three stages: identify dead vertices, run through triangle list placing vertices, and append remaining vertices to the end
	const int VERTEX_SHIFT = 15000;  //If we have more vertices than this, this algorithm fails miserably.

	MeshParameters mpSlice(mpBuilding);
	{
		mpSlice.pVertices = (VECTOR *)MALLOC(NULL, 
			(mpBuilding.nVertexCount+2*mpBuilding.nTriangleCount)*sizeof(VECTOR));
		//At most, 2 extra vertices per triangle, and two triangles spawned from each triangle.
		mpSlice.pwIndexBuffer = (WORD *)MALLOC(NULL, mpBuilding.nIndexBufferSize*2);

		mpSlice.nVertexCount = 0;
		mpSlice.nTriangleCount=0;

		int *VertexMapping = (int *)MALLOC(NULL, mpBuilding.nVertexCount*sizeof(int));

		int nNewIndex = VERTEX_SHIFT;
		for(i = 0; i < mpBuilding.nVertexCount; i++)
		{
			if(mpBuilding.pVertices[i].fZ < fZBottom) VertexMapping[i] = -1; //deleted
			else
			{
				VertexMapping[i] = nNewIndex;
				nNewIndex++;
			}
		}

		int k;
		BOOL deleted[3];
		float fDiff;
		VECTOR vTop;

		for(i = 0; i < mpBuilding.nTriangleCount; i++ )
		{
			int nDeleted = 0;

			for(k = 0; k < 3; k++)
			{
				if(VertexMapping[mpBuilding.pwIndexBuffer[3*i+k]] == -1)
				{
					nDeleted++;
					deleted[k] = true;
				}
				else deleted[k] = false;
			}


			int nDeletedIndex[2];
			int nKeptIndex[2];
			int a, b; //I've used up all the good names...

			switch (nDeleted)
			{
			case 3:
				continue;
				break;
			case 0:
				for(k = 0; k < 3; k++)
					mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)VertexMapping[mpBuilding.pwIndexBuffer[3*i+k]];
				mpSlice.nTriangleCount++;
				break;
			case 2:
				a = b = 0;
				for(k = 0; k < 3; k++) if(deleted[k])
				{
					nDeletedIndex[a] = mpBuilding.pwIndexBuffer[3*i+k];
					a++;
				}
				else 
				{
					nKeptIndex[0] = mpBuilding.pwIndexBuffer[3*i+k];
					b = k;
				}
				ASSERT_CONTINUE(a == 2);
				vTop = mpBuilding.pVertices[ nKeptIndex[0] ];
				fDiff = vTop.fZ - fZBottom;

				mpSlice.pVertices[mpSlice.nVertexCount] = vTop + fDiff/(vTop.fZ - mpBuilding.pVertices[ nDeletedIndex[0] ].fZ)
					*(mpBuilding.pVertices[ nDeletedIndex[0] ] - vTop);
				mpSlice.pVertices[mpSlice.nVertexCount+1] = vTop + fDiff/(vTop.fZ - mpBuilding.pVertices[ nDeletedIndex[1] ].fZ)
					*(mpBuilding.pVertices[ nDeletedIndex[1] ] - vTop);

				ASSERT( ABS(mpSlice.pVertices[mpSlice.nVertexCount].fZ - fZBottom) < EPSILON);

				//Assign the vertices in the right order.
				a = 0;
				for(k = 0; k < 3; k++)
				{
					if(b == k) mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[0]];
					else
					{
						mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)(mpSlice.nVertexCount + a);
						a++;
					}
				}
				ASSERT_CONTINUE(a == 2);

				mpSlice.nTriangleCount++;
				mpSlice.nVertexCount+= 2;
				break;

			case 1:
				//Similar to case 2, we make projections from both vertices onto fZ = fZBottom.
				//In this one, however, we create two triangles.
				a = b = 0;
				for(k = 0; k < 3; k++) if(!deleted[k])
				{
					nKeptIndex[a] = mpBuilding.pwIndexBuffer[3*i+k];
					a++;
				}
				else
				{
					nDeletedIndex[0] = mpBuilding.pwIndexBuffer[3*i+k];
					b = k;
				}
				ASSERT_CONTINUE(a==2);

				VECTOR vBottom = mpBuilding.pVertices[ nDeletedIndex[0] ];
				fDiff = fZBottom - vBottom.fZ;
				mpSlice.pVertices[mpSlice.nVertexCount] = vBottom + fDiff/(mpBuilding.pVertices[ nKeptIndex[0] ].fZ - vBottom.fZ)
					*(mpBuilding.pVertices[ nKeptIndex[0] ] - vBottom);
				mpSlice.pVertices[mpSlice.nVertexCount+1] = vBottom + fDiff/(mpBuilding.pVertices[ nKeptIndex[1] ].fZ - vBottom.fZ)
					*(mpBuilding.pVertices[ nKeptIndex[1] ] - vBottom);

				ASSERT( ABS(mpSlice.pVertices[mpSlice.nVertexCount].fZ - fZBottom) < EPSILON);

				a = 0;
				for(k = 0; k < 3; k++)
				{
					if(b == k) mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)mpSlice.nVertexCount;
					else
					{
						mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[a]];
						a++;
					}
				}
				mpSlice.nTriangleCount++;
				//first triangle with one new vertex, second triangle with two.
				//Unsure of this, possible I'm inverting a triangle:

				a = 0;
				for(k = 0; k < 3; k++)
				{
					if(b == k) mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[1]];
					else
					{
						mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)(mpSlice.nVertexCount + 1 - a);
						a++;
					}
				}
				ASSERT_CONTINUE(a == 2);

				mpSlice.nTriangleCount++;
				mpSlice.nVertexCount+=2;


				break;
			}
		}
		//Append remaining vertices.
		int nVertexUnshift = mpSlice.nVertexCount - VERTEX_SHIFT;
		for(i = 0; i < mpSlice.nTriangleCount*3; i++) 
			if(mpSlice.pwIndexBuffer[i] >= VERTEX_SHIFT) mpSlice.pwIndexBuffer[i] = (WORD)(mpSlice.pwIndexBuffer[i] + nVertexUnshift);

		for(i = 0; i < mpBuilding.nVertexCount; i++) if(VertexMapping[i] != -1)
		{
			mpSlice.pVertices[mpSlice.nVertexCount] = mpBuilding.pVertices[i];//[VertexMapping[i] + nVertexUnshift];
			ASSERT(mpSlice.nVertexCount == VertexMapping[i] + nVertexUnshift);
			mpSlice.nVertexCount++;
		}
		FREE(NULL, VertexMapping);
		mpSlice.nVertexBufferSize = mpSlice.nVertexCount*mpSlice.nVertexStructSize;
		mpSlice.nIndexBufferSize = mpSlice.nTriangleCount*3*sizeof(WORD);
	}
	MeshParameters mpSlice2(mpSlice);
	{
		mpSlice2.pVertices = (VECTOR *)MALLOC(NULL, 
			(mpSlice.nVertexCount+2*mpSlice.nTriangleCount)*sizeof(VECTOR));
		//At most, 2 extra vertices per triangle, and two triangles spawned from each triangle.
		mpSlice2.pwIndexBuffer = (WORD *)MALLOC(NULL, mpSlice.nIndexBufferSize*2);

		mpSlice2.nVertexCount = 0;
		mpSlice2.nTriangleCount=0;

		int *VertexMapping = (int *)MALLOC(NULL, mpSlice.nVertexCount*sizeof(int));

		int nNewIndex = VERTEX_SHIFT;
		for(i = 0; i < mpSlice.nVertexCount; i++)
		{
			if(mpSlice.pVertices[i].fZ > fZTop) VertexMapping[i] = -1; //deleted
			else
			{
				VertexMapping[i] = nNewIndex;
				nNewIndex++;
			}
		}

		int k;
		BOOL deleted[3];
		float fDiff;
		VECTOR vTop;

		for(i = 0; i < mpSlice.nTriangleCount; i++ )
		{
			int nDeleted = 0;

			for(k = 0; k < 3; k++)
			{
				if(VertexMapping[mpSlice.pwIndexBuffer[3*i+k]] == -1)
				{
					nDeleted++;
					deleted[k] = true;
				}
				else deleted[k] = false;
			}


			int nDeletedIndex[2];
			int nKeptIndex[2];
			int a, b; //I've used up all the good names...

			switch (nDeleted)
			{
			case 3:
				continue;
				break;
			case 0:
				for(k = 0; k < 3; k++)
					mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)VertexMapping[mpSlice.pwIndexBuffer[3*i+k]];
				mpSlice2.nTriangleCount++;
				break;
			case 2:
				a = b = 0;
				for(k = 0; k < 3; k++) if(deleted[k])
				{
					nDeletedIndex[a] = mpSlice.pwIndexBuffer[3*i+k];
					a++;
				}
				else 
				{
					nKeptIndex[0] = mpSlice.pwIndexBuffer[3*i+k];
					b = k;
				}
				ASSERT_CONTINUE(a == 2);
				vTop = mpSlice.pVertices[ nKeptIndex[0] ];
				fDiff = vTop.fZ - fZTop;

				mpSlice2.pVertices[mpSlice2.nVertexCount] = vTop + fDiff/(vTop.fZ - mpSlice.pVertices[ nDeletedIndex[0] ].fZ)
					*(mpSlice.pVertices[ nDeletedIndex[0] ] - vTop);
				mpSlice2.pVertices[mpSlice2.nVertexCount+1] = vTop + fDiff/(vTop.fZ - mpSlice.pVertices[ nDeletedIndex[1] ].fZ)
					*(mpSlice.pVertices[ nDeletedIndex[1] ] - vTop);

				ASSERT( ABS(mpSlice2.pVertices[mpSlice2.nVertexCount].fZ - fZTop) < EPSILON);

				//Assign the vertices in the right order.
				a = 0;
				for(k = 0; k < 3; k++)
				{
					if(b == k) mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[0]];
					else
					{
						mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)(mpSlice2.nVertexCount + a);
						a++;
					}
				}
				ASSERT_CONTINUE(a == 2);

				mpSlice2.nTriangleCount++;
				mpSlice2.nVertexCount+= 2;
				break;

			case 1:
				//Similar to case 2, we make projections from both vertices onto fZ = fZBottom.
				//In this one, however, we create two triangles.
				a = b = 0;
				for(k = 0; k < 3; k++) if(!deleted[k])
				{
					nKeptIndex[a] = mpSlice.pwIndexBuffer[3*i+k];
					a++;
				}
				else
				{
					nDeletedIndex[0] = mpSlice.pwIndexBuffer[3*i+k];
					b = k;
				}
				ASSERT_CONTINUE(a==2);

				VECTOR vBottom = mpSlice.pVertices[ nDeletedIndex[0] ];
				fDiff = fZTop - vBottom.fZ;
				mpSlice2.pVertices[mpSlice2.nVertexCount] = vBottom + fDiff/(mpSlice.pVertices[ nKeptIndex[0] ].fZ - vBottom.fZ)
					*(mpSlice.pVertices[ nKeptIndex[0] ] - vBottom);
				mpSlice2.pVertices[mpSlice2.nVertexCount+1] = vBottom + fDiff/(mpSlice.pVertices[ nKeptIndex[1] ].fZ - vBottom.fZ)
					*(mpSlice.pVertices[ nKeptIndex[1] ] - vBottom);

				ASSERT( ABS(mpSlice2.pVertices[mpSlice2.nVertexCount].fZ - fZTop) < EPSILON);

				a = 0;
				for(k = 0; k < 3; k++)
				{
					if(b == k) mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)(mpSlice2.nVertexCount);
					else
					{
						mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[a]];
						a++;
					}
				}
				mpSlice2.nTriangleCount++;
				//first triangle with one new vertex, second triangle with two.
				//Unsure of this, possible I'm inverting a triangle:

				a = 0;
				for(k = 0; k < 3; k++)
				{
					if(b == k) mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[1]];
					else
					{
						mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)(mpSlice2.nVertexCount + 1 - a);
						a++;
					}
				}
				ASSERT_CONTINUE(a == 2);

				mpSlice2.nTriangleCount++;
				mpSlice2.nVertexCount+=2;


				break;
			}
		}
		//Append remaining vertices.
		int nVertexUnshift = mpSlice2.nVertexCount - VERTEX_SHIFT;
		for(i = 0; i < mpSlice2.nTriangleCount*3; i++) 
			if(mpSlice2.pwIndexBuffer[i] >= VERTEX_SHIFT) mpSlice2.pwIndexBuffer[i] = (WORD)(mpSlice2.pwIndexBuffer[i] + nVertexUnshift);

		for(i = 0; i < mpSlice.nVertexCount; i++) if(VertexMapping[i] != -1)
		{
			mpSlice2.pVertices[mpSlice2.nVertexCount] = mpSlice.pVertices[i];//[VertexMapping[i] + nVertexUnshift];
			ASSERT(mpSlice2.nVertexCount == VertexMapping[i] + nVertexUnshift);
			mpSlice2.nVertexCount++;
		}
		FREE(NULL, VertexMapping);

		mpSlice2.nVertexBufferSize = mpSlice2.nVertexCount*mpSlice2.nVertexStructSize;
		mpSlice2.nIndexBufferSize = mpSlice2.nTriangleCount*3*sizeof(WORD);
	}

	mpSlice.Finalize();
	return mpSlice2;
}

MeshParameters ProportionalTotalHorizontalSlicer(const MeshParameters & mpBuilding, float fBottom, float fTop)
{
	//Like TotalHorizontalSlicer, but determines top and bottom proportions by direct float.  This code
	//will be a truly shameless copy-paste job with one or two variables changed.

	//Slices from fBottom of the way up to fTop of the way up.

	//For use in levels spiral staircases and such.  I'll call ledgemaker after this, as opposed
	//to before.

	float fXMax =-999999.0f, fYMax =-999999.0f, fZMax =-999999.0f;
	float fXMin = 999999.0f, fYMin = 999999.0f, fZMin = 999999.0f;

	int i;

	for(i = 0; i < mpBuilding.nVertexCount; i++)
	{
		if(fYMax < ((VECTOR *)(mpBuilding.pVertices))[i].fY) fYMax = ((VECTOR *)(mpBuilding.pVertices))[i].fY;
		if(fYMin > ((VECTOR *)(mpBuilding.pVertices))[i].fY) fYMin = ((VECTOR *)(mpBuilding.pVertices))[i].fY;
		if(fZMax < ((VECTOR *)(mpBuilding.pVertices))[i].fZ) fZMax = ((VECTOR *)(mpBuilding.pVertices))[i].fZ;
		if(fZMin > ((VECTOR *)(mpBuilding.pVertices))[i].fZ) fZMin = ((VECTOR *)(mpBuilding.pVertices))[i].fZ;
		if(fXMax < ((VECTOR *)(mpBuilding.pVertices))[i].fX) fXMax = ((VECTOR *)(mpBuilding.pVertices))[i].fX;
		if(fXMin > ((VECTOR *)(mpBuilding.pVertices))[i].fX) fXMin = ((VECTOR *)(mpBuilding.pVertices))[i].fX;
	}
	float fXRange = fXMax - fXMin;
	REF(fXRange);
	float fYRange = fYMax - fYMin;
	REF(fYRange);
	float fZRange = fZMax - fZMin;

	fTop += EPSILON;
	fBottom -= EPSILON;

	float fZTop = fZMin + fTop * fZRange;
	float fZBottom = fZMin + fBottom*fZRange;

	//float fZTop = fZMin + (fZRange/nSliceCount)*(1+nSlice); //top of the slice
	//float fZBottom = fZMin + (fZRange/nSliceCount)*nSlice; //bottom of the slice

	//Two passes.  The first will truncate triangles below the bottom. 
	//The second will move down vertices above the top.

	//First pass has three stages: identify dead vertices, run through triangle list placing vertices, and append remaining vertices to the end
	const int VERTEX_SHIFT = 15000;  //If we have more vertices than this, this algorithm fails miserably.

	MeshParameters mpSlice(mpBuilding);
	{
		mpSlice.pVertices = (VECTOR *)MALLOC(NULL, 
			(mpBuilding.nVertexCount+2*mpBuilding.nTriangleCount)*sizeof(VECTOR));
		//At most, 2 extra vertices per triangle, and two triangles spawned from each triangle.
		mpSlice.pwIndexBuffer = (WORD *)MALLOC(NULL, mpBuilding.nIndexBufferSize*2);

		mpSlice.nVertexCount = 0;
		mpSlice.nTriangleCount=0;

		int *VertexMapping = (int *)MALLOC(NULL, mpBuilding.nVertexCount*sizeof(int));

		int nNewIndex = VERTEX_SHIFT;
		for(i = 0; i < mpBuilding.nVertexCount; i++)
		{
			if(mpBuilding.pVertices[i].fZ < fZBottom) VertexMapping[i] = -1; //deleted
			else
			{
				VertexMapping[i] = nNewIndex;
				nNewIndex++;
			}
		}

		int k;
		BOOL deleted[3];
		float fDiff;
		VECTOR vTop;

		for(i = 0; i < mpBuilding.nTriangleCount; i++ )
		{
			int nDeleted = 0;

			for(k = 0; k < 3; k++)
			{
				if(VertexMapping[mpBuilding.pwIndexBuffer[3*i+k]] == -1)
				{
					nDeleted++;
					deleted[k] = true;
				}
				else deleted[k] = false;
			}


			int nDeletedIndex[2];
			int nKeptIndex[2];
			int a, b; //I've used up all the good names...

			switch (nDeleted)
			{
			case 3:
				continue;
				break;
			case 0:
				for(k = 0; k < 3; k++)
					mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)VertexMapping[mpBuilding.pwIndexBuffer[3*i+k]];
				mpSlice.nTriangleCount++;
				break;
			case 2:
				a = b = 0;
				for(k = 0; k < 3; k++) if(deleted[k])
				{
					nDeletedIndex[a] = mpBuilding.pwIndexBuffer[3*i+k];
					a++;
				}
				else 
				{
					nKeptIndex[0] = mpBuilding.pwIndexBuffer[3*i+k];
					b = k;
				}
				ASSERT_CONTINUE(a == 2);
				vTop = mpBuilding.pVertices[ nKeptIndex[0] ];
				fDiff = vTop.fZ - fZBottom;

				mpSlice.pVertices[mpSlice.nVertexCount] = vTop + fDiff/(vTop.fZ - mpBuilding.pVertices[ nDeletedIndex[0] ].fZ)
					*(mpBuilding.pVertices[ nDeletedIndex[0] ] - vTop);
				mpSlice.pVertices[mpSlice.nVertexCount+1] = vTop + fDiff/(vTop.fZ - mpBuilding.pVertices[ nDeletedIndex[1] ].fZ)
					*(mpBuilding.pVertices[ nDeletedIndex[1] ] - vTop);

				ASSERT( ABS(mpSlice.pVertices[mpSlice.nVertexCount].fZ - fZBottom) < EPSILON);

				//Assign the vertices in the right order.
				a = 0;
				for(k = 0; k < 3; k++)
				{
					if(b == k) mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[0]];
					else
					{
						mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)(mpSlice.nVertexCount + a);
						a++;
					}
				}
				ASSERT_CONTINUE(a == 2);

				mpSlice.nTriangleCount++;
				mpSlice.nVertexCount+= 2;
				break;

			case 1:
				//Similar to case 2, we make projections from both vertices onto fZ = fZBottom.
				//In this one, however, we create two triangles.
				a = b = 0;
				for(k = 0; k < 3; k++) if(!deleted[k])
				{
					nKeptIndex[a] = mpBuilding.pwIndexBuffer[3*i+k];
					a++;
				}
				else
				{
					nDeletedIndex[0] = mpBuilding.pwIndexBuffer[3*i+k];
					b = k;
				}
				ASSERT_CONTINUE(a==2);

				VECTOR vBottom = mpBuilding.pVertices[ nDeletedIndex[0] ];
				fDiff = fZBottom - vBottom.fZ;
				mpSlice.pVertices[mpSlice.nVertexCount] = vBottom + fDiff/(mpBuilding.pVertices[ nKeptIndex[0] ].fZ - vBottom.fZ)
					*(mpBuilding.pVertices[ nKeptIndex[0] ] - vBottom);
				mpSlice.pVertices[mpSlice.nVertexCount+1] = vBottom + fDiff/(mpBuilding.pVertices[ nKeptIndex[1] ].fZ - vBottom.fZ)
					*(mpBuilding.pVertices[ nKeptIndex[1] ] - vBottom);

				ASSERT( ABS(mpSlice.pVertices[mpSlice.nVertexCount].fZ - fZBottom) < EPSILON);

				a = 0;
				for(k = 0; k < 3; k++)
				{
					if(b == k) mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)mpSlice.nVertexCount;
					else
					{
						mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[a]];
						a++;
					}
				}
				mpSlice.nTriangleCount++;
				//first triangle with one new vertex, second triangle with two.
				//Unsure of this, possible I'm inverting a triangle:

				a = 0;
				for(k = 0; k < 3; k++)
				{
					if(b == k) mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[1]];
					else
					{
						mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)(mpSlice.nVertexCount + 1 - a);
						a++;
					}
				}
				ASSERT_CONTINUE(a == 2);

				mpSlice.nTriangleCount++;
				mpSlice.nVertexCount+=2;


				break;
			}
		}
		//Append remaining vertices.
		int nVertexUnshift = mpSlice.nVertexCount - VERTEX_SHIFT;
		for(i = 0; i < mpSlice.nTriangleCount*3; i++) 
			if(mpSlice.pwIndexBuffer[i] >= VERTEX_SHIFT) mpSlice.pwIndexBuffer[i] = (WORD)(mpSlice.pwIndexBuffer[i] + nVertexUnshift);

		for(i = 0; i < mpBuilding.nVertexCount; i++) if(VertexMapping[i] != -1)
		{
			mpSlice.pVertices[mpSlice.nVertexCount] = mpBuilding.pVertices[i];//[VertexMapping[i] + nVertexUnshift];
			ASSERT(mpSlice.nVertexCount == VertexMapping[i] + nVertexUnshift);
			mpSlice.nVertexCount++;
		}
		FREE(NULL, VertexMapping);
		mpSlice.nVertexBufferSize = mpSlice.nVertexCount*mpSlice.nVertexStructSize;
		mpSlice.nIndexBufferSize = mpSlice.nTriangleCount*3*sizeof(WORD);
	}
	MeshParameters mpSlice2(mpSlice);
	{
		mpSlice2.pVertices = (VECTOR *)MALLOC(NULL, 
			(mpSlice.nVertexCount+2*mpSlice.nTriangleCount)*sizeof(VECTOR));
		//At most, 2 extra vertices per triangle, and two triangles spawned from each triangle.
		mpSlice2.pwIndexBuffer = (WORD *)MALLOC(NULL, mpSlice.nIndexBufferSize*2);

		mpSlice2.nVertexCount = 0;
		mpSlice2.nTriangleCount=0;

		int *VertexMapping = (int *)MALLOC(NULL, mpSlice.nVertexCount*sizeof(int));

		int nNewIndex = VERTEX_SHIFT;
		for(i = 0; i < mpSlice.nVertexCount; i++)
		{
			if(mpSlice.pVertices[i].fZ > fZTop) VertexMapping[i] = -1; //deleted
			else
			{
				VertexMapping[i] = nNewIndex;
				nNewIndex++;
			}
		}

		int k;
		BOOL deleted[3];
		float fDiff;
		VECTOR vTop;

		for(i = 0; i < mpSlice.nTriangleCount; i++ )
		{
			int nDeleted = 0;

			for(k = 0; k < 3; k++)
			{
				if(VertexMapping[mpSlice.pwIndexBuffer[3*i+k]] == -1)
				{
					nDeleted++;
					deleted[k] = true;
				}
				else deleted[k] = false;
			}


			int nDeletedIndex[2];
			int nKeptIndex[2];
			int a, b; //I've used up all the good names...

			switch (nDeleted)
			{
			case 3:
				continue;
				break;
			case 0:
				for(k = 0; k < 3; k++)
					mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)VertexMapping[mpSlice.pwIndexBuffer[3*i+k]];
				mpSlice2.nTriangleCount++;
				break;
			case 2:
				a = b = 0;
				for(k = 0; k < 3; k++) if(deleted[k])
				{
					nDeletedIndex[a] = mpSlice.pwIndexBuffer[3*i+k];
					a++;
				}
				else 
				{
					nKeptIndex[0] = mpSlice.pwIndexBuffer[3*i+k];
					b = k;
				}
				ASSERT_CONTINUE(a == 2);
				vTop = mpSlice.pVertices[ nKeptIndex[0] ];
				fDiff = vTop.fZ - fZTop;

				mpSlice2.pVertices[mpSlice2.nVertexCount] = vTop + fDiff/(vTop.fZ - mpSlice.pVertices[ nDeletedIndex[0] ].fZ)
					*(mpSlice.pVertices[ nDeletedIndex[0] ] - vTop);
				mpSlice2.pVertices[mpSlice2.nVertexCount+1] = vTop + fDiff/(vTop.fZ - mpSlice.pVertices[ nDeletedIndex[1] ].fZ)
					*(mpSlice.pVertices[ nDeletedIndex[1] ] - vTop);

				ASSERT( ABS(mpSlice2.pVertices[mpSlice2.nVertexCount].fZ - fZTop) < EPSILON);

				//Assign the vertices in the right order.
				a = 0;
				for(k = 0; k < 3; k++)
				{
					if(b == k) mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[0]];
					else
					{
						mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)(mpSlice2.nVertexCount + a);
						a++;
					}
				}
				ASSERT_CONTINUE(a == 2);

				mpSlice2.nTriangleCount++;
				mpSlice2.nVertexCount+= 2;
				break;

			case 1:
				//Similar to case 2, we make projections from both vertices onto fZ = fZBottom.
				//In this one, however, we create two triangles.
				a = b = 0;
				for(k = 0; k < 3; k++) if(!deleted[k])
				{
					nKeptIndex[a] = mpSlice.pwIndexBuffer[3*i+k];
					a++;
				}
				else
				{
					nDeletedIndex[0] = mpSlice.pwIndexBuffer[3*i+k];
					b = k;
				}
				ASSERT_CONTINUE(a==2);

				VECTOR vBottom = mpSlice.pVertices[ nDeletedIndex[0] ];
				fDiff = fZTop - vBottom.fZ;
				mpSlice2.pVertices[mpSlice2.nVertexCount] = vBottom + fDiff/(mpSlice.pVertices[ nKeptIndex[0] ].fZ - vBottom.fZ)
					*(mpSlice.pVertices[ nKeptIndex[0] ] - vBottom);
				mpSlice2.pVertices[mpSlice2.nVertexCount+1] = vBottom + fDiff/(mpSlice.pVertices[ nKeptIndex[1] ].fZ - vBottom.fZ)
					*(mpSlice.pVertices[ nKeptIndex[1] ] - vBottom);

				ASSERT( ABS(mpSlice2.pVertices[mpSlice2.nVertexCount].fZ - fZTop) < EPSILON);

				a = 0;
				for(k = 0; k < 3; k++)
				{
					if(b == k) mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)mpSlice2.nVertexCount;
					else
					{
						mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[a]];
						a++;
					}
				}
				mpSlice2.nTriangleCount++;
				//first triangle with one new vertex, second triangle with two.
				//Unsure of this, possible I'm inverting a triangle:

				a = 0;
				for(k = 0; k < 3; k++)
				{
					if(b == k) mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[1]];
					else
					{
						mpSlice2.pwIndexBuffer[mpSlice2.nTriangleCount*3+k] = (WORD)(mpSlice2.nVertexCount + 1 - a);
						a++;
					}
				}
				ASSERT_CONTINUE(a == 2);

				mpSlice2.nTriangleCount++;
				mpSlice2.nVertexCount+=2;


				break;
			}
		}
		//Append remaining vertices.
		int nVertexUnshift = mpSlice2.nVertexCount - VERTEX_SHIFT;
		for(i = 0; i < mpSlice2.nTriangleCount*3; i++) 
			if(mpSlice2.pwIndexBuffer[i] >= VERTEX_SHIFT) mpSlice2.pwIndexBuffer[i] = (WORD)(mpSlice2.pwIndexBuffer[i] + nVertexUnshift);

		for(i = 0; i < mpSlice.nVertexCount; i++) if(VertexMapping[i] != -1)
		{
			mpSlice2.pVertices[mpSlice2.nVertexCount] = mpSlice.pVertices[i];//[VertexMapping[i] + nVertexUnshift];
			ASSERT(mpSlice2.nVertexCount == VertexMapping[i] + nVertexUnshift);
			mpSlice2.nVertexCount++;
		}
		FREE(NULL, VertexMapping);

		mpSlice2.nVertexBufferSize = mpSlice2.nVertexCount*mpSlice2.nVertexStructSize;
		mpSlice2.nIndexBufferSize = mpSlice2.nTriangleCount*3*sizeof(WORD);
	}

	mpSlice.Finalize();
	return mpSlice2;
}

MeshParameters HorizontalSlicer(const MeshParameters & mpBuilding, int nSlice, int nSliceCount)
//We used aspect ratio to determine how many slices to make, now chop chop chop.
{//Convention: the bottom is the first (index 0) slice.
	float fXMax =-999999.0f, fYMax =-999999.0f, fZMax =-999999.0f;
	float fXMin = 999999.0f, fYMin = 999999.0f, fZMin = 999999.0f;

	int i;

	for(i = 0; i < mpBuilding.nVertexCount; i++)
	{
		if(fYMax < ((VECTOR *)(mpBuilding.pVertices))[i].fY) fYMax = ((VECTOR *)(mpBuilding.pVertices))[i].fY;
		if(fYMin > ((VECTOR *)(mpBuilding.pVertices))[i].fY) fYMin = ((VECTOR *)(mpBuilding.pVertices))[i].fY;
		if(fZMax < ((VECTOR *)(mpBuilding.pVertices))[i].fZ) fZMax = ((VECTOR *)(mpBuilding.pVertices))[i].fZ;
		if(fZMin > ((VECTOR *)(mpBuilding.pVertices))[i].fZ) fZMin = ((VECTOR *)(mpBuilding.pVertices))[i].fZ;
		if(fXMax < ((VECTOR *)(mpBuilding.pVertices))[i].fX) fXMax = ((VECTOR *)(mpBuilding.pVertices))[i].fX;
		if(fXMin > ((VECTOR *)(mpBuilding.pVertices))[i].fX) fXMin = ((VECTOR *)(mpBuilding.pVertices))[i].fX;
	}
	float fXRange = fXMax - fXMin;
	REF(fXRange);
	float fYRange = fYMax - fYMin;
	REF(fYRange);
	float fZRange = fZMax - fZMin;

	float fZTop = fZMin + (fZRange/nSliceCount)*(1+nSlice); //top of the slice
	float fZBottom = fZMin + (fZRange/nSliceCount)*nSlice; //bottom of the slice

	//Two passes.  The first will truncate triangles below the bottom. 
	//The second will move down vertices above the top.

	//First pass has three stages: identify dead vertices, run through triangle list placing vertices, and append remaining vertices to the end
	const int VERTEX_SHIFT = 15000;  //If we have more vertices than this, this algorithm fails miserably.

	MeshParameters mpSlice(mpBuilding);

	mpSlice.pVertices = (VECTOR *)MALLOC(NULL, 
							(mpBuilding.nVertexCount+2*mpBuilding.nTriangleCount)*sizeof(VECTOR));
	//At most, 2 extra vertices per triangle, and two triangles spawned from each triangle.
	mpSlice.pwIndexBuffer = (WORD *)MALLOC(NULL, mpBuilding.nIndexBufferSize*2);

	mpSlice.nVertexCount = 0;
	mpSlice.nTriangleCount=0;

	int *VertexMapping = (int *)MALLOC(NULL, mpBuilding.nVertexCount*sizeof(int));

	int nNewIndex = VERTEX_SHIFT;
	for(i = 0; i < mpBuilding.nVertexCount; i++)
	{
		if(mpBuilding.pVertices[i].fZ < fZBottom) VertexMapping[i] = -1; //deleted
		else
		{
			VertexMapping[i] = nNewIndex;
			nNewIndex++;
		}
	}

	int k;
	BOOL deleted[3];
	float fDiff;
	VECTOR vTop;

	for(i = 0; i < mpBuilding.nTriangleCount; i++ )
	{
		int nDeleted = 0;

		for(k = 0; k < 3; k++)
		{
			if(VertexMapping[mpBuilding.pwIndexBuffer[3*i+k]] == -1)
			{
				nDeleted++;
				deleted[k] = true;
			}
			else deleted[k] = false;
		}


		int nDeletedIndex[2];
		int nKeptIndex[2];
		int a, b; //I've used up all the good names...

		switch (nDeleted)
		{
		case 3:
			continue;
			break;
		case 0:
			for(k = 0; k < 3; k++)
				mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)VertexMapping[mpBuilding.pwIndexBuffer[3*i+k]];
			mpSlice.nTriangleCount++;
			break;
		case 2:
			a = b = 0;
			for(k = 0; k < 3; k++) if(deleted[k])
			{
				nDeletedIndex[a] = mpBuilding.pwIndexBuffer[3*i+k];
				a++;
			}
			else 
			{
				nKeptIndex[0] = mpBuilding.pwIndexBuffer[3*i+k];
				b = k;
			}
			ASSERT_CONTINUE(a == 2);
			vTop = mpBuilding.pVertices[ nKeptIndex[0] ];
			fDiff = vTop.fZ - fZBottom;

			mpSlice.pVertices[mpSlice.nVertexCount] = vTop + fDiff/(vTop.fZ - mpBuilding.pVertices[ nDeletedIndex[0] ].fZ)
				*(mpBuilding.pVertices[ nDeletedIndex[0] ] - vTop);
			mpSlice.pVertices[mpSlice.nVertexCount+1] = vTop + fDiff/(vTop.fZ - mpBuilding.pVertices[ nDeletedIndex[1] ].fZ)
				*(mpBuilding.pVertices[ nDeletedIndex[1] ] - vTop);

			ASSERT( ABS(mpSlice.pVertices[mpSlice.nVertexCount].fZ - fZBottom) < EPSILON);

			//Assign the vertices in the right order.
			a = 0;
			for(k = 0; k < 3; k++)
			{
				if(b == k) mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[0]];
				else
				{
					mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)(mpSlice.nVertexCount + a);
					a++;
				}
			}
			ASSERT_CONTINUE(a == 2);

			mpSlice.nTriangleCount++;
			mpSlice.nVertexCount+= 2;
			break;

		case 1:
			//Similar to case 2, we make projections from both vertices onto fZ = fZBottom.
			//In this one, however, we create two triangles.
			a = b = 0;
			for(k = 0; k < 3; k++) if(!deleted[k])
			{
				nKeptIndex[a] = mpBuilding.pwIndexBuffer[3*i+k];
				a++;
			}
			else
			{
				nDeletedIndex[0] = mpBuilding.pwIndexBuffer[3*i+k];
				b = k;
			}
			ASSERT_CONTINUE(a==2);

			VECTOR vBottom = mpBuilding.pVertices[ nDeletedIndex[0] ];
			fDiff = fZBottom - vBottom.fZ;
			mpSlice.pVertices[mpSlice.nVertexCount] = vBottom + fDiff/(mpBuilding.pVertices[ nKeptIndex[0] ].fZ - vBottom.fZ)
						*(mpBuilding.pVertices[ nKeptIndex[0] ] - vBottom);
			mpSlice.pVertices[mpSlice.nVertexCount+1] = vBottom + fDiff/(mpBuilding.pVertices[ nKeptIndex[1] ].fZ - vBottom.fZ)
						*(mpBuilding.pVertices[ nKeptIndex[1] ] - vBottom);

			ASSERT( ABS(mpSlice.pVertices[mpSlice.nVertexCount].fZ - fZBottom) < EPSILON);

			a = 0;
			for(k = 0; k < 3; k++)
			{
				if(b == k) mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)mpSlice.nVertexCount;
				else
				{
					mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[a]];
					a++;
				}
			}
			mpSlice.nTriangleCount++;
			//first triangle with one new vertex, second triangle with two.
			//Unsure of this, possible I'm inverting a triangle:
			
			a = 0;
			for(k = 0; k < 3; k++)
			{
				if(b == k) mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)VertexMapping[nKeptIndex[1]];
				else
				{
					mpSlice.pwIndexBuffer[mpSlice.nTriangleCount*3+k] = (WORD)(mpSlice.nVertexCount + 1 - a);
					a++;
				}
			}
			ASSERT_CONTINUE(a == 2);

			mpSlice.nTriangleCount++;
			mpSlice.nVertexCount+=2;


			break;
		}
	}
	//Append remaining vertices.
	int nVertexUnshift = mpSlice.nVertexCount - VERTEX_SHIFT;
	for(i = 0; i < mpSlice.nTriangleCount*3; i++) 
		if(mpSlice.pwIndexBuffer[i] >= VERTEX_SHIFT) mpSlice.pwIndexBuffer[i] = (WORD)(mpSlice.pwIndexBuffer[i] + nVertexUnshift);

	for(i = 0; i < mpBuilding.nVertexCount; i++) if(VertexMapping[i] != -1)
	{
		mpSlice.pVertices[mpSlice.nVertexCount] = mpBuilding.pVertices[i];//[VertexMapping[i] + nVertexUnshift];
		ASSERT(mpSlice.nVertexCount == VertexMapping[i] + nVertexUnshift);
		mpSlice.nVertexCount++;
	}

	//End of phase 1.  Phase 2 will only take a few lines.

	for(i = 0; i < mpSlice.nVertexCount; i++)
	{
		ASSERT( (mpSlice.pVertices[i].fZ + EPSILON) >= fZBottom);
		if(mpSlice.pVertices[i].fZ > fZTop) 
			mpSlice.pVertices[i].fZ = fZTop;
	}

	FREE(NULL, VertexMapping);

	mpSlice.nVertexBufferSize = mpSlice.nVertexCount*mpSlice.nVertexStructSize;
	mpSlice.nIndexBufferSize = mpSlice.nTriangleCount*3*sizeof(WORD);

	return mpSlice;
}

MeshParameters CubeFaceChopper(const MeshParameters &mpOld, int nSlice, int nSliceCount)
{//Processes meshes by throwing out all points below the facade of whatever direction we look.
	VECTOR directions[6] = {VECTOR( 0, 0, -1), VECTOR( 0, 0, 1),
		VECTOR( -1, 0, 0), VECTOR( 1, 0, 0),
		VECTOR( 0, -1, 0), VECTOR( 0, 1, 0) };
	//Photography angles.

	float fDepths[6] = {.66f, .66f, .66f, .66f, .66f, .66f};
	//How deep our snapshot looks compared to min(fYRange, fZRange).  Set low for just facade.

	orthogonalProjection * pProjected = 
		(orthogonalProjection *)MALLOCZ(NULL, mpOld.nVertexCount*sizeof(orthogonalProjection));

	int i;
	for(i = 0; i < mpOld.nVertexCount; i++)
	{
		pProjected[i].original = ((VECTOR *)(mpOld.pVertices))[i];
		pProjected[i].makeProjection(directions[nSlice], directions[(nSlice+2)%6], directions[(nSlice+4)%6]);
		pProjected[i].eExclusion = INCLUDED;
	}

	int * vectorMapping = (int *) MALLOC(NULL, mpOld.nVertexCount*sizeof(int));
	memset(vectorMapping, 0xff, mpOld.nVertexCount*sizeof(int));

//	for(i = 0; i < mpOld.nVertexCount; i++) pProjected[i](mpOld.pVertices[i], 
//		directions[nSlice], directions[(nSlice+2)%6], directions[(nSlice+4)%6]);

	float fXMax =-999999.0f, fYMax =-999999.0f, fZMax =-999999.0f;
	float fXMin = 999999.0f, fYMin = 999999.0f, fZMin = 999999.0f;

	MeshParameters mpNew(mpOld);
	mpNew.nVertexCount = 0;
	mpNew.nTriangleCount=0;
	mpNew.pVertices = (VECTOR *)MALLOCZ(NULL, mpOld.nVertexCount*sizeof(VECTOR));
	mpNew.pwIndexBuffer = (WORD*) MALLOC(NULL, mpOld.nIndexBufferSize*sizeof(WORD));


	for(i = 0; i < mpOld.nVertexCount; i++)
	{
		if(fYMax < pProjected[i].projection.fY) fYMax = pProjected[i].projection.fY;
		if(fYMin > pProjected[i].projection.fY) fYMin = pProjected[i].projection.fY;
		if(fZMax < pProjected[i].projection.fZ) fZMax = pProjected[i].projection.fZ;
		if(fZMin > pProjected[i].projection.fZ) fZMin = pProjected[i].projection.fZ;
		if(fXMax < pProjected[i].projection.fX) fXMax = pProjected[i].projection.fX;
		if(fXMin > pProjected[i].projection.fX) fXMin = pProjected[i].projection.fX;
	}

	float fMinLength = min( ABS(fYMax - fYMin), ABS(fZMax - fZMin) );

	//fMinLength = min(ABS(fXMax - fXMin), fMinLength); //A design decision which splits
	//even moppfiles which would work unsplit.  Unnecessary, but it helps for testing.

	float fMaxLength = max( ABS(fYMax - fYMin), ABS(fZMax - fZMin) );
	fMaxLength = min(ABS(fXMax - fXMin), fMaxLength);


	float fXLimit = fXMin + fDepths[nSlice]*fMinLength;

	for(i = 0; i < mpOld.nVertexCount; i++)
	{
		if ( (pProjected[i]).projection.fX > fXLimit) 
		{
			(pProjected[i]).eExclusion = CUTOFF;
			continue;
		}

		mpNew.pVertices[mpNew.nVertexCount] = pProjected[i].original;
		vectorMapping[i] = mpNew.nVertexCount;
		mpNew.nVertexCount++;
		pProjected[i].bMarked = true;
	}

	for(i = 0; i < mpOld.nTriangleCount; i++)
	{
		int nVertexIndex;
		for(int k = 0; k < 3; k++)
		{
			nVertexIndex= mpOld.pwIndexBuffer[i*3 + k];
			if(vectorMapping[nVertexIndex] == -1) goto nextTriangle; //continue one level up
			mpNew.pwIndexBuffer[mpNew.nTriangleCount*3 + k] = (WORD)vectorMapping[nVertexIndex];
		}
		mpNew.nTriangleCount++;
		mpNew.nIndexBufferSize+= 3 * sizeof( WORD );// INDEX_BUFFER_TYPE );

nextTriangle: continue;
	}

	mpNew.nVertexBufferSize = mpNew.nVertexCount*mpNew.nVertexStructSize;
	mpNew.nIndexBufferSize = mpNew.nTriangleCount*3*sizeof(WORD);

	FREE(NULL, vectorMapping);

	return mpNew;
}

//Generic meshchopper function.
//In future, may pick chopping function based on whether we're indoors or outdoors, etc.
MeshParameters * GeneralMeshChopper(const MeshParameters & mpBuilding, int &nSliceCount, int nSliceType = 0)
{
//Possible slice types:
//	0, do a HorizontalSlice just to force the z-projection.  Use aspectratio for slicecount.
//	1, do a TotalHorizontalSlice in order to split catwalks etc.  Use nSliceCount given.
//Musing instead to do 10-19, and get nSliceCount from nSliceType %10.  Of course it'd always be >12
//Actually, 11 if I wanted to use reverseledgemaker instead of normal ledgemaker.
//Using the 10-19 scheme, I'd like to have just a single number for slicetype.


	//Figure out how many slices are made.  Slicer function specific, may change. if (nRoomType == BUILDING)
	if(nSliceType == 0)
	{//putting in automatic vertical projection and no longer doing a slice-based force.
		//float fRatio = AspectRatio(mpBuilding); //min(x,y)/z
		//nSliceCount = 1 + int(1/(fRatio*0.9f) ); //sliced Z will be at least 10% smaller than either X or Y.
		nSliceCount = 1;

		ASSERT(nSliceCount >= 1);
		
		if(nSliceCount < 1) nSliceCount = 1;
	}
	else if (nSliceType >= 10 && nSliceType <= 19)
	{
		ASSERT(nSliceType > 10); //Actually, 10 might be useful when we want to not even include a shape.
		//If I ever decide it's useful, I'll just comment out this assert.

		ASSERT(nSliceType <= 19);

		nSliceCount = nSliceType - 10;
	}
	else if(nSliceType >= 200 && nSliceType <= 300)
	{
		ASSERT(nSliceType != 200);
		ASSERT(nSliceType != 300); //degenerate cases.

		nSliceCount = 2;
	}
	else if(nSliceType >= 1000 && nSliceType <= 1999)
	{
		nSliceCount = 1;
	}
	else
	{
		nSliceCount = 1;
		//ASSERT(FALSE);//Invalid setting.
	}

	ASSERT(nSliceCount <= 10);

//	nSliceCount = 5; //debug code for total slicer.
	MeshParameters * pmpSlices = (MeshParameters *) MALLOC(NULL, nSliceCount*sizeof(MeshParameters));



	if(nSliceCount <= 1)
	{
		pmpSlices[0] = MeshParameters(mpBuilding);
		pmpSlices[0].pVertices = (VECTOR *)MALLOC(NULL, mpBuilding.nVertexBufferSize);
		MemoryCopy(pmpSlices[0].pVertices, pmpSlices[0].nVertexBufferSize,
			mpBuilding.pVertices, mpBuilding.nVertexBufferSize);
		pmpSlices[0].pwIndexBuffer = (WORD *)MALLOC(NULL, mpBuilding.nIndexBufferSize);
		MemoryCopy(pmpSlices[0].pwIndexBuffer, pmpSlices[0].nIndexBufferSize,
			mpBuilding.pwIndexBuffer, mpBuilding.nIndexBufferSize);

		return pmpSlices;
	}


	for(int nSlice = 0; nSlice < nSliceCount; nSlice++)
	{
		if (nSliceType == 0) pmpSlices[nSlice] = TotalHorizontalSlicer/*CubeFaceChopper*/(mpBuilding, nSlice, nSliceCount); //change this to whichever slicer.
		else if (nSliceType >= 10 && nSliceType <= 19)
			pmpSlices[nSlice] = TotalHorizontalSlicer(mpBuilding, nSlice, nSliceCount);
		else if(nSliceType >= 200 && nSliceType <= 300)
		{
			float fSliceHeight = (nSliceType- 200) / 100.0f; //slice at the percentile, into two pieces.
			ASSERT(fSliceHeight > 0.0f && fSliceHeight < 1.0f);
			pmpSlices[0] = ProportionalTotalHorizontalSlicer(mpBuilding, 0.0f, fSliceHeight);
			pmpSlices[1] = ProportionalTotalHorizontalSlicer(mpBuilding, fSliceHeight, 1.0f);
			break;
		}
	}

	return pmpSlices;
}


MeshParameters GeneralMeshProcessor(const MeshParameters & mpOriginalOld, int nSliceType = 0)
{
	MeshParameters mpOriginal = SeamExtender(mpOriginalOld);
	MeshParameters mpNew;
//	if(nSliceType == 0)
//	{
//		MeshParameters mpIntermediate = DoubleDownwardLedgeMaker(mpOriginal);//LedgeMaker(mpOriginal);
//		mpNew = DeleteCeilings(mpIntermediate);
//		mpIntermediate.Finalize();
//	}
	/*else*/ if (nSliceType >= 1000 && nSliceType <= 1999)
	{
		MeshParameters mpIntermediate = ReverseLedgeMaker(mpOriginal, (nSliceType-1000)/100.0f );
		mpNew = DeleteCeilings(mpIntermediate);
		mpIntermediate.Finalize();
	}
	else //if(nSliceType >=10 && nSliceType<= 19)
	{
		mpNew = DeleteCeilings(mpOriginal);
	}
	mpOriginal.Finalize();
	return mpNew;
}

MeshParameters GeneralPostProcessor(MeshParameters &mpOriginal, int nSliceType = 0)
{
	//To be called after a slicer.
	//Unlike the others, it will finalize its input, as its input will never
	//be used again, for sure.  This is for compatibility with removing the function entirely.
	if(/*nSliceType == 0 ||*/ nSliceType >= 1000)	return mpOriginal;
	else //if(nSliceType >= 10 && nSliceType <= 19)
	{
		MeshParameters mpNew = DoubleDownwardLedgeMaker(mpOriginal);//ReverseLedgeMaker(mpOriginal);
		mpOriginal.Finalize();
		return mpNew;
	}
}

//Slice Types:
//0: default, deletes ceilings, makes ledges, and as many cuts as needed to force vertical raycast.
//11-19: deletes ceilings, cuts into n-10 slices, and does a reverse ledgemaker.
//200-299: cuts into two pieces, the first vertically proportioned by (n-200)/100.0f
//1000: same as zero, but does a reverse ledgemaker with width (n-1000)/100.0f


int HavokSaveMultiMoppCode( //returns number of moppfiles created.
					   const char * pszFilename, 
					   int nVertexStructSize, 
					   int nVertexBufferSize, 
					   int nVertexCount, 
					   void * pVertices, 
					   int nIndexBufferSize, 
					   int nTriangleCount, 
					   WORD * pwIndexBuffer,
					   VECTOR vHavokShapeTranslation[])
{
	char pszMoppFileName[MAX_XML_STRING_LENGTH];
	char ext[4] = "mop";

	MeshParameters mpOriginal(nVertexStructSize, nVertexBufferSize, nVertexCount, pVertices,
						nIndexBufferSize, nTriangleCount, pwIndexBuffer);
	//Look up the excel spreadsheet for nHavokSliceType
	char szFileWithExt[MAX_XML_STRING_LENGTH];
	PStrRemovePath(szFileWithExt, MAX_XML_STRING_LENGTH, FILE_PATH_BACKGROUND, pszFilename);
	char szExcelIndex[MAX_XML_STRING_LENGTH];
	PStrRemoveExtension(szExcelIndex, MAX_XML_STRING_LENGTH, szFileWithExt);
	//int nExcelRoomIndexLine = ExcelGetLineByStringIndex(NULL, DATATABLE_ROOM_INDEX, szExcelIndex);
	//ROOM_INDEX * pRoomIndex = (ROOM_INDEX*)ExcelGetDataByStringIndex(NULL, DATATABLE_ROOM_INDEX, szExcelIndex);
	ROOM_INDEX * pRoomIndex = RoomFileGetRoomIndex( szExcelIndex );
	//Now we pass all the GeneralProcessors pRoomIndex->nHavokSliceType

	//Note: Props, for example, are not on this spreadsheet.  In that case, pRoomIndex will be NULL, and rightly so.
	//Thus, we have to check for a null and only get the slicetype if it's valid.
	int nSliceType = 0;
	if(pRoomIndex != NULL) nSliceType = pRoomIndex->nHavokSliceType;



	//May change this to mpIntermediate and make mpProcessed in the loop.
	MeshParameters mpProcessed = GeneralMeshProcessor(mpOriginal, nSliceType);//LedgeMaker(mpOriginal, VECTOR(0.0f, 0.0f, 1.0f));

	int nSliceCount;

	MeshParameters * pmpSlices = GeneralMeshChopper(mpProcessed, nSliceCount, nSliceType);

	int i;
	int nExtension = 0;

	for(i = 0; i < nSliceCount; i++)
	{
		ext[2] = char('0' + nExtension);
		PStrReplaceExtension(pszMoppFileName, MAX_XML_STRING_LENGTH, pszFilename, ext);

		pmpSlices[i] = GeneralPostProcessor(pmpSlices[i], nSliceType);

		BOOL bCanSave = TRUE;
		if ( FileIsReadOnly( pszMoppFileName ) )
			bCanSave = DataFileCheck( pszMoppFileName );

		if ( bCanSave )
		{
			HavokSaveMoppCode(pszMoppFileName,
				pmpSlices[i].nVertexStructSize, pmpSlices[i].nVertexBufferSize,
				pmpSlices[i].nVertexCount, pmpSlices[i].pVertices, 
				pmpSlices[i].nIndexBufferSize, pmpSlices[i].nTriangleCount,
				pmpSlices[i].pwIndexBuffer);
		}

		nExtension++;

		vHavokShapeTranslation[i] = VECTOR(0.0f, 0.0f, 0.0f); //Not using translation.

		pmpSlices[i].Finalize();
	}
	mpProcessed.Finalize();
	FREE(NULL, pmpSlices);

	return nExtension;
}



//Changing code to split models into 6 mopp files, each from a different
//perspective: top, bottom, left, right, front, back.
//For generality, we use VectorDot to get an orthogonal projection with
//the perspective points.

//We then process the array of orthogonalProjections, throwing out points
//occluded by points above them, and all points further away than 2/3*min(xrange,yrange)

//Notes:  I've reconsidered this approach, worried it may create huge "pits"
//at the edges, walling things off.  Once I get the integration going I'll test
//this.  I will likely switch to an all top-down approach, where we simply slice
//horizontally to force the z-axis to be the minimum.  Below, we'll simply move
//all the vertices downward to the slice, effectively flattening the top part of the
//the building.  Above, we'll cutoff everything below, and re-jigger all triangles
//that have only one or two uncut vertices.

//Luckily, this code can be used for either purpose.

//It also may be necessary to slightly perturb the edges to either create
//or prevent pits.  In SoundStage, only one end of the (whole number float)
//board has an edge pit.

//Now that we've found translation works, I've decided to comment out the fairly
//complicated function and try a very simple one that just calculates simple
//X,Y,Z translation locations.  It may be necessary to detect whether the surface
//is outer or inner, however.  We can do this by calling VectorTriangleNormal and
//dotting the result with the position relative to the center for every triangle.
//Actually, I'm guessing it isn't, as this would kill non-concave shapes.

/*int HavokSaveMultiMoppCode( //returns number of moppfiles created.
						  const char * pszFilename, 
						  int nVertexStructSize, 
						  int nVertexBufferSize, 
						  int nVertexCount, 
						  void * pVertices, 
						  int nIndexBufferSize, 
						  int nTriangleCount, 
						  WORD * pwIndexBuffer,
						  VECTOR vHavokShapeTranslation[])
{
	char pszMoppFileName[MAX_XML_STRING_LENGTH];
	char ext[4] = "mop";

	int i;

	float fXMax =-999999.0f, fYMax =-999999.0f, fZMax =-999999.0f;
	float fXMin = 999999.0f, fYMin = 999999.0f, fZMin = 999999.0f;

	for(i = 0; i < nVertexCount; i++)
	{
		if(fYMax < ((VECTOR *)pVertices)[i].fY) fYMax = ((VECTOR *)pVertices)[i].fY;
		if(fYMin > ((VECTOR *)pVertices)[i].fY) fYMin = ((VECTOR *)pVertices)[i].fY;
		if(fZMax < ((VECTOR *)pVertices)[i].fZ) fZMax = ((VECTOR *)pVertices)[i].fZ;
		if(fZMin > ((VECTOR *)pVertices)[i].fZ) fZMin = ((VECTOR *)pVertices)[i].fZ;
		if(fXMax < ((VECTOR *)pVertices)[i].fX) fXMax = ((VECTOR *)pVertices)[i].fX;
		if(fXMin > ((VECTOR *)pVertices)[i].fX) fXMin = ((VECTOR *)pVertices)[i].fX;
	}
	float fXRange = fXMax - fXMin;
	float fYRange = fYMax - fYMin;
	float fZRange = fZMax - fZMin;


	float fMinLength = min( ABS(fYMax - fYMin), ABS(fZMax - fZMin) );
	fMinLength = min(ABS(fXMax - fXMin), fMinLength);

	VECTOR vCenter( (fXMin+fXMax)/2,(fYMin+fYMax)/2,(fZMin+fZMax)/2);

	for(i = 0; i < 6; i++) vHavokShapeTranslation[i] = vCenter;
	
	vHavokShapeTranslation[0].fZ += fZRange*0.5f - fMinLength*0.33f;
	vHavokShapeTranslation[1].fZ -= fZRange*0.5f - fMinLength*0.33f;
	vHavokShapeTranslation[2].fY += fYRange*0.5f - fMinLength*0.33f;
	vHavokShapeTranslation[3].fY -= fYRange*0.5f - fMinLength*0.33f;
	vHavokShapeTranslation[4].fX += fXRange*0.5f - fMinLength*0.33f;
	vHavokShapeTranslation[5].fX -= fXRange*0.5f - fMinLength*0.33f;

	int nExtension = 0; //In case we decide to use less than 6 mopps.

	for(int n = 0; n < 6; n++)
	{
		VECTOR * pNewVertices = (VECTOR *)MALLOC(NULL, nVertexCount*sizeof(VECTOR));

		for(i = 0; i < nVertexCount; i++) pNewVertices[i] = 
				((VECTOR*)pVertices)[i] - vHavokShapeTranslation[n];


		ext[2] = ('0' + nExtension);
		PStrReplaceExtension(pszMoppFileName, MAX_XML_STRING_LENGTH, pszFilename, ext);

		HavokSaveMoppCode(pszMoppFileName,nVertexStructSize, nVertexBufferSize,
			nVertexCount, pNewVertices, nIndexBufferSize, nTriangleCount, pwIndexBuffer);

		FREE(NULL, pNewVertices);



		nExtension++;
	}

	return nExtension;
}*/



/*int HavokSaveMultiMoppCode( //returns number of moppfiles created.
					   const char * pszFilename, 
					   int nVertexStructSize, 
					   int nVertexBufferSize, 
					   int nVertexCount, 
					   void * pVertices, 
					   int nIndexBufferSize, 
					   int nTriangleCount, 
					   WORD * pwIndexBuffer,
					   VECTOR vHavokShapeTranslation[])
{
	char pszMoppFileName[MAX_XML_STRING_LENGTH];
	char ext[4] = "mop";

	//if(bDoNotSliceUp)  //May have terrain where we don't want/need this approach.
	//{
	//	PStrReplaceExtension(pszMoppFileName, MAX_XML_STRING_LENGTH, pszFilename, ext);

	//	HavokSaveMoppCode(pszMoppFileName,nVertexStructSize,nVertexBufferSize, 
	//		nVertexCount, pVertices, nIndexBufferSize, nTriangleCount, pwIndexBuffer);

		//	return;
	//}//Just calling both functions in c_granny_rigid.

	VECTOR directions[6] = {VECTOR( 0, 0, -1), VECTOR( 0, 0, 1),
		VECTOR( -1, 0, 0), VECTOR( 1, 0, 0),
		VECTOR( 0, -1, 0), VECTOR( 0, 1, 0) };
	//Photography angles.

	float fDepths[6] = {.66f, .66f, .66f, .66f, .66f, .66f};
	//How deep our snapshot looks compared to min(fYRange, fZRange).  Set low for just facade.

	orthogonalProjection * pProjected = 
		(orthogonalProjection *)MALLOCZ(NULL, nVertexCount*sizeof(orthogonalProjection));

	int * vectorMapping = (int *) MALLOC(NULL, nVertexCount*sizeof(int));

	int i;

	for(i = 0; i < nVertexCount; i++)
	{
		pProjected[i].original = ((VECTOR *)(pVertices))[i];
	}

	int nExtension = 0;

	for(int n = 0; n < 6; n++)
	{//create a bunch of orthogonal projections, sort them, use them to throw out points, feed remaining points
		VECTOR * pNewVertices = (VECTOR *)MALLOCZ(NULL, nVertexCount*sizeof(VECTOR));
		int nNewVertexCount = 0;

		memset(vectorMapping, 0xff, nVertexCount*sizeof(int)); //sets to -1, might want to just do a loop

		int j;

		for(j = 0; j < nVertexCount; j++) 
			//pProjected[j] = orthogonalProjection
			pProjected[j].makeProjection
			(directions[n], directions[(n+2)%6], directions[(n+4)%6]), 
			pProjected->eExclusion = INCLUDED;

		//std::sort(pProjected, pProjected + nVertexCount);

		float fXMax =-999999.0f, fYMax =-999999.0f, fZMax =-999999.0f;
		float fXMin = 999999.0f, fYMin = 999999.0f, fZMin = 999999.0f;

		//Actually, we just sorted it by fX, so:
		//fXMax = pProjected[nVertexCount-1].projection.fX;
		//fXMin = pProjected[0].projection.fX;
		//nuts to sorting.

		for(i = 0; i < nVertexCount; i++)
		{
			if(fYMax < pProjected[i].projection.fY) fYMax = pProjected[i].projection.fY;
			if(fYMin > pProjected[i].projection.fY) fYMin = pProjected[i].projection.fY;
			if(fZMax < pProjected[i].projection.fZ) fZMax = pProjected[i].projection.fZ;
			if(fZMin > pProjected[i].projection.fZ) fZMin = pProjected[i].projection.fZ;
			if(fXMax < pProjected[i].projection.fX) fXMax = pProjected[i].projection.fX;
			if(fXMin > pProjected[i].projection.fX) fXMin = pProjected[i].projection.fX;
		}
		float fMinLength = min( ABS(fYMax - fYMin), ABS(fZMax - fZMin) );

		fMinLength = min(ABS(fXMax - fXMin), fMinLength); //A design decision which splits
		//even moppfiles which would work unsplit.  Unnecessary, but it helps for testing.

		float fMaxLength = max( ABS(fYMax - fYMin), ABS(fZMax - fZMin) );
		fMaxLength = min(ABS(fXMax - fXMin), fMaxLength);


		float fXLimit = fXMin + fDepths[n]*fMinLength;

		//figure out the translation necessary to make Xmin negative, but smaller in
		//magnitude than the other directions.  Note that this may be a special case
		//for SoundStage, and other cases where we look at the outside may need Xmin 
		//to be positive, but still smaller in magnitude than the others.

		//In postprocessing, we'll add this translation (de-orthgonalized) to each
		//and every vector, then set vHavokShapeTranslation to its negative.

		//We'll center fY and fZ, then set up fXMin to be 2/3s of the minimum magnitude.
		float fXMinGoal = -fMinLength*(0.5f)*fDepths[n];
		float fYMinGoal = -0.5f*ABS(fYMax - fYMin);
		float fZMinGoal = fZMinGoal = 0.5f*ABS(fZMax - fZMin);

		VECTOR vProjectedTranslation(fXMinGoal-fXMin, fYMinGoal - fYMin, fZMinGoal - fZMin);
		VECTOR vOriginalTranslation = invertProjection(vProjectedTranslation, 
									directions[n], directions[(n+2)%6], directions[(n+4)%6]);


		//CHash<float> PointHash;
		//work in progress: abandoning the hash approach for a more robust
		//granny-based occlusion test.  For now, not even using occlusion,
		//and just doing cutoffs.

		for(i = 0; i < nVertexCount; i++)
		{
			//if ( (pProjected[i]).projection.fX > fXLimit) 
			//{
			//	(pProjected[i]).eExclusion = CUTOFF;
			//	continue;
			//} //Turning off cutoffs and relying PURELY on translation.

			//if(hash.find( hashed(y,z) ) && hash[hashed(y,z)] < (z-RESOLUTION*MAX_SLOPE) ) continue;
			//int nHashCoord = HavokPositionHashFunction(pProjected->projection.fY,pProjected->projection.fZ);

			//if(float nHashX = *(HashGet(PointHash, nHashCoord)) )
			{
				//either continue if it's too low, or perturb.
				//if(pProjected->projection.fX - nHashX > RESOLUTION*MAX_SLOPE) continue;
				//I'll write the perturb option when/if this fails.  It needs normals, and thus surgery.
			}
			//hash[hashed(y,z)] = x;
			//else 
			//{
				//float *pTmp = //HashAdd(PointHash, nHashCoord);
				//*pTmp = pProjected->projection.fX;
			//}

			//That is, if the point is occluded by a mesh point.
			//Another possibility is to perturb the point very slightly based on height and normal:
			//Towards the center of the pit if in the bottom of the pit, away if at the top.

			pNewVertices[nNewVertexCount] = pProjected[i].original;
			vectorMapping[i] = nNewVertexCount;
			nNewVertexCount++;
			pProjected[i].bMarked = true;
		}
		//now that we have thrown out a bunch of vertices, go through the index buffer
		//and truncate or remove any triangles which don't have all their vertices.

		//Mark 1 approach: simple removal of anything with non-included vertex, 
			//combined with re-mapping the vertex numbers correctly.

		int nNewIndexBufferSize = 0; 
		int nNewTriangleCount   = 0;
		WORD * pwNewIndexBuffer = (WORD*) MALLOC(NULL, nIndexBufferSize*sizeof(WORD));

		for(i = 0; i < nTriangleCount; i++)
		{//runs through the index buffer throwing out or keeping based on vectorMapping
			int nVertexIndex;
			for(int j = 0; j < 3; j++)
			{
				nVertexIndex= pwIndexBuffer[i*3 + j];
				if(vectorMapping[nVertexIndex] == -1) goto nextTriangle; //continue one level up
				pwNewIndexBuffer[nNewTriangleCount*3 + j] = vectorMapping[nVertexIndex];
			}
			nNewTriangleCount++;
			nNewIndexBufferSize+= 3 * sizeof( WORD );// INDEX_BUFFER_TYPE );

nextTriangle: continue;
		}

		//Begin postprocessing.
		//Here we will replace cliffs with walls with thickness, and drops with walls with thickness.
		//We look at triangles whose normals are entirely perpendicular to viewing direction.
		//We then extend the major edge of these with a rectangle of width at least maxdimension/100
		//Postprocessing will ignore projections and work directly with Z.
		//if(post_processing_works)
		

			//Postprocessing could potentially create a lot of vertices and triangles.
		int nPostTriangleCount = nNewTriangleCount;
		int nPostVertexCount = nNewVertexCount;
		int nPostIndexBufferSize = nNewIndexBufferSize;
		//if(false) //turn off postprocessing.
		{
			pwNewIndexBuffer = (WORD *)REALLOC(NULL, pwNewIndexBuffer, 3*nNewIndexBufferSize);
			pNewVertices = (VECTOR *)REALLOC(NULL, pNewVertices, 
				(nNewVertexCount + 2*nNewTriangleCount)*sizeof(VECTOR));

			
			float fLedgeWidth = -fMaxLength/30.0f; //Could go as low as 100.0 if the sampler is 128 x 128
			//Keeping it high for test purposes
			for(i = 0; i < nNewTriangleCount; i++)
			{
				VECTOR vNormal = VectorTriangleNormal(pNewVertices[pwNewIndexBuffer[3*i]],
					pNewVertices[pwNewIndexBuffer[3*i+1]], pNewVertices[pwNewIndexBuffer[3*i+2]]);
				if(  FABS(vNormal.fZ) < EPSILON && VectorLength(vNormal) > EPSILON)
					for(int k = 0; k < 3; k++)
				{
					int nVertexIndex1 = pwNewIndexBuffer[3*i + k];
					int nVertexIndex2 = pwNewIndexBuffer[3*i + ((k+1)%3)];
					if(ABS (pNewVertices[nVertexIndex2].fZ - pNewVertices[nVertexIndex1].fZ)
						< EPSILON) //flat edge of the triangle
					{
						//Create ledge vertices
						int nVertexIndex3 = nPostVertexCount;
						int nVertexIndex4 = nPostVertexCount + 1;
						VECTOR vDisplacement;
						VectorCross(vDisplacement, //swapped the last two arguements to switch direction.
							VECTOR(0.0f, 0.0f, 1.0f), pNewVertices[nVertexIndex1] - pNewVertices[nVertexIndex2]);
						VectorNormalize(vDisplacement);
						vDisplacement *= fLedgeWidth;

						pNewVertices[nVertexIndex3] = pNewVertices[nVertexIndex1] + vDisplacement;
						pNewVertices[nVertexIndex4] = pNewVertices[nVertexIndex2] + vDisplacement;

						//Create triangles to make ledge.  Note that it's possible I'm entering
						//them in the wrong order.

						pwNewIndexBuffer[3*nPostTriangleCount    ] = nVertexIndex1;
						pwNewIndexBuffer[3*nPostTriangleCount + 1] = nVertexIndex4;
						pwNewIndexBuffer[3*nPostTriangleCount + 2] = nVertexIndex3;
						pwNewIndexBuffer[3*nPostTriangleCount + 3] = nVertexIndex1;
						pwNewIndexBuffer[3*nPostTriangleCount + 4] = nVertexIndex2;
						pwNewIndexBuffer[3*nPostTriangleCount + 5] = nVertexIndex4;
						
						nPostVertexCount += 2;
						nPostTriangleCount += 2;
						nPostIndexBufferSize += 6*sizeof(WORD);
					}
				}
			}



		}

		//Vertex shift postprocess
		for(i = 0; i < nNewVertexCount; i++) pNewVertices[i] += vOriginalTranslation;
		

		


		ext[2] = ('0' + nExtension);
		PStrReplaceExtension(pszMoppFileName, MAX_XML_STRING_LENGTH, pszFilename, ext);//"mop");

//		pwNewIndexBuffer = (WORD *)REALLOC(NULL, pwNewIndexBuffer, nNewIndexBufferSize);//*sizeof(WORD);
//		pNewVertices = (VECTOR *)REALLOC(NULL, pNewVertices, nNewVertexCount*sizeof(VECTOR));
		//Unnecessary since we're just going to free it in a second anyway.

		int nNewVertexBufferSize = nNewVertexCount*nVertexStructSize;
		int nPostVertexBufferSize = nPostVertexCount*nVertexStructSize;

		//Debug test: shift every vertex down a bunch to see if we can get walls working.
		//for(int jj = 0; jj < nPostVertexCount; jj++) pNewVertices[jj].fZ+=8.0f;

		//Note that even though we've added stuff onto the end, using NewTriangleCount et al. 
		//will do the same thing and ignore postprocessing, if you want to do that instead.

		if(nNewTriangleCount > 0)
		{
			HavokSaveMoppCode(pszMoppFileName,nVertexStructSize,nPostVertexBufferSize, 
			nPostVertexCount, pNewVertices, nPostIndexBufferSize, nPostTriangleCount, pwNewIndexBuffer);
			//HavokSaveMoppCode(pszMoppFileName,nVertexStructSize,nNewVertexBufferSize, 
			//nNewVertexCount, pNewVertices, nNewIndexBufferSize, nNewTriangleCount, pwNewIndexBuffer);
			vHavokShapeTranslation[nExtension] = -vOriginalTranslation;
			nExtension ++;
		}
		//else //For now, we're just feeding in the entire thing.  In future, we'll just skip it.
			//HavokSaveMoppCode(pszMoppFileName,nVertexStructSize, nVertexBufferSize,
			//	nVertexCount, pVertices, nIndexBufferSize, nTriangleCount, pwIndexBuffer); //skipping it.

		
		

		FREE(NULL, pwNewIndexBuffer);
		FREE(NULL, pNewVertices); //Note that because the realloc is unnecessary, it's more efficient
		//to make and free these one level up.  However, I'm going to get it working before optimizing.
		//Of course, if the save works on a thread, as a disk-accessor might, then I can't FREE squat.

	}

	//Check that every point is done.
	int nUsedVertices = 0;
	for(i = 0; i < nVertexCount; i++) if (pProjected[i].bMarked == true) nUsedVertices++;

	FREE(NULL, pProjected);
	FREE(NULL, vectorMapping);

	ASSERT(nUsedVertices == nVertexCount);

	return nExtension; //trying a different count for debug purposes, and 1 is all we need for soundgarden
	//return 6; //Right now we're always doing all 6.  In future we may put a break in if everything
	//is marked, and return n;
}*/

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
hkMoppCode*	HavokLoadMoppCode( 
	const char* pszFilename )
{	//moving extension one level up.
	//char pszMoppFileName[MAX_XML_STRING_LENGTH];
	//PStrReplaceExtension(pszMoppFileName, MAX_XML_STRING_LENGTH, pszFilename, "mop");

	DECLARE_LOAD_SPEC( spec, pszFilename/*pszMoppFileName*/ );
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
	void * pFileData = PakFileLoadNow(spec);

	// CHB 2006.06.21 - File not found.
	ASSERT_RETNULL(pFileData != 0);

	hkMemoryStreamReader tStreamReader( pFileData, spec.bytesread, hkMemoryStreamReader::MEMORY_INPLACE );

	// Create an archive from the streambuf
	hkIArchive inputArchive( &tStreamReader );

	// Read mopp data in. This method allocates the hkMoppCode too. N.B. The MOPP actually holds its
	// data *immediately* after the class itself, so the number of bytes actually allocated is NOT
	// sizeof(hkMoppCode), but rather sizeof(hkMoppCode) + [number bytes required for byte data]
	// which is stored in the class as m_totalSize. You can query this using the getSize() method of
	// hkMoppCode. The code initially has reference count of 1.

	hkMoppCode* code = hkMoppCodeStreamer::readMoppCodeFromArchive(inputArchive);			

	HK_ASSERT2(0, code != HK_NULL, "hkMoppCode failed to read in correctly from Input Archive. Is the filename/path correct? Is the file corrupt?");

	FREE( NULL, pFileData );

	return code;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokCreateShapeFromMesh( 
	HAVOK_SHAPE_HOLDER & tHavokShapeHolder,
	const char * pszFilename, 
	int nVertexStructSize, 
	int nVertexBufferSize, 
	int nVertexCount, 
	void * pVertices, 
	int nIndexBufferSize, 
	int nTriangleCount, 
	WORD * pwIndexBuffer )
{
	char pszMoppFilename[MAX_XML_STRING_LENGTH];
	PStrReplaceExtension(pszMoppFilename, MAX_XML_STRING_LENGTH, pszFilename, "mop");

	hkStorageMeshShape * pStorageMeshShape = sCreateStorageMeshShape( nVertexStructSize, nVertexBufferSize, nVertexCount, pVertices, nIndexBufferSize, nTriangleCount, pwIndexBuffer );
	hkMoppCode * pMopp = HavokLoadMoppCode( pszMoppFilename );

	ASSERT( pMopp );

	// Use the shape and code to create a bvtree shape.
	hkMoppBvTreeShape* moppBvTree = new hkMoppBvTreeShape( pStorageMeshShape, pMopp );

	// Give mopp code ownership to bvtree shape.
	pMopp->removeReference();
	pStorageMeshShape->removeReference();

	sHavokShapeHolderInit( tHavokShapeHolder, moppBvTree );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokCreateShapeSphere( 
	HAVOK_SHAPE_HOLDER & tHavokShapeHolder,
	float fRadius )
{
	hkShape * pShape = new hkSphereShape ( fRadius );

	sHavokShapeHolderInit( tHavokShapeHolder, pShape );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokCreateShapeBox( 
	HAVOK_SHAPE_HOLDER & tHavokShapeHolder,
	VECTOR & vSize )
{
	hkVector4 vHalfExtents;
	vHalfExtents.set( vSize.fX / 2.0f, vSize.fY / 2.0f, vSize.fZ / 2.0f);
	ASSERT( vSize.fX > 0.0f && vSize.fY > 0.0f && vSize.fZ > 0.0f );
	hkShape * pShape = new hkBoxShape ( vHalfExtents );

	sHavokShapeHolderInit( tHavokShapeHolder, pShape );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokReleaseShape( 
	HAVOK_SHAPE_HOLDER & tHavokShapeHolder )
{
	if ( tHavokShapeHolder.pHavokShape )
	{
		tHavokShapeHolder.pHavokShape->m_memSizeAndFlags = (hkUint16) tHavokShapeHolder.dwMemSizeAndFlags;
		tHavokShapeHolder.pHavokShape->m_referenceCount = 1; 
		tHavokShapeHolder.pHavokShape->removeReference();
		tHavokShapeHolder.pHavokShape = NULL;
	}
}

class CUnitSoundCollisionListener : public hkCollisionListener
{
public:

	/*
	* Construction and Destruction
	*/

	CUnitSoundCollisionListener( ):
		m_newContactDistanceThreshold( 2.0f ),
		m_crashTolerance( 5.0f ),
		m_bounceTolerance( 10.0f ),
		m_normalSeparationVelocity( 0.01f ),
		m_slidingThreshold( 0.5f ),
		m_rollingContactMaxVelocity( 0.1f ),
		m_offNormalCOMRelativeVelocityThreshold( 0.5f )
	  {
		  nGoreId = GlobalIndexGet( GI_SOUND_ITEM_CRASH );
		  nGoreId2 = GlobalIndexGet( GI_SOUND_ITEM_BOUNCE );
	  }

	  /*
	  * Members from base class hkCollisionListener which must be implemented:
	  */

	  // Called after a contact point was added 
	  void contactPointAddedCallback(	hkContactPointAddedEvent& event )
	  {
		  // If the relative velocity of the objects projected onto the contact normal is greater than m_crashTolerance
		  // it could be a bounce or a crash, we check if the new point is sufficiently distant from other existing
		  // contacts and if so we register a bounce / crash
		  //trace("id=%d vel=%f\n", event.m_contactPointId, abs(event.m_projectedVelocity));
		  //if ( abs(event.m_projectedVelocity) > m_crashTolerance )
		  //{
			 // hkBool playSound = true;
			 // hkArray<hkContactPointId> pointIds;
			 // event.m_contactMgr.getAllContactPointIds( pointIds );

			 // hkBool printCrash = true;
			 // for( int i=0; i < pointIds.getSize(); i++ )
			 // {
				//  if ( pointIds[i] != event.m_contactPointId )
				//  {
				//	  const hkVector4& contactPos = event.m_contactPoint->getPosition();
				//	  const hkVector4& otherContactPos = event.m_contactMgr.getContactPoint( pointIds[i] )->getPosition();
				//	  hkVector4 diff;
				//	  diff.setSub4( contactPos, otherContactPos );
				//	  if ( diff.lengthSquared3() < m_newContactDistanceThreshold )
				//	  {
				//		  playSound = false;
				//		  break;
				//	  }
				//  }
			 // }
			 // if ( playSound )
			 // {
				//  if ( abs(event.m_projectedVelocity) > m_bounceTolerance )
				//  {
				//	  //hkcout << "BOUNCE!\n";
				//	  VECTOR pos;
				//	  //VECTOR vel;
				//	  hkVector4 position = event.m_contactPoint->getPosition();
				//	  pos.fX = position(0);
				//	  pos.fY = position(1);
				//	  pos.fZ = position(2);
				//	  /*
				//	  hkVector4 velocity = event.m_projectedVelocity->getPosition();
				//	  vel.fX = velocity(0);
				//	  vel.fY = velocity(1);
				//	  vel.fZ = velocity(2);
				//	  // */
				//	  //trace("Havok Bounce id=%d vel=%f\n", event.m_contactPointId, abs(event.m_projectedVelocity));
				//	  c_SoundPlay( nGoreId2, &pos );
				//  }
				//  else
				//  {
				//	  //hkcout << "CRASH!\n";
				//	  VECTOR pos;
				//	  //VECTOR vel;
				//	  hkVector4 position = event.m_contactPoint->getPosition();
				//	  pos.fX = position(0);
				//	  pos.fY = position(1);
				//	  pos.fZ = position(2);
				//	  /*
				//	  hkVector4 velocity = event.m_contactPoint->getPosition();
				//	  vel.fX = velocity(0);
				//	  vel.fY = velocity(1);
				//	  vel.fZ = velocity(2);
				//	  // */
				//	  //trace("Havok Crash id=%d vel=%f\n", event.m_contactPointId, abs(event.m_projectedVelocity));
				//	  c_SoundPlay( nGoreId, &pos );
				//  }
			 // }
		  //}
	  }

	  void contactPointConfirmedCallback( hkContactPointConfirmedEvent& event) {};

	  // Called before a contact point gets removed. We do not implement this for this demo.
	  void contactPointRemovedCallback( hkContactPointRemovedEvent& event )
	  { }

	  // Called just before the collisionResult is passed to the constraint system (solved).
	  void contactProcessCallback( hkContactProcessEvent& event )
	  {
		  //hkRigidBody* rbA = static_cast<hkRigidBody*> (event.m_collidableA.getOwner());
		  //hkRigidBody* rbB = static_cast<hkRigidBody*> (event.m_collidableB.getOwner());

		  //hkProcessCollisionOutput& result = event.m_collisionResult;
		  //for (int i = 0; i < HK_MAX_CONTACT_POINT; i++ )
		  //{
			 // const hkVector4& start = result.m_contactPoints[i].m_contact.getPosition();
			 // hkVector4 normal = result.m_contactPoints[i].m_contact.getNormal();

			 // // Get (relative) velocity of contact point. It's arbitrary whether we choose A-B or B-A
			 // hkVector4 velPointOnA, velPointOnB, worldRelativeVelocity;
			 // rbA->getPointVelocity(start, velPointOnA);
			 // rbB->getPointVelocity(start, velPointOnB);
			 // worldRelativeVelocity.setSub4(velPointOnB, velPointOnA);

			 // // Get velocity in normal direction
			 // hkReal velocityInNormalDir = worldRelativeVelocity.dot3(normal);
			 // hkReal velocityOffNormal = 0.0f;

			 // // And velocity vector not in normal: velocity - (normal * velProjectedOnNormal)
			 // {
				//  hkVector4 offNormalVelocityVector;
				//  offNormalVelocityVector.setMul4(velocityInNormalDir, normal);
				//  offNormalVelocityVector.setSub4(worldRelativeVelocity, offNormalVelocityVector);
				//  velocityOffNormal = offNormalVelocityVector.length3();
			 // }

			 // // If the bodies have a minimal separating velocity (along the contact normal) 
			 // // and they have a sufficient velocity off normal then we identify a sliding contact
			 // if (	(hkMath::fabs(velocityInNormalDir) < m_normalSeparationVelocity ) 
				//  && (velocityOffNormal > m_slidingThreshold) ) 
			 // {
				//  //hkcout << velocityOffNormal << " SLIDING\n";
				//  VECTOR pos;
				//  //VECTOR vel;
				//  hkVector4 position = result.m_currentContactPoint->m_contact.getPosition();
				//  pos.fX = position(0);
				//  pos.fY = position(1);
				//  pos.fZ = position(2);
				//  /*
				//  hkVector4 velocity = event.m_contactPoint->getPosition();
				//  vel.fX = velocity(0);
				//  vel.fY = velocity(1);
				//  vel.fZ = velocity(2);
				//  // */
				//  //trace("Slide\n");
				//  //c_SoundPlay( nGoreId, &pos );
			 // }
			 // else
			 // {
				//  // Test for rolling, first see if the relative velocity of the bodies at the contact point is below
				//  // a given threshold: for rolling contacts the effect of the angular velocity of the body and the linear
				//  // velocity tend to cancel each other out giving the contact point a zero world velocity.
				//  // Once this has been satisfied we look to see if the component of the relative velocity of the bodies' 
				//  // centers of mass not in the direction of the contact normal is above another threshold and if so
				//  // we recognise a rolling contact. The defaults for these values are arbitrary and should be tweaked to 
				//  // achieve a satisfactory result for each use case.
				//  if (worldRelativeVelocity.length3() < m_rollingContactMaxVelocity)
				//  {
				//	  hkVector4 relativeVelocityOfCOMs;
				//	  relativeVelocityOfCOMs.setSub4(rbB->getLinearVelocity(), rbA->getLinearVelocity());

		  //	  hkReal relativeCOMVelocityInNormalDir = relativeVelocityOfCOMs.dot3(normal);

				//	  // Find component of centres of mass relative velocity not in direction of contact normal
				//	  hkVector4 velCOMNotInNormal;
				//	  {
				//		  velCOMNotInNormal.setMul4(relativeCOMVelocityInNormalDir, normal);
				//		  velCOMNotInNormal.setSub4(relativeVelocityOfCOMs, velCOMNotInNormal);
				//	  }

				//	  if (velCOMNotInNormal.length3() > m_offNormalCOMRelativeVelocityThreshold)
				//	  {
				//		  //hkcout <<  velCOMNotInNormal.length3() <<" ROLLING\n";
				//		  VECTOR pos;
				//		  //VECTOR vel;
				//		  hkVector4 position = result.m_currentContactPoint->m_contact.getPosition();
				//		  pos.fX = position(0);
				//		  pos.fY = position(1);
				//		  pos.fZ = position(2);
				//		  /*
				//		  hkVector4 velocity = event.m_contactPoint->getPosition();
				//		  vel.fX = velocity(0);
				//		  vel.fY = velocity(1);
				//		  vel.fZ = velocity(2);
				//		  // */
				//		  //trace("Roll\n");
				//		  //c_SoundPlay( nGoreId, &pos );
				//	  }
				//  }
			 // }
		  //}
	  }

private:

	//
	// Add Contact Values
	//

	// If new contacts are less than this distance from an existing contact then they are ignored 
	// and do not generate a crash/ bounce.
	hkReal m_newContactDistanceThreshold;

	// These two values look at the projected velocity of the bodies' relative velocity onto the contact normal 
	// If it is above m_crashTolerance and passes m_newContactDistanceThreshold then it either generates
	// A crash or a bounce depending on whether or not it exceeds m_bounceTolerance (bounce > crash)
	hkReal m_crashTolerance;
	hkReal m_bounceTolerance;

	//
	// Process Contact Values
	//

	// When processing contacts if the bodies are not separating ( < m_normalSeparationVelocity is the relative 
	// velocity along the contact normal ) and the contact's velocity not in the direction of the contact normal
	// is greater than m_slidingThreshold we recognise a sliding contact
	hkReal m_normalSeparationVelocity;
	hkReal m_slidingThreshold;

	// If the relative velocity of the bodies at the contact point is below m_rollingContactMaxVelocity 
	// and the component of the relative velocity of the bodies' centers of mass not in the direction of the 
	// contact normal is above m_offNormalCOMRelativeVelocityThreshold then we identify a Rolling contact.
	hkReal m_rollingContactMaxVelocity;
	hkReal m_offNormalCOMRelativeVelocityThreshold;

	// The ID of the temporary 'gore' Sound
	int nGoreId;
	int nGoreId2;
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
hkRigidBody * HavokCreateBackgroundRigidBody( 
	GAME * pGame, 
	hkWorld * pWorld,
	hkShape * pHavokShape, 
	VECTOR * pPosition, 
	QUATERNION * pRotation,
	BOOL bFXAdd, BOOL bFXOnly)
{
	ASSERT_RETNULL( pWorld );
	ASSERT_RETNULL( pHavokShape );

	hkRigidBodyCinfo rigidBodyInfo;
	rigidBodyInfo.m_shape = pHavokShape;

	// Compute the shapes inertia tensor

	//hkReal fMass = 0.1f;
	//hkMassProperties massProperties;
	//hkInertiaTensorComputer::computeShapeVolumeMassProperties( pHavokShape, fMass, massProperties );

	//rigidBodyInfo.m_mass = fMass;
	//rigidBodyInfo.m_inertiaTensor = massProperties.m_inertiaTensor;
	//rigidBodyInfo.m_centerOfMass  = massProperties.m_centerOfMass;
	rigidBodyInfo.m_linearDamping = 0.05f;
	rigidBodyInfo.m_friction = 0.95f;

	rigidBodyInfo.m_motionType = hkMotion::MOTION_FIXED;

	rigidBodyInfo.m_position.set( pPosition->fX, pPosition->fY, pPosition->fZ );
	if ( pRotation )
		rigidBodyInfo.m_rotation.m_vec.set( pRotation->x, pRotation->y, pRotation->z, pRotation->w );

	hkRigidBody * pRigidBody = new hkRigidBody(rigidBodyInfo);
	if(!bFXOnly)
	{
#if USE_HAVOK_4
		pRigidBody->setCollisionFilterInfo( hkGroupFilter::calcFilterInfo( COLFILTER_BACKGROUND_LAYER, COLFILTER_BACKGROUND_SYSTEM ) );
#else
		pRigidBody->getCollidable()->setCollisionFilterInfo( hkGroupFilter::calcFilterInfo( COLFILTER_BACKGROUND_LAYER, COLFILTER_BACKGROUND_SYSTEM ) );
#endif
		//CUnitSoundCollisionListener * pListener = new CUnitSoundCollisionListener;
		//pRigidBody->addCollisionListener( pListener );
		pWorld->addEntity( pRigidBody );
	}

	if(bFXAdd) e_HavokFXAddH3RigidBody( pRigidBody, TRUE, FX_BRIDGE_BODY_SAMPLEDIR_Z);
//always doing a vertical projection unless something comes up
	return pRigidBody;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
hkRigidBody * HavokCreateUnitRigidBody( 
	GAME * pGame, 
	hkWorld * pWorld,
	hkShape * pHavokShape, 
	const VECTOR & vPosition, 
	const VECTOR & vFaceDirection,
	const VECTOR & vUpDirection,
	float fBounce,
	float fDampening,
	float fFriction )
{
	ASSERT_RETNULL( pHavokShape );
	hkRigidBodyCinfo rigidBodyInfo;
	rigidBodyInfo.m_shape = pHavokShape;

	// Compute the shapes inertia tensor

	hkReal fMass = 1.0f;
	hkMassProperties massProperties;
	hkInertiaTensorComputer::computeShapeVolumeMassProperties( pHavokShape, fMass, massProperties );

	rigidBodyInfo.m_mass = fMass;
	rigidBodyInfo.m_inertiaTensor = massProperties.m_inertiaTensor;
	rigidBodyInfo.m_centerOfMass  = massProperties.m_centerOfMass;
	rigidBodyInfo.m_friction	   = fFriction  != 0.0f ? fFriction  : 0.5f;
	rigidBodyInfo.m_linearDamping  = fDampening != 0.0f ? fDampening : 0.5f;
	rigidBodyInfo.m_angularDamping = fDampening != 0.0f ? fDampening : 0.5f;
	rigidBodyInfo.m_restitution = fBounce;
	rigidBodyInfo.m_restitution = min( rigidBodyInfo.m_restitution, 1.9f );
	rigidBodyInfo.m_restitution = max( rigidBodyInfo.m_restitution, 0.0f );

	rigidBodyInfo.m_motionType = hkMotion::MOTION_BOX_INERTIA;

	rigidBodyInfo.m_position.set( vPosition.fX, vPosition.fY, vPosition.fZ );

	rigidBodyInfo.m_qualityType = HK_COLLIDABLE_QUALITY_MOVING;

	// set the rotation
	ASSERT_RETNULL( !VectorIsZero( vFaceDirection ) );
	ASSERT_RETNULL( !VectorIsZero( vUpDirection ) );

	MATRIX mRotation;
	MatrixFromVectors( mRotation, VECTOR(0), vUpDirection, vFaceDirection );

	hkTransform	mHavokTransform;
	mHavokTransform.set4x4ColumnMajor( (float *) mRotation );

	rigidBodyInfo.m_rotation.set( mHavokTransform.getRotation() );

	hkRigidBody * pRigidBody = new hkRigidBody(rigidBodyInfo);
#if USE_HAVOK_4
	pRigidBody->setCollisionFilterInfo( hkGroupFilter::calcFilterInfo( COLFILTER_UNIT_LAYER, COLFILTER_UNIT_SYSTEM) );
#else
	pRigidBody->getCollidable()->setCollisionFilterInfo( hkGroupFilter::calcFilterInfo( COLFILTER_UNIT_LAYER, COLFILTER_UNIT_SYSTEM) );
#endif
	//CUnitSoundCollisionListener * pListener = new CUnitSoundCollisionListener;
	//pRigidBody->addCollisionListener( pListener );

	if ( pWorld )
		pWorld->addEntity( pRigidBody );

	return pRigidBody;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokReleaseRigidBody( 
	GAME * pGame, 
	hkWorld * pWorld,
	hkRigidBody * pBody )
{
	if ( ! pBody )
		return;

	const hkArray<hkCollisionListener*>& pListeners = pBody->getCollisionListeners();
	for ( int i = 0; i < pListeners.getSize(); i++ )
	{
		// CHB 2007.03.30 - Cache a copy of the pointer first,
		// because the call to removeCollisionListener() modifies
		// the listener array (setting the entry to NULL) thus
		// causing the delete to accomplish nothing.  This leads
		// to memory leaks.
		hkCollisionListener * pCL = pListeners[ i ];
		pBody->removeCollisionListener( pCL );
		delete pCL;
	}
	hkWorld * pBodyWorld = pBody->getWorld();
	if ( pBodyWorld == pWorld && pWorld )
	{
		pWorld->removeEntity( pBody );
	} else {
		if ( pBodyWorld )
			pBodyWorld->removeEntity( pBody );
		if ( pWorld )
			pWorld->removeEntity( pBody );
	}
	pBody->removeReference();
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokReleaseInvisibleRigidBody( 
						   GAME * pGame, 
						   hkWorld * pWorld,
						   hkRigidBody * pBody )
{
	if ( ! pBody )
		return;

	pBody->removeReference();

}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokRigidBodyRemoveFromWorld( 
	GAME * pGame, 
	hkWorld * pWorld,
	hkRigidBody * pHavokRigidBody )
{
	if ( pGame && pWorld && pHavokRigidBody )
		pWorld->removeEntity( pHavokRigidBody );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokRigidBodyAddToWorld( 
	GAME * pGame, 
	hkWorld * pWorld,
	hkRigidBody * pHavokRigidBody )
{
	if ( pGame && pWorld && pHavokRigidBody )
		pWorld->addEntity( pHavokRigidBody );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokRigidBodySetVelocity( 
	hkRigidBody * pRigidBody, 
	VECTOR * pVelocity)
{
	if ( pRigidBody && pVelocity )
	{
		hkVector4 vVelocity;
		vVelocity.set( pVelocity->fX, pVelocity->fY, pVelocity->fZ, 0.0f );
		if ( vVelocity.isOk3() )
			pRigidBody->setLinearVelocity( vVelocity );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokRigidBodyGetVelocity( 
	hkRigidBody * pRigidBody,
	VECTOR & vVelocity)
{
	if ( ! pRigidBody )
	{
		vVelocity = VECTOR( 0 );
		return;
	}

	hkVector4 vHavokVelocity = pRigidBody->getLinearVelocity();

	vVelocity.fX = vHavokVelocity.getSimdAt( 0 );
	vVelocity.fY = vHavokVelocity.getSimdAt( 1 );
	vVelocity.fZ = vHavokVelocity.getSimdAt( 2 );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokRigidBodyGetPosition( 
	hkRigidBody * pRigidBody, 
	VECTOR * pPosition)
{
	if ( pRigidBody && pPosition )
	{
		hkVector4 vHavokPosition = pRigidBody->getPosition();
		pPosition->fX = vHavokPosition(0);
		pPosition->fY = vHavokPosition(1);
		pPosition->fZ = vHavokPosition(2);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokRigidBodyWarp(
	GAME * pGame,
	hkWorld * pWorldNew,
	hkRigidBody * pRigidBody,
	const VECTOR & vPosition, 
	const VECTOR & vFaceDirection, 
	const VECTOR & vUpDirection )
{
	ASSERT_RETURN( pRigidBody );
	hkWorld * pWorldOld = pRigidBody->getWorld();
	if ( pWorldOld != pWorldNew )
	{
		if ( pWorldOld )
			pWorldOld->removeEntity( pRigidBody );
	}

	MATRIX mMatrix;
	MatrixFromVectors( mMatrix, vPosition, vUpDirection, vFaceDirection );

	ASSERT_RETURN( !VectorIsZero( vFaceDirection ) );
	ASSERT_RETURN( !VectorIsZero( vUpDirection ) );
	hkQsTransform	mHavokTransform;
	mHavokTransform.set4x4ColumnMajor( (float *) mMatrix );

	pRigidBody->setRotation( mHavokTransform.getRotation() );
	pRigidBody->setPosition( mHavokTransform.getTranslation() );

	if ( pWorldOld != pWorldNew )
	{
		if ( pWorldNew )
			pWorldNew->addEntity( pRigidBody );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokRigidBodyGetMatrix( 
	hkRigidBody * pRigidBody, 
	MATRIX & mWorld,
	const VECTOR & vOffset )
{
	hkTransform mHavokTransform = pRigidBody->getTransform();

	if ( ! VectorIsZero( vOffset ) )
	{
		hkVector4 vHavokVector;
		vHavokVector.set( -vOffset.fX, -vOffset.fY, -vOffset.fZ );
		hkTransform mOffsetTransform;
		mOffsetTransform.setIdentity();
		mOffsetTransform.setTranslation( vHavokVector );

		mHavokTransform.setMulEq( mOffsetTransform );

		mOffsetTransform.setTranslation( hkVector4( 0.0f, 0.0f, -0.1f ) );
		mHavokTransform.setMul( mOffsetTransform, mHavokTransform );
	}

	mHavokTransform.get4x4ColumnMajor( (float *)&mWorld );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define HAVOK_FORCE_MULTIPLIER 10.0f // This is to make forces in about the 1-10 range instead of hundreds
void HavokRigidBodyApplyImpact ( 
	RAND& rand,
	hkRigidBody * pRigidBody, 
	const HAVOK_IMPACT & tHavokImpact )
{ 
	const hkVector4 & vCenterOfMass = pRigidBody->getCenterOfMassInWorld();

	VECTOR vDelta;
	vDelta.fX = (float) vCenterOfMass( 0 ) - tHavokImpact.vSource.fX;
	vDelta.fY = (float) vCenterOfMass( 1 ) - tHavokImpact.vSource.fY;
	vDelta.fZ = (float) vCenterOfMass( 2 ) - tHavokImpact.vSource.fZ;
	VectorNormalize( vDelta, vDelta );

	VectorScale( vDelta, vDelta, tHavokImpact.fForce * HAVOK_FORCE_MULTIPLIER ); 
	hkVector4 vImpulse;
	vImpulse.set( vDelta.fX, vDelta.fY, vDelta.fZ );
	if ( vImpulse.isOk3() )
		pRigidBody->applyLinearImpulse( vImpulse );
	//LogMessage(PERF_LOG, "applying impact %f %f %f", vDelta.fX, vDelta.fY, vDelta.fZ);

	if ( tHavokImpact.dwFlags & HAVOK_IMPACT_FLAG_TWIST )
	{
		hkVector4 vAngular;
		vAngular.set( 6.0f, 1.0f, 1.0f, 1.0f );
		pRigidBody->setAngularVelocity( vAngular );
	}

	if ( ( tHavokImpact.dwFlags & HAVOK_IMPACT_FLAG_ON_CENTER ) == 0 )
	{
		VECTOR vRandom = RandGetVector(rand);
		VectorNormalize( vRandom );

		hkVector4 vAngular;
		vAngular.set( 6.0f, vRandom.fX, vRandom.fY, vRandom.fZ );
		if ( vAngular.isOk4() )
			pRigidBody->applyAngularImpulse( vAngular );
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokAddFloor(
	GAME * pGame,
	hkWorld * pWorld )
{
	VECTOR vFloorVec(40.0f, 40.0f, 2.0f);
	hkVector4 vHalfExtents;
	vHalfExtents.set( vFloorVec.fX / 2.0f, vFloorVec.fY / 2.0f, vFloorVec.fZ / 2.0f);
	hkShape * pShape = new hkBoxShape ( vHalfExtents );

	VECTOR vDownVec(0, 0, -1.0f);
	pFloor = HavokCreateBackgroundRigidBody( pGame, pWorld, pShape, &vDownVec, NULL, true );
	pFloor->removeReference();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokRemoveFloor(
	GAME * pGame,
	hkWorld * pWorld)
{
	if(pFloor)
	{
		// CHB 2007.03.14
		HavokReleaseRigidBody(pGame, pWorld, pFloor);
		//pWorld->removeEntity(pFloor);
		pFloor = NULL;
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class CUnitCollisionListener : public hkCollisionListener
{
public:
	///  Called after a contact point is added. 
	///  You can use this callback to:
	///	 - Override the friction and restitution values in the contact point's hkContactPointProperties.
	///  - Reject the contact point.
	///  - Trigger game events, sound effects etc.
	void contactPointAddedCallback(	hkContactPointAddedEvent& event)
	{
		if ( m_pfnUnitHit && m_idUnit != INVALID_ID && m_pGame )
		{
			UNIT * pUnit = UnitGetById( m_pGame, m_idUnit );

			if ( pUnit && event.m_bodyB.getShape()->getType() == HK_SHAPE_TRIANGLE )
			{
				const hkTriangleShape * pTriangle = (hkTriangleShape *) event.m_bodyB.getShape();
				const hkVector4* pVertices = pTriangle->getVertices();
				ASSERT_RETURN( pVertices );
				hkVector4 vDeltas[ 2 ];
				vDeltas[ 0 ].setSub4( pVertices[ 0 ], pVertices[ 1 ] );
				vDeltas[ 1 ].setSub4( pVertices[ 2 ], pVertices[ 1 ] );
				vDeltas[ 0 ].normalize3();
				vDeltas[ 1 ].normalize3();
				hkVector4 vNormal;
				vNormal.setCross( vDeltas[ 0 ], vDeltas[ 1 ] );
				vNormal.normalize3();

				hkVector4 vCollisionNormal = event.m_contactPoint->getNormal();
				if ( vNormal.dot3( vCollisionNormal ) < 0.0f )
					vNormal.setNeg3( vNormal );

				VECTOR vContactNormal;
				vContactNormal.fX = vNormal.getSimdAt( 0 );
				vContactNormal.fY = vNormal.getSimdAt( 1 );
				vContactNormal.fZ = vNormal.getSimdAt( 2 );

				hkVector4 vCollisionPosition = event.m_contactPoint->getPosition();
				VECTOR vContactPosition;
				vContactPosition.fX = vCollisionPosition.getSimdAt( 0 );
				vContactPosition.fY = vCollisionPosition.getSimdAt( 1 );
				vContactPosition.fZ = vCollisionPosition.getSimdAt( 2 );
				UnitSetPosition(pUnit, vContactPosition);

				m_pfnUnitHit( m_pGame, pUnit, vContactNormal, m_pCallbackData );
			}

			//if ( pUnit &&
			//	event.m_bodyB.getParent() && 
			//	event.m_bodyB.getParent()->getShape()->getType() == HK_SHAPE_MOPP )
			//{
			//	hkMoppBvTreeShape * pMoppShape = (hkMoppBvTreeShape *) event.m_bodyB.getParent()->getShape();

			//	VECTOR vUnitPos = UnitGetPosition( pUnit );
			//	VECTOR vUnitMoveDirection = UnitGetMoveDirection( pUnit );
			//	const UNIT_DATA * pUnitData = UnitGetData( pUnit );
			//	vUnitPos += vUnitMoveDirection * -pUnitData->fHitBackup;

			//	hkVector4 vCenter( vUnitPos.fX, vUnitPos.fY, vUnitPos.fZ );
			//	hkTransform tTransform = event.m_bodyB.getParent()->getTransform();
			//	vCenter.setTransformedInversePos( tTransform, vCenter );
			//	hkSphere tSphere( vCenter, 3.0f );

			//	hkArray<hkShapeKey> tSphereHits;
			//	pMoppShape->querySphere( tSphere, tSphereHits );

			//	float fLargestMinTriangleSizeSquared = 0.0f;
			//	float fClosestDistanceToLargeTriangle = tSphere.getRadius() + 1.0f;

			//	hkVector4 vMoveDir( vUnitMoveDirection.fX, vUnitMoveDirection.fY, vUnitMoveDirection.fZ );
			//	hkVector4 vNormal;
			//	vNormal.setNeg3( vMoveDir );

			//	hkShapeCollection::ShapeBuffer tShapeBuffer;
			//	for ( int i = 0; i < tSphereHits.getSize(); i++ )
			//	{
			//		const hkShape* pChildShape = pMoppShape->getShapeCollection()->getChildShape( tSphereHits[ i ], tShapeBuffer );
	
			//		if ( pChildShape->getType() == HK_SHAPE_TRIANGLE )
			//		{
			//			const hkTriangleShape * pTriangle = (hkTriangleShape *) pChildShape;

			//			float fDistanceToCenter;
			//			{
			//				hkCollideTriangleUtil::ClosestPointTriangleCache TriangleDistanceCache;
			//				hkCollideTriangleUtil::setupClosestPointTriangleCache( pTriangle->getVertices(), TriangleDistanceCache );

			//				hkCollideTriangleUtil::ClosestPointTriangleResult result;

			//				hkCollideTriangleUtil::closestPointTriangle( vCenter, pTriangle->getVertices(), TriangleDistanceCache, result );

			//				fDistanceToCenter = result.distance;
			//			}

			//			const hkVector4* pVertices = pTriangle->getVertices();
			//			ASSERT_RETURN( pVertices );
			//			hkVector4 vDeltas[ 3 ];
			//			vDeltas[ 0 ].setSub4( pVertices[ 0 ], pVertices[ 1 ] );
			//			vDeltas[ 1 ].setSub4( pVertices[ 2 ], pVertices[ 1 ] );
			//			vDeltas[ 2 ].setSub4( pVertices[ 2 ], pVertices[ 0 ] );

			//			float fSmallestSide = vDeltas[ 0 ].lengthSquared3();
			//			for ( int j = 1; j < 3; j++ )
			//			{
			//				float fLengthSquared = vDeltas[ j ].lengthSquared3();
			//				if ( fLengthSquared < fSmallestSide )
			//					fSmallestSide = fLengthSquared;
			//			}
			//			if ( fSmallestSide > fLargestMinTriangleSizeSquared || 
			//				 (fDistanceToCenter < fClosestDistanceToLargeTriangle && fLargestMinTriangleSizeSquared > 0.25f ) )
			//			{
			//				fLargestMinTriangleSizeSquared = fSmallestSide;
			//				fClosestDistanceToLargeTriangle = fDistanceToCenter;
			//				vDeltas[ 0 ].normalize3();
			//				vDeltas[ 1 ].normalize3();
			//				vNormal.setCross( vDeltas[ 0 ], vDeltas[ 1 ] );
			//				vNormal.normalize3();

			//				// face away from the the move direction
			//				if ( vNormal.dot3( vMoveDir ) > 0.0f )
			//					vNormal.setNeg3( vNormal );
			//			}
			//		}
			//	}

			//	vNormal.setTransformedPos( tTransform, vNormal );
			//	VECTOR vNormalOut;
			//	vNormalOut.fX = vNormal.getSimdAt( 0 );
			//	vNormalOut.fY = vNormal.getSimdAt( 1 );
			//	vNormalOut.fZ = vNormal.getSimdAt( 2 );
			//	m_pfnUnitHit( m_pGame, pUnit, vNormalOut, m_pCallbackData );
			//}
		}
	}

	void contactPointConfirmedCallback( hkContactPointConfirmedEvent& event) {};


	/// Called before a contact point gets removed.
	void contactPointRemovedCallback( hkContactPointRemovedEvent& event ) {};

	/// Called just before the collisionResult is passed to the constraint system. This callback allows you to change any information in the result. 
	void contactProcessCallback( hkContactProcessEvent& event ) {};

	FP_HAVOKHIT * m_pfnUnitHit;
	UNITID m_idUnit;
	GAME * m_pGame;
	void * m_pCallbackData;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokRigidBodySetHitFunc(
	GAME * pGame,
	UNIT * pUnit,
	hkRigidBody * pRigidBody, 
	FP_HAVOKHIT pHitFunc,
	void * pData )
{
	CUnitCollisionListener * pListener = new CUnitCollisionListener;
	pListener->m_pGame = pGame;
	pListener->m_idUnit = UnitGetId( pUnit );
	pListener->m_pfnUnitHit = pHitFunc;
	pListener->m_pCallbackData = pData;
	pRigidBody->addCollisionListener( pListener );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
VECTOR HavokGetGravity()
{
	return VECTOR( 0, 0, -9.800723f );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
hkWorld * HavokGetHammerWorld()
{
	return sgpHammerWorld;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int HavokGetHammerFXWorld()
{
	return sgnHammerFxWorld;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokSaveSnapshot(
   hkWorld * pWorld)
{
	static int nSnapshotNumber = 0;
	char szFileName[ 256 ];
	PStrPrintf( szFileName, 256, "data\\snapshot%d.xml", nSnapshotNumber++ );
	hkOstream outfile( szFileName );
	HK_ON_DEBUG( hkBool res = ) hkHavokSnapshot::save( pWorld, outfile.getStreamWriter(), FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL sHavokValidatePositionAndDirection(
	const VECTOR& vPosition, 
	const VECTOR& vDirection)
{
#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEVELOPMENT)		// don't assert on live servers to reduce server spam
	IF_NOT_RETFALSE(_finite(vPosition.fX) && !_isnan(vPosition.fX));
	IF_NOT_RETFALSE(_finite(vPosition.fY) && !_isnan(vPosition.fY));
	IF_NOT_RETFALSE(_finite(vPosition.fZ) && !_isnan(vPosition.fZ));
	IF_NOT_RETFALSE(_finite(vDirection.fX) && !_isnan(vDirection.fX));
	IF_NOT_RETFALSE(_finite(vDirection.fY) && !_isnan(vDirection.fY));
	IF_NOT_RETFALSE(_finite(vDirection.fZ) && !_isnan(vDirection.fZ));
	IF_NOT_RETFALSE(!VectorIsZeroEpsilon(vDirection));
#else
	ASSERT_RETFALSE(_finite(vPosition.fX) && !_isnan(vPosition.fX));
	ASSERT_RETFALSE(_finite(vPosition.fY) && !_isnan(vPosition.fY));
	ASSERT_RETFALSE(_finite(vPosition.fZ) && !_isnan(vPosition.fZ));
	ASSERT_RETFALSE(_finite(vDirection.fX) && !_isnan(vDirection.fX));
	ASSERT_RETFALSE(_finite(vDirection.fY) && !_isnan(vDirection.fY));
	ASSERT_RETFALSE(_finite(vDirection.fZ) && !_isnan(vDirection.fZ));
	ASSERT_RETFALSE(!VectorIsZeroEpsilon(vDirection));
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float HavokLineCollideLen( 
	hkWorld * pWorld,
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen, 
	VECTOR* pvNormal,
	int* pCollideKey)
{
	ASSERT_RETZERO(pWorld);

	if ( fMaxLen < 0.001f )
		return fMaxLen;

	if( !sHavokValidatePositionAndDirection( vPosition, vDirection ) )
		return fMaxLen;

	// compute endpoint for the ray
	VECTOR vDirectionScaled;
	VectorScale( vDirectionScaled, vDirection, fMaxLen );
	VECTOR vRayEnd;
	VectorAdd( vRayEnd, vPosition, vDirectionScaled );

	hkWorldRayCastInput tRayCastInput;
	tRayCastInput.m_from.set( vPosition.fX, vPosition.fY, vPosition.fZ );
	tRayCastInput.m_to  .set( vRayEnd.fX,	vRayEnd.fY,	  vRayEnd.fZ );
	tRayCastInput.m_enableShapeCollectionFilter = FALSE;
	tRayCastInput.m_filterInfo = hkGroupFilter::calcFilterInfo( COLFILTER_RAYCAST_LAYER, COLFILTER_RAYCAST_SYSTEM );
	hkWorldRayCastOutput tRayCastOutput;

	pWorld->castRay(tRayCastInput, tRayCastOutput);

	// change the normal back into world coordinates
	if ( tRayCastOutput.hasHit() ) 
	{
		if ( pvNormal )
		{
			pvNormal->fX = tRayCastOutput.m_normal.getSimdAt( 0 );
			pvNormal->fY = tRayCastOutput.m_normal.getSimdAt( 1 );
			pvNormal->fZ = tRayCastOutput.m_normal.getSimdAt( 2 );
			//MatrixMultiplyNormal( pvNormal, pvNormal, &pRoom->tWorldMatrix );
		}
		if ( pCollideKey )
		{
#if USE_HAVOK_4
			hkShapeKey hitKey = HK_INVALID_SHAPE_KEY;
			for( int sk = 0; sk < hkShapeRayCastOutput::MAX_HIERARCHY_DEPTH; sk++ )
			{
				if( tRayCastOutput.m_shapeKeys[sk] == HK_INVALID_SHAPE_KEY )
					break;
				hitKey = tRayCastOutput.m_shapeKeys[sk];
			}
			*pCollideKey = hitKey;
#else
			*pCollideKey = tRayCastOutput.m_shapeKey;
#endif
		}
	}
	else
	{
		ASSERT_RETZERO( tRayCastOutput.m_hitFraction == 1.0f );
	}
	return tRayCastOutput.m_hitFraction * fMaxLen;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void HavokLoadShapeForAppearance( 
	int nAppearanceDefId, 
	const char * pszFilename )
{
}

#endif //HAVOK