//----------------------------------------------------------------------------
// memorymacros.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

// Switches
//
#define DEBUG_SINGLE_ALLOCATIONS				0 // When specified, each individual allocation is tracked instead of each allocation origin (very slow).
#define VERIFY_MEMORY_ALLOCATIONS				0 // Set to 1 to validate memory allocators.  Used when debugging allocators themselves(very slow).
#define DEBUG_MEMORY_ALLOCATIONS				(ISVERSION(DEBUG_VERSION) || ISVERSION(SERVER_VERSION))
#define USE_MEMORY_MANAGER						1 // Specifies use of new CMemoryManager / IMemoryAllocator system
#define DEBUG_MEMORY_OVERRUN_GUARD				0 // Used system allocator exclusively and uses page protection to detect memory over/underruns






// Passing file/line parameters to memory functions
//
#if DEBUG_MEMORY_ALLOCATIONS
#define MEMORY_FUNCNOARGS()						const char * file, unsigned int line
#define MEMORY_FUNCARGS(...)					__VA_ARGS__, const char * file, unsigned int line
#define MEMORY_FUNC_FILELINE(...)				__VA_ARGS__, (const char *)__FILE__, (unsigned int)__LINE__
#define MEMORY_FUNCNOARGS_PASSFL()				(const char *)(file), (unsigned int)(line)
#define MEMORY_FUNC_PASSFL(...)					__VA_ARGS__, (const char *)(file), (unsigned int)(line)
#define MEMORY_FUNC_FLPARAM(file, line, ...)	__VA_ARGS__, (const char *)(file), (unsigned int)(line)
#define MEMORY_DEBUG_REF(x)						REF(x)
#define MEMORY_NDEBUG_REF(x)					
#else // else DEBUG_MEMORY_ALLOCATIONS
#define MEMORY_FUNCNOARGS()						
#define MEMORY_FUNCARGS(...)					__VA_ARGS__
#define MEMORY_FUNC_FILELINE(...)				__VA_ARGS__
#define MEMORY_FUNCNOARGS_PASSFL()				
#define MEMORY_FUNC_PASSFL(...)					__VA_ARGS__
#define MEMORY_FUNC_FLPARAM(file, line, ...)	__VA_ARGS__
#define MEMORY_DEBUG_REF(x)						
#define MEMORY_NDEBUG_REF(x)					REF(x)
#endif // end DEBUG_MEMORY_ALLOCATIONS

#define memclear(p, s)											memset(p, 0, s)
#define PTR_TO_SCALAR(p)										((size_t)((BYTE *)(p) - (BYTE *)0))
#define IS_ALIGNED(ptr, align)									((((BYTE *)(ptr) - (BYTE *)0) % (size_t)(align)) == 0)


//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------
#define MALLOC(pool, size)										MemoryAlloc(MEMORY_FUNC_FILELINE(pool, size))
#define MALLOCZ(pool, size)										MemoryAllocZ(MEMORY_FUNC_FILELINE(pool, size))
#define MALLOCFL(pool, size, file, line)						MemoryAlloc(MEMORY_FUNC_FLPARAM(file, line, pool, size))
#define MALLOCPASSFL(pool, size)								MemoryAlloc(MEMORY_FUNC_PASSFL(pool, size))
#define MALLOCZFL(pool, size, file, line)						MemoryAllocZ(MEMORY_FUNC_FLPARAM(file, line, pool, size))
#define MALLOCZPASSFL(pool, size)								MemoryAllocZ(MEMORY_FUNC_PASSFL(pool, size))
#define MALLOCBIG(pool, size)									MemoryAllocBig(MEMORY_FUNC_FILELINE(pool, size))
#define MALLOCBIGFL(pool, size, file, line)						MemoryAllocBig(MEMORY_FUNC_FLPARAM(file, line, pool, size))
#define ALIGNED_MALLOC(pool, size, align)						MemoryAlignedAlloc(MEMORY_FUNC_FILELINE(pool, size, align))
#define ALIGNED_MALLOCZ(pool, size, align)						MemoryAlignedAllocZ(MEMORY_FUNC_FILELINE(pool, size, align))
#define ALIGNED_MALLOCFL(pool, size, align, file, line)			MemoryAlignedAlloc(MEMORY_FUNC_FLPARAM(file, line, pool, size, align))
#define ALIGNED_MALLOCPASSFL(pool, size, align)					MemoryAlignedAlloc(MEMORY_FUNC_PASSFL(pool, size, align))
#define ALIGNED_MALLOCZFL(pool, size, align, file, line)		MemoryAlignedAllocZ(MEMORY_FUNC_FLPARAM(file, line, pool, size, align))

