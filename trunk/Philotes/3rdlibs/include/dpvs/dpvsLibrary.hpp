#ifndef __DPVSLIBRARY_HPP
#define __DPVSLIBRARY_HPP
/*****************************************************************************
 *
 * dPVS
 * -----------------------------------------
 *
 * (C) 1999-2005 Hybrid Graphics Ltd.
 * (C) 2006-     Umbra Software Ltd.
 * All Rights Reserved.
 *	
 * This file consists of unpublished, proprietary source code of
 * Umbra Software, and is considered Confidential Information for
 * purposes of non-disclosure agreement. Disclosure outside the terms
 * outlined in signed agreement may result in irreparable harm to
 * Umbra Software and legal action against the party in breach.
 *
 * Description:     Library interface
 *
 * $Id: //umbra/products/dpvs/interface/dpvsLibrary.hpp#1 $
 * $Date: 2006/11/07 $
 * $Author: otso $
 * 
 ******************************************************************************/

#if !defined (__DPVSDEFS_HPP)
#   include "dpvsDefs.hpp"
#endif

/******************************************************************************
 *
 * Class:           DPVS::Library
 *
 * Description:     DPVS global functions
 *
 * Notes:           This class contains functions for initializing and
 *                  shutting down the library, for querying performance
 *                  statistics and other debug information.
 *
 *****************************************************************************/

namespace DPVS
{

class LibraryDefs
{
public:

    enum InfoString
    {
        VERSION                     = 0,        // version string of the library (in format: "1.0.5")
        COPYRIGHT                   = 1,        // library copyright info string
        BUILD_TIME                  = 2,        // library build date & time
        FUNCTIONALITY               = 3,        // "registered", "trial/non-functional (tampered/no dongle)", "trial/fully functional"
        CUSTOMER                    = 4,        // name of the customer to whom the library is licensed
        CPU_OPTIMIZATIONS           = 5,        // string containing CPU-specific optimizations enabled for current processor
		COMPILER					= 6			// compiler used to compile dPVS
    };

    enum MatrixFormat
    {
        COLUMN_MAJOR                = 0,        // matrix format of OpenGL, Direct3D, Matlab, Renderman(?)
        ROW_MAJOR                   = 1         // matrix format of SurRender3D
    };

    enum LineType
    {
        LINE_MISC                   = (1<<0),   // miscellaneous
        LINE_OBJECT_BOUNDS          = (1<<1),   // show cell-space AABBs of objects
        LINE_VOXELS                 = (1<<2),   // voxels
        LINE_RECTANGLES             = (1<<3),   // object bounding rectangles
        LINE_SILHOUETTES            = (1<<4),   // occluder silhouettes
        LINE_VIRTUAL_CAMERA_AXII    = (1<<5),   // virtual camera axii
        LINE_PORTAL_RECTANGLES      = (1<<6),   // portal scissor rectangles
		LINE_VPT					= (1<<7),	// visible point tracking lines
		LINE_TEST_SILHOUETTES		= (1<<8),	// object test silhouettes
		LINE_REGIONS_OF_INFLUENCE   = (1<<9),	// regions of influence
		LINE_OBJECT_TEST_MODEL		= (1<<10),	// show visible object test model triangles (only if MeshModel is used)
		LINE_OBJECT_WRITE_MODEL		= (1<<11),	// show visible object write model triangles (only if MeshModel is used)
		LINE_OBJECT_OBBS			= (1<<12)	// show exact OBBs used in culling tests
    };

    enum BufferType
    {
        BUFFER_COVERAGE             = (1<<0),   // coverage buffer
        BUFFER_DEPTH                = (1<<1),   // depth buffer
        BUFFER_FULLBLOCKS           = (1<<2),   // full blocks buffer
        BUFFER_STENCIL              = (1<<3)    // stencil buffer
    };

    enum FlagType
    {
        LINEDRAW                    = 0,        // draw lines
        BUFFER                      = 1         
    };

