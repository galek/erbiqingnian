#pragma once
//------------------------------------------------------------------------------
/**
    @file core/config.h

    Nebula3 compiler specific defines and configuration.
    
    (C) 2006 Radon Labs GmbH
*/

// setup platform identification macros

#ifdef __WIN32__
#undef __WIN32__
#endif
#ifdef WIN32
#define __WIN32__ (1)
#endif

//------------------------------------------------------------------------------
/**
    Nebula3 configuration.
*/

#ifdef _DEBUG
#define PH_DEBUG (1)
#endif

/// max size of a data slice is 16 kByte - 1 byte
/// this needs to be in a header, which is accessable from SPU too,
/// thats why its here
static const int JobMaxSliceSize = 0x3FFF;

#if PUBLIC_BUILD
#define __PHILO_NO_ASSERT__ (0)    // DON'T SET THIS TO (1) - PUBLIC_BUILD SHOULD STILL DISPLAY ASSERTS!
#else
#define __PHILO_NO_ASSERT__ (0)
#endif

// define whether a platform comes with archive support built into the OS
#define PH_NATIVE_ARCHIVE_SUPPORT (0)

// enable/disable Nebula3 memory stats
#if PH_DEBUG
#define PH_MEMORY_STATS (1)
#define PH_MEMORY_ADVANCED_DEBUGGING (0)
#else
#define PH_MEMORY_STATS (0) // not useable on xbox360 in release mode cause of HeapWalk
#define PH_MEMORY_ADVANCED_DEBUGGING (0)
#endif

// enable/disable memory pool allocation for refcounted object
// FIXME -> memory pool is disabled for all platforms, cause it causes crashes (reproducable on xbox360)
#if (__WII__ || __MAYA__ || __WIN32__ || __PS3__)
#define PH_OBJECTS_USE_MEMORYPOOL (0)
#else
#define PH_OBJECTS_USE_MEMORYPOOL (0)
#endif

// Enable/disable serial job system (ONLY SET FOR DEBUGGING!)
// You'll also need to fix the foundation_*.epk file to use the jobs/serial source files
// instead of jobs/tp!
// On the Wii, the serial job system is always active.
#define PH_USE_SERIAL_JOBSYSTEM (0)

// enable/disable thread-local StringAtom tables
#define PH_ENABLE_THREADLOCAL_STRINGATOM_TABLES (0)

// enable/disable growth of StringAtom buffer
#define PH_ENABLE_GLOBAL_STRINGBUFFER_GROWTH (1)

// size of of a chunk of the global string buffer for StringAtoms
#define PH_GLOBAL_STRINGBUFFER_CHUNKSIZE (32 * 1024)

// enable/disable Nebula3 animation system log messages
#define PH_ANIMATIONSYSTEM_VERBOSELOG (0)
#define PH_ANIMATIONSYSTEM_FRAMEDUMP (0)

// override SQLite filesystem functions with Nebula functions?
// only useful on consoles
// win32 doesn't work without !!!
#define PH_OVERRIDE_SQLITE_FILEFUNCTIONS (1)

// enable/disable bounds checking in the container util classes
#if PH_DEBUG
#define PH_BOUNDSCHECKS (1)
#else
#define PH_BOUNDSCHECKS (1)
#endif
         
// enable/disable the builtin HTTP server
#if PUBLIC_BUILD || __WII_PROFILING__
#define __PH_HTTP__ (0)
#else
#define __PH_HTTP__ (1)
#endif

// enable/disable the transparent web filesystem
#if __WIN32__
#define __PH_HTTP_FILESYSTEM__ (1)
#else
#define __PH_HTTP_FILESYSTEM__ (0)
#endif

// enable/disable profiling (see Debug::DebugTimer, Debug::DebugCounter)
#if PUBLIC_BUILD
#define PH_ENABLE_PROFILING (0)
#elif __PH_HTTP__
// profiling needs http
#define PH_ENABLE_PROFILING (1)
#else 
#define PH_ENABLE_PROFILING (0)
#endif

// max length of a path name
#define PH_MAXPATH (512)

// enable/disable support for Nebula2 file formats and concepts
#define PH_LEGACY_SUPPORT (1)

// enable/disable mini dumps
#define PH_ENABLE_MINIDUMPS (1)

// enable/disable debug messages in fmod coreaudio
#define PH_FMOD_COREAUDIO_VERBOSE_ENABLED  (0)

// enable fmod profiling feature
#define PH_FMOD_ENABLE_PROFILING (0)

// Nebula3's main window class
#define PH_WINDOW_CLASS "Nebula3::MainWindow"

// number of lines in the IO::HistoryConsoleHandler ring buffer
#define PH_CONSOLE_HISTORY_SIZE (256)

// maximum number of local players for local coop games
#define PH_MAX_LOCAL_PLAYERS (4)

// use raknet on xbox360 platform?
#define XBOX360_USE_RAKNET    (0)

// enable legacy support for database vectors (vector3/vector4 stuff)
#define PH_DATABASE_LEGACY_VECTORS (1)

// enable legacy support for 3-component vectors in XML files
#define PH_XMLREADER_LEGACY_VECTORS (1)

// enable/disable scriping (NOTE: scripting support has been moved into an Addon)
#define __PH_SCRIPTING__ (1)

// define the standard IO scheme for the platform
#define DEFAULT_IO_SCHEME "file"

#if defined(__x86_64__) || defined(_M_X64) || defined(__powerpc64__) || defined(__alpha__) || defined(__ia64__) || defined(__s390__) || defined(__s390x__)
#   define _ARCHITECTURE_64
#else
#   define _ARCHITECTURE_32
#endif

#define PH_LITTLE_ENDIAN

// VisualStudio settings
#ifdef _MSC_VER
#define __VC__ (1)
#endif
#ifdef __VC__
#pragma warning( disable : 4251 )       // class XX needs DLL interface to be used...
#pragma warning( disable : 4355 )       // initialization list uses 'this' 
#pragma warning( disable : 4275 )       // base class has not dll interface...
#pragma warning( disable : 4786 )       // symbol truncated to 255 characters
#pragma warning( disable : 4530 )       // C++ exception handler used, but unwind semantics not enabled
#pragma warning( disable : 4995 )       // _OLD_IOSTREAMS_ARE_DEPRECATED
#pragma warning( disable : 4996 )       // _CRT_INSECURE_DEPRECATE, VS8: old string routines are deprecated
#pragma warning( disable : 4512 )       // 'class' : assignment operator could not be generated
#pragma warning( disable : 4610 )       // object 'class' can never be instantiated - user-defined constructor required
#pragma warning( disable : 4510 )       // 'class' : default constructor could not be generated
#endif

#if !defined(__GNUC__) && !defined(__WII__)
#define  __attribute__(x)  /**/
#endif


// define max texture space for resource streaming
#if __WIN32__
// 512 MB
#define __maxTextureBytes__ (524288000)
#else
// 256 MB
#define __maxTextureBytes__ (268435456)
#endif

// ‘› ±”√LIB
#define  _PhiloCommonExport

//------------------------------------------------------------------------------
