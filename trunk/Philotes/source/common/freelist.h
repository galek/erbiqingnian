//----------------------------------------------------------------------------
//	freelist.h
//  Copyright 2006, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _FREELIST_H_
#define _FREELIST_H_


#ifndef _LIST_H_
#include "list.h"
#endif


//----------------------------------------------------------------------------
// FREELIST
// DESC: thread safe free list.
// NOTE: class T must be derived from LIST_SL_NODE.
//----------------------------------------------------------------------------
template<class T>
struct FREELIST
{
private:
	LIST_SING			m_list;
	CCritSectLite		m_listLock;
	volatile UINT		m_allocCount;
	UINT				m_allocLimit;
	volatile UINT		m_freeListCount;
	UINT				m_freeListLimit;
public:
	//------------------------------------------------------------------------
	void Init(
		UINT allocLimit,
		UINT freeListLimit )
	{
		m_list.Init();
		m_listLock.Init();
		m_allocCount = 0;
		m_allocLimit = allocLimit;
		m_freeListCount = 0;
		m_freeListLimit = freeListLimit;
	}
	//------------------------------------------------------------------------
	void Destroy(
		struct MEMORYPOOL * pool )
	{
		T * entry = (T*)m_list.Pop();
		while( entry )
		{
			FREE( pool, entry );
			entry = (T*)m_list.Pop();
		}
		m_listLock.Free();
		m_allocCount = 0;
		m_freeListCount = 0;
	}
	//------------------------------------------------------------------------
	void ReleaseAllMemory(
		struct MEMORYPOOL * pool )
	{
		CSLAutoLock lock = &m_listLock;
		T * entry = (T*)m_list.Pop();
		while( entry )
		{
			FREE( pool, entry );
			entry = (T*)m_list.Pop();
		}
		m_allocCount -= m_freeListCount;
		m_freeListCount = 0;
	}
	//------------------------------------------------------------------------
	// NOT THREAD SAFE
	//------------------------------------------------------------------------
	#define FREELIST_PREALLOC( list, pool, count ) list.PreAllocate(pool,count,__FILE__,__LINE__)
	BOOL PreAllocate(
		struct MEMORYPOOL * pool,
		UINT count,
		char * file,
		int line )
	{
		MEMORY_NDEBUG_REF(file);
		MEMORY_NDEBUG_REF(line);
		CSLAutoLock lock = &m_listLock;

		if( m_allocCount + count > m_allocLimit ||
			m_freeListCount + count > m_freeListLimit )
		{
			return FALSE;
		}

		UINT ii;
		for( ii = 0; ii < count; ++ii )
		{
			T * entry = (T*)MALLOCZFL( pool, sizeof(T), file, line );
			FL_ASSERT_RETFALSE(entry,file,line);
			++m_allocCount;

			m_list.Push(entry);
			++m_freeListCount;
		}

		return TRUE;
	}
	//------------------------------------------------------------------------
	#define FREELIST_GET( list, pool )	list.Get( pool, __FILE__, __LINE__ )
	//------------------------------------------------------------------------
	T * Get(
		struct MEMORYPOOL * pool,
		char * file,
		int line )
	{
		MEMORY_NDEBUG_REF(file);
		MEMORY_NDEBUG_REF(line);

		T * toRet = NULL;
		{
			CSLAutoLock lock = &m_listLock;

			toRet = (T*)m_list.Pop();

			if( !toRet )
			{
				//	need to malloc one
				FL_ASSERTX_RETNULL( (m_allocCount + 1) <= m_allocLimit, "FREELIST: list alloc count over the specified limit.", file, line );
			
				toRet = (T*)MALLOCZFL( pool, sizeof(T),file,line );
				FL_ASSERT_RETNULL(toRet,file,line);

				++m_allocCount;
			}
			else
			{
				//	got from free list
				--m_freeListCount;
			}
		}

		new (toRet) T();

		return toRet;
	}
	//------------------------------------------------------------------------
	void Free(
		struct MEMORYPOOL * pool,
		T * toFree )
	{
		if( !toFree )
			return;

		toFree->~T();

		{
			CSLAutoLock lock = &m_listLock;

			if( (m_freeListCount + 1) > m_freeListLimit )
			{
				FREE(pool,toFree);
				--m_allocCount;
			}
			else
			{
				m_list.Push(toFree);
				++m_freeListCount;
			}
		}
	}
	//------------------------------------------------------------------------
	UINT Count(
		void )
	{
		return m_freeListCount;
	}
};