    enum Statistic
    {
        STAT_SILHOUETTECACHEINSERTIONS = 0,     // number of silhouettes created and inserted into the cache
        STAT_SILHOUETTECACHEREMOVALS,           // number of silhouettes removed from the cache
        STAT_SILHOUETTECACHEQUERIES,            // number of silhouette queries
        STAT_SILHOUETTECACHEQUERYITERS,         // number of silhouette query inner iterations 
        STAT_SILHOUETTECACHEHITS,               // number of silhouette cache hits (misses = m_queries-m_hits)
        STAT_SILHOUETTECACHEMEMORYUSED,         // number of bytes used by the silhouette cache currently
		STAT_SILHOUETTECACHECONGESTED,			// number of queries when silhouette cache has been congested (and we need to take action)
        STAT_OBJECTMATRIXUPDATES,               // number of times object->cell matrices have been updated
        STAT_OBJECTCAMERAMATRIXUPDATES,         // number of times object*camera matrices have been updated
		STAT_OBJECTVPTFAILED,					// number of times object VPT has failed
        STAT_SPHEREBOUNDSQUERIED,               // number of times sphere bounding rectangles queried internally
        STAT_CAMERARESOLVEVISIBILITYCALLS,      // number of times DPVS::Camera::resolveVisibility() has been called
        STAT_CAMERAVISIBILITYCALLBACKS,         // number of times camera has performed visibility callbacks
        STAT_HZBUFFERLEVELUPDATES,              // number of times a HDEB level has been updated
        STAT_WRITEQUEUEPOINTQUERIES,            // write queue point visibility queries
        STAT_WRITEQUEUESILHOUETTEQUERIES,       // write queue silhouette visibility queries
        STAT_WRITEQUEUEOBJECTQUERIES,           // write queue object visibility queries
		STAT_WRITEQUEUEOCCLUDEESDEPTHREJECTED,	// write queue occludees rejected due to "closing depth"
        STAT_WRITEQUEUEWRITESREQUESTED,         // writes submitted to write queue
        STAT_WRITEQUEUEWRITESPERFORMED,         // writes submitted to occlusion buffer
        STAT_WRITEQUEUEWRITESPOSTPONED,         // writes postponed by the write queue (bad Z)
        STAT_WRITEQUEUEWRITESDISCARDED,         // writes discarded (invalid silhouette, beyond far plane)
        STAT_WRITEQUEUEOVERFLOW,                // insufficient allocation space in write queue (expensive)
        STAT_WRITEQUEUEOVERFLOWEVERYTHING,      // insufficient allocation space in write queue (even more expensive)
        STAT_WRITEQUEUEDEPTHWRITES,             // depth buffer writes performed by write queue
        STAT_WRITEQUEUEDEPTHCLEARS,             // depth buffer clears performed 
        STAT_WRITEQUEUEFLUSHES,                 // number of write queue flushes performed
        STAT_WRITEQUEUEBUCKETFLUSHWORK,         // number of buckets*objects traversed in flush work
        STAT_WRITEQUEUEHIDDENOCCLUDERS,         // number of hidden occluders used
        STAT_WRITEQUEUEFRONTCLIPPINGOCCLUDERSTESTED,// number of front clipping occluder tests
        STAT_WRITEQUEUEFRONTCLIPPINGOCCLUDERSUSED,  // number of front clipping occluders used
        STAT_OCCLUSIONSILHOUETTEQUERIES,		// number of occlusion buffer silhouette queries
        STAT_OCCLUSIONPOINTQUERIES,				// number of occlusion buffer point queries
        STAT_OCCLUSIONRECTANGLEQUERIES,			// number of occlusion buffer rectangle queries
        STAT_OCCLUSIONACCURATEPOINTQUERIES,     // number of points where full precision query was performed
        STAT_OCCLUSIONACCURATEBLOCKQUERIES,     // number of blocks where full precision query was performed
        STAT_OCCLUSIONACCURATEPOINTUSEFUL,      // number of points where full precision query was useful
        STAT_OCCLUSIONACCURATEBLOCKUSEFUL,      // number of blocks where full precision query was useful
        STAT_OCCLUSIONBUFFERBUCKETSCLEARED,     // number of times occlusion buffer buckets were cleared
        STAT_OCCLUSIONBUFFERBUCKETSPROCESSED,   // number of times occlusion buffer buckets were processed
		STAT_OCCLUSIONBUFFEREDGESRASTERIZED,	// number of edges rasterized (write+test)
		STAT_OCCLUSIONBUFFEREDGESTESTED,		// number of edges tested (test only)
		STAT_OCCLUSIONBUFFEREDGESCLIPPING,		// number of edges clipping (both rasterized+tested)
		STAT_OCCLUSIONBUFFEREDGESSINGLEBUCKET,	// number of single-bucket edges
		STAT_OCCLUSIONBUFFEREDGEPIXELS,			// number of edge pixels rasterized
		STAT_OCCLUSIONBUFFERTESTEDGEPIXELS,		// number of test edge pixels rasterized
		STAT_OCCLUSIONBUFFEREXACTZTESTS,		// number of pixels exact Z-tested
        STAT_DATABASETRAVERSALS,                // number of top level traverse calls
        STAT_DATABASENODESINSERTED,             // number of nodes created
        STAT_DATABASENODESREMOVED,              // number of nodes removed
        STAT_DATABASENODEDIRTYUPDATES,          // number of nodes dirty updated    
        STAT_DATABASENODESTRAVERSED,            // number of nodes traversed
        STAT_DATABASELEAFNODESTRAVERSED,        // number of leaf nodes traversed
        STAT_DATABASENODESSKIPPED,              // number of nodes node-skipped during traversal,
        STAT_DATABASENODESVFTESTED,             // number of nodes view frustum tested
        STAT_DATABASENODESVFCULLED,             // number of nodes view frustum culled by primary test
        STAT_DATABASENODESVFCULLED2,            // number of nodes view frustum culled by secondary test
        STAT_DATABASENODESOCCLUSIONTESTED,      // number of nodes occlusion tested
        STAT_DATABASENODESOCCLUSIONCULLED,      // number of nodes occlusion culled
		STAT_DATABASENODEVPTFAILED,				// number of node VPT guesses gone wrong
		STAT_DATABASENODEVPTSUCCEEDED,			// number of node VPT guesses gone right
		STAT_DATABASENODEVPTUPDATED,			// number of node VPT updates (by feedback)
        STAT_DATABASENODESVISIBLE,              // number of nodes visible after VF/occlusion culling
        STAT_DATABASENODESPLITS,                // number of nodes split
        STAT_DATABASENODENEWROOTS,              // number of times a new root node has been created (i.e. upward collapses)
        STAT_DATABASENODESFRONTCLIPPING,        // number of front clipping nodes traversed
        STAT_DATABASEOBSTATUSCHANGES,           // number of times object's visibility status has changed from visible->hidden or vice versaa
        STAT_DATABASEOBSINSERTED,               // number of objects inserted into the database
        STAT_DATABASEOBSREMOVED,                // number of objects removed from the database
        STAT_DATABASEOBSUPDATED,                // number of object update calls
        STAT_DATABASEOBSUPDATEPROCESSED,        // number of times object update calls really caused work
        STAT_DATABASEOBINSTANCESTRAVERSED,      // number of times an ob instance has been touched
        STAT_DATABASEOBSTRAVERSED,              // number of objects traversed
        STAT_DATABASEOBSVFTESTED,               // number of objects view frustum tested
        STAT_DATABASEOBSVFCULLED,               // number of objects view frustum culled
        STAT_DATABASEOBSVFEXACTTESTED,          // number of objects view frustum tested by exact tests
        STAT_DATABASEOBSVFEXACTCULLED,          // number of objects view frustum culled by exact tests
        STAT_DATABASEOBSVISIBLE,                // number of objects determined visible
        STAT_DATABASEOBSVISIBILITYPARENTCULLED, // number of objects culled by relation to visibility parents
        STAT_DATABASEOBSOCCLUSIONSKIPPED,       // number of objects whose occlusion test was skipped
        STAT_DATABASEOBSOCCLUSIONTESTED,        // number of objects occlusion tested
        STAT_DATABASEOBSOCCLUSIONCULLED,        // number of objects occlusion culled
        STAT_DATABASEOBSBACKFACETESTED,			// number of objects back-face tested
        STAT_DATABASEOBSBACKFACECULLED,			// number of objects back-face culled
        STAT_DATABASEOBNEWVISIBLEPOINTS,        // number of times a new "visible point" has been computed
        STAT_DATABASEINSTANCESINSERTED,         // number of instances inserted
        STAT_DATABASEINSTANCESREMOVED,          // number of instances removed
        STAT_DATABASEINSTANCESMOVED,            // number of instances moved (i.e. skipped a removal/insertion pair)
        STAT_ROIACTIVE,                         // number of regions of influence found visible
        STAT_ROISTATECHANGES,                   // number of region of influence state changes (active <-> inactive)
        STAT_ROIOBJECTOVERLAPTESTS,             // number of ROI vs. object overlap tests performed
        STAT_ROIOBJECTOVERLAPS,                 // number of ROI vs. object overlaps
        STAT_HOAX0,                             // hoax counter 0
        STAT_HOAX1,                             // hoax counter 1
        STAT_HOAX2,                             // hoax counter 2
        STAT_HOAX3,                             // hoax counter 3
        STAT_HOAX4,                             // hoax counter 4
        STAT_HOAX5,                             // hoax counter 5
        STAT_HOAX6,                             // hoax counter 6
        STAT_HOAX7,                             // hoax counter 7
		STAT_MODELRECTANGLESQUERIED,			// number of times a bounding rectangle has been calculated for a model1
		STAT_MODELEXACTRECTANGLESQUERIED,		// number of times an exact bounding rectangle has been calculated for a model
		STAT_MODELTESTSILHOUETTESQUERIED,		// number of times a test silhouette has been queried
		STAT_MODELTESTSILHOUETTESCLIPPED,		// number of times a test silhouette has been clipped (front-clipping)
		STAT_MODELTESTSILHOUETTESREJECTED,		// number of times a test silhouette has been rejected (i.e. determined automatically as visible)
		STAT_MODELWRITESILHOUETTESQUERIED,		// number of times a write silhouette has been queried
		STAT_MODELTOPOLOGYCOMPUTED,				// number of times model derived topology has been computed
        STAT_MODELDERIVEDMEMORYUSED,			// number of bytes of cache used by model derived topology
		STAT_MODELDERIVED,						// number of model derived allocations
		STAT_TIME,                              // time in seconds since last reset (timer accuracy is platform-dependent)
		STAT_MEMORYUSED,						// number of bytes of memory used by DPVS heap (i.e. allocated from the OS)
		STAT_MEMORYCURRENTALLOCATIONS,			// number of current memory allocations
		STAT_MEMORYOPERATIONS,					// number of (internal) memory allocs/releases since last reset (handled by the heap of dPVS)
		STAT_MEMORYEXTERNALOPERATIONS,			// number of (external) memory allocs/releases since last reset (true operating system memory allocs)
		STAT_LIVECAMERAS,						// number of cameras in the world
		STAT_LIVECELLS,							// number of cells in the world
		STAT_LIVEMODELS,						// number of models in the world
		STAT_LIVEOBJECTS,						// number of objects in the world (includes ROIs & portals)
		STAT_LIVEPHYSICALPORTALS,				// number of physical portals in the world
		STAT_LIVEREGIONSOFINFLUENCE,			// number of ROIs
		STAT_LIVEVIRTUALPORTALS,				// number of virtual portals in the world
		STAT_LIVENODES,							// number of spatial database nodes in the world
		STAT_LIVEINSTANCES,						// number of object instances in nodes in the world
        STAT_MAX                                // end of list
    };