#define MALLOC_TYPE(type, pool, count)							(type *)MALLOC(pool, sizeof(type) * count)
#define MALLOCZ_TYPE(type, pool, count)							(type *)MALLOCZ(pool, sizeof(type) * count)
#define REALLOC_TYPE(type, pool, ptr, count)					(type *)REALLOC(pool, (void *)ptr, sizeof(type) * count)

#define REALLOC(pool, ptr, size)								MemoryRealloc(MEMORY_FUNC_FILELINE(pool, (void *)ptr, size))
#define REALLOCZ(pool, ptr, size)								MemoryReallocZ(MEMORY_FUNC_FILELINE(pool, (void *)ptr, size))
#define REALLOCFL(pool, ptr, size, file, line)					MemoryRealloc(MEMORY_FUNC_FLPARAM(file, line, pool, (void *)ptr, size))
#define REALLOCPASSFL(pool, ptr, size)							MemoryRealloc(MEMORY_FUNC_PASSFL(pool, (void *)ptr, size))
#define REALLOCZFL(pool, ptr, size, file, line)					MemoryReallocZ(MEMORY_FUNC_FLPARAM(file, line, pool, (void *)ptr, size))
#define REALLOCZPASSFL(pool, ptr, size)							MemoryReallocZ(MEMORY_FUNC_PASSFL(pool, (void *)ptr, size))
#define ALIGNED_REALLOC(pool, ptr, size, align)					MemoryAlignedRealloc(MEMORY_FUNC_FILELINE(pool, ptr, size, align))
#define ALIGNED_REALLOCZ(pool, ptr, size, align)				MemoryAlignedReallocZ(MEMORY_FUNC_FILELINE(pool, ptr, size, align))
#define ALIGNED_REALLOCFL(pool, ptr, size, align, file, line)	MemoryAlignedRealloc(MEMORY_FUNC_FLPARAM(file, line, pool, ptr, size, align))
#define ALIGNED_REALLOCPASSFL(pool, ptr, size, align)			MemoryAlignedRealloc(MEMORY_FUNC_PASSFL(pool, ptr, size, align))
#define ALIGNED_REALLOCZFL(pool, ptr, size, align, file, line)	MemoryAlignedReallocZ(MEMORY_FUNC_FLPARAM(file, line, pool, ptr, size, align))

#define FREE(pool, ptr)											MemoryFree(MEMORY_FUNC_FILELINE(pool, (void *)ptr))
#define FREEFL(pool, ptr, file, line)							MemoryFree(MEMORY_FUNC_FLPARAM(file, line, pool, (void *)ptr))

#define MSIZE(pool, ptr)										MemorySize(MEMORY_FUNC_FILELINE(pool, (void *)ptr))


//-----------------------------------------------------------------------------
// NEW/DELETE MACROS
//-----------------------------------------------------------------------------
#undef	DELETE
#define DELETE(pool, ptr, type)									((type *)ptr)->~type(); FREE(pool, ptr)

// use this macro to delete objects with private destructors
#define MEMORYPOOL_PRIVATE_DELETE(pool, ptr, type)				if (ptr) { (ptr)->~type(); FREE(pool, ptr); }
#define MEMORYPOOL_NEW(pool)									new(MEMORY_FUNC_FILELINE(static_cast<MEMORYPOOL *>(pool)))	 // static_cast handles the null ptr case
#define MEMORYPOOL_NEWFL(pool, file, line)						new(MEMORY_FUNC_PASSFL(static_cast<MEMORYPOOL *>(pool)))
#define MEMORYPOOL_DELETE(pool, ptr)							FreeDelete(MEMORY_FUNC_FILELINE(pool, ptr))
#define MEMORYPOOL_DELETEFL(pool, ptr)							FreeDelete(MEMORY_FUNC_PASSFL(pool, ptr))

#define MALLOC_NEW(pool, type)									MallocNew<type>(MEMORY_FUNC_FILELINE(pool))
#define MALLOC_NEWFL(pool, type, file, line)					MallocNew<type>(MEMORY_FUNC_PASSFL(pool))
#define MALLOC_NEWARRAY(pool, type, count)						MallocNewArray<type>(MEMORY_FUNC_FILELINE(pool, count))
#define MALLOC_NEWARRAYFL(pool, type, count)					MallocNewArray<type>(MEMORY_FUNC_PASSFL(pool, count))
#define FREE_DELETE(pool, ptr, type)							FreeDelete(MEMORY_FUNC_FILELINE(pool, (type *)ptr))
#define FREE_DELETE_ARRAY(pool, ptr, type)						FreeDeleteArray<type>(MEMORY_FUNC_FILELINE(pool, (type *)ptr))