//----------------------------------------------------------------------------
// LAZY_FREELIST
// DESC: thread safe lazy free list.
// NOTE: class T must be derived from LIST_SL_NODE.
// We take a function pointer specifying how to free a class, and only
// execute the free sequence when we reuse a T.
//
// Unlike above, we don't allow a freelist limit.
// This is necessary to guarantee lazy free.
//----------------------------------------------------------------------------
template<class T>
struct LAZY_FREELIST
{
private:
	LIST_SING			m_list;
	CCritSectLite		m_listLock;
	volatile UINT		m_allocCount;
	UINT				m_allocLimit;
	volatile UINT		m_freeListCount;
	void				(*m_fpFree)(void *);
public:
	//------------------------------------------------------------------------
	void Init(
		UINT allocLimit,
		void (*fpFree)(void *) )
	{
		m_list.Init();
		m_listLock.Init();
		m_allocCount = 0;
		m_allocLimit = allocLimit;
		m_freeListCount = 0;
		m_fpFree = fpFree;
	}
	//------------------------------------------------------------------------
	void Destroy(
	struct MEMORYPOOL * pool )
	{
		T * entry = (T*)m_list.Pop();
		while( entry )
		{
			m_fpFree(entry);
			FREE( pool, entry );
			entry = (T*)m_list.Pop();
		}
		m_listLock.Free();
		m_allocCount = 0;
		m_freeListCount = 0;
	}
	//------------------------------------------------------------------------
	void ReleaseAllMemory(
	struct MEMORYPOOL * pool )
	{
		CSLAutoLock lock = &m_listLock;
		T * entry = (T*)m_list.Pop();
		while( entry )
		{
			m_fpFree(entry);
			FREE( pool, entry );
			entry = (T*)m_list.Pop();
		}
		m_allocCount -= m_freeListCount;
		m_freeListCount = 0;
	}
	//------------------------------------------------------------------------
	// NOT THREAD SAFE
	//------------------------------------------------------------------------
#define FREELIST_PREALLOC( list, pool, count ) list.PreAllocate(pool,count,__FILE__,__LINE__)
	BOOL PreAllocate(
	struct MEMORYPOOL * pool,
		UINT count,
		char * file,
		int line )
	{
		MEMORY_NDEBUG_REF(file);
		MEMORY_NDEBUG_REF(line);
		CSLAutoLock lock = &m_listLock;

		if( m_allocCount + count > m_allocLimit )
		{
			return FALSE;
		}

		UINT ii;
		for( ii = 0; ii < count; ++ii )
		{
			T * entry = (T*)MALLOCZFL( pool, sizeof(T), file, line );
			FL_ASSERT_RETFALSE(entry,file,line);
			++m_allocCount;

			m_list.Push(entry);
			++m_freeListCount;
		}

		return TRUE;
	}
	//------------------------------------------------------------------------
#define FREELIST_GET( list, pool )	list.Get( pool, __FILE__, __LINE__ )
	//------------------------------------------------------------------------
	T * Get(
	struct MEMORYPOOL * pool,
		char * file,
		int line )
	{
		MEMORY_NDEBUG_REF(file);
		MEMORY_NDEBUG_REF(line);

		T * toRet = NULL;
		{
			CSLAutoLock lock = &m_listLock;

			toRet = (T*)m_list.Pop();

			if( !toRet )
			{
				//	need to malloc one
				FL_ASSERTX_RETNULL( (m_allocCount + 1) <= m_allocLimit, "FREELIST: list alloc count over the specified limit.", file, line );

				toRet = (T*)MALLOCZFL( pool, sizeof(T),file,line );
				FL_ASSERT_RETNULL(toRet,file,line);

				++m_allocCount;
			}
			else
			{
				//	got from free list
				--m_freeListCount;
				//  lazy free.
				m_fpFree(toRet);
			}
		}

		new (toRet) T();

		return toRet;
	}
	//------------------------------------------------------------------------
	void Free(
	struct MEMORYPOOL * pool,
		T * toFree )
	{
		UNREFERENCED_PARAMETER(pool);
		if( !toFree )
			return;

		toFree->~T();

		{
			CSLAutoLock lock = &m_listLock;

			{
				m_list.Push(toFree);
				++m_freeListCount;
			}
		}
	}
	//------------------------------------------------------------------------
	UINT Count(
		void )
	{
		return m_freeListCount;
	}
};
#endif	//	_FREELIST_H_