	class Services
	{
	public:
		DPVSDEC					Services			(void);
		DPVSDEC virtual			~Services			(void);
		DPVSDEC virtual void	error				(const char* message);	// fatal error (this func should not return)
		DPVSDEC virtual void*	allocateMemory		(size_t bytes);			// malloc()
		DPVSDEC virtual void	releaseMemory		(void*);				// free()
		DPVSDEC virtual float	getTime				(void);					// return time in seconds since startup (w/ 20 Hz accuracy at least, please)
		DPVSDEC virtual void	enterMutex			(void);					// enter global mutex
		DPVSDEC virtual void	leaveMutex			(void);					// leave global mutex
	private:
								Services			(const Services&);
		Services&				operator=			(const Services&);
	};
};

class Library : public LibraryDefs
{
public:
    static DPVSDEC void				checkConsistency		(void);
    static DPVSDEC void				clearFlags              (FlagType,unsigned int);
    static DPVSDEC void				exit                    (void);
    static DPVSDEC unsigned int		getFlags                (FlagType);
    static DPVSDEC const char*		getInfoString           (InfoString);
    static DPVSDEC float			getStatistic            (Statistic);
	static DPVSDEC const char*		getStatisticName		(Statistic);
    static DPVSDEC void				init                    (MatrixFormat mf, Services* s = 0);
    static DPVSDEC void				minimizeMemoryUsage     (void);
    static DPVSDEC void				resetStatistics         (void);
    static DPVSDEC void				setFlags                (FlagType,unsigned int);
    static DPVSDEC void				suggestGarbageCollect   (class Commander*, float);
    static DPVSDEC int				textCommand             (class Commander*, const char*);     // external debugging
private:                                            
									Library                 (void);             // no constructor exists
                                    Library                 (const Library&);   // no constructor exists
    static class ImpLibrary*        s_implementation;                           // implementation pointer
};

} // namespace DPVS

//------------------------------------------------------------------------
#endif //__DPVSLIBRARY_HPP



