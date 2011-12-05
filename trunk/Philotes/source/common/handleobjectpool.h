//----------------------------------------------------------------------------
//	handleobjectpool.h
//  Copyright 2006, Flagship Studios
//
//	Provides handle-based access to pooled object memory.
//----------------------------------------------------------------------------

#pragma once


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _CRITSECT_H_
#include "critsect.h"
#endif
#ifndef _LIST_H_
#include "list.h"
#endif
#include "hlocks.h"
#include "handleobjectpool_dbg.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define INVALID_OBJECTHANDLE	((OBJECTHANDLE)-1)

// helpful for finding places that assume NULL is invalid rather than INVALID_OBJECTHANDLE
#define DEBUG_NULL_AS_INVALID	1


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//	handle used to reference pooled memory
//----------------------------------------------------------------------------
typedef UINT64 OBJECTHANDLE;

//----------------------------------------------------------------------------
// lock types
//----------------------------------------------------------------------------
DEF_HLOCK(HObjPoolFreeListLock, HLCriticalSection)
	HLOCK_LOCK_RESTRICTION(ALL_LOCKS_HLOCK_NAME)
END_HLOCK

//----------------------------------------------------------------------------
//	base handle memory manager
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
struct _HANDLE_OBJECT_POOL_BASE
{
private:
	//------------------------------------------------------------------------
	// TYPES
	//------------------------------------------------------------------------
	template<class T>
	struct OBJECT_CONTAINER : LIST_SL_NODE
	{
		CCritSectLite	ObjectLock;
		volatile DWORD	ObjectRefCount;
		volatile DWORD	ObjectDisposalFlag;
		DWORD			ObjectCounter;
		DWORD			ObjectIndex;
		T				Object;
	};

	typedef typename _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::OBJECT_CONTAINER<T> _OBJ_CONTAINER;
	
	struct HOBJECTPTR_WRAPPER
	{
	private:
		friend class HOBJECTPTR;
		_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR> *	m_objectPool;
		OBJECTHANDLE							m_hObject;
		T *										m_pObject;

	public:
		HOBJECTPTR_WRAPPER(
			_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR> * pPool,
			OBJECTHANDLE hObject,
			T * pObject ) :
			m_objectPool(pPool),
			m_hObject(hObject),
			m_pObject(pObject) {};
	};

	//------------------------------------------------------------------------
	// DATA
	//------------------------------------------------------------------------
	OBJECT_CONTAINER<T>**	m_buckets;
	volatile UINT			m_bucketCount;
	UINT					m_maxBuckets;
	UINT					m_bucketSize;

	volatile LONG			m_liveObjectCount;
	volatile LONG			m_freeObjectCount;
	volatile LONG			m_pendingFreeObjectCount;
	UINT					m_totalObjectCount;
	UINT					m_maxObjects;

	HObjPoolFreeListLock	m_freeObjectLock;
	LIST_SING				m_freeObjectList;
	DWORD					m_freeObjectIndex;
	struct MEMORYPOOL *		m_freeObjectMemPool;

	_DBG_MGR				m_debugManager;

	//------------------------------------------------------------------------
	// PRIVATE METHODS
	//------------------------------------------------------------------------
	inline OBJECTHANDLE		GetHandle( DWORD objCounter, DWORD objIndex ) {
		return ((OBJECTHANDLE)objCounter << BITS_PER_DWORD) | (OBJECTHANDLE)objIndex;
	};
	inline void				SplitHandle( OBJECTHANDLE hObject, DWORD & objCounter, DWORD & objIndex ) {
		objCounter = (DWORD)(hObject >> BITS_PER_DWORD);
		objIndex   = (DWORD)hObject;
	};
	OBJECT_CONTAINER<T> *	AllocFreeObject();
	OBJECT_CONTAINER<T> *	GetObjectAtIndex(DWORD index);
	void					InitObjectContainer(OBJECT_CONTAINER<T> * toInit,DWORD index);
	OBJECT_CONTAINER<T> *	GetLockAndRefObject(OBJECTHANDLE hObject,BOOL bIgnoreDisposeFlag);
	void					AddObjectToFreePool(OBJECT_CONTAINER<T> * toFree);

public:
	//----------------------------------------------------------------------------
	//	TYPES
	//	smart pointer wrapper for accessing memory.
	//	code should only hold smart pointers for as long as absolutely necessary,
	//		and much prefer referencing objects by handles.
	//----------------------------------------------------------------------------
	class HOBJECTPTR
	{
	private:
		_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR> *	m_objectPool;
		OBJECTHANDLE							m_hObject;
		T *										m_pObject;

	private:
		friend struct _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>;
		HOBJECTPTR(
			_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR> * pool,
			OBJECTHANDLE hObject,
			T * pObject );

	public:
		HOBJECTPTR( const HOBJECTPTR_WRAPPER & wrapper );
		HOBJECTPTR( const HOBJECTPTR & other );
		~HOBJECTPTR();

		T & operator * ( void ) { return *m_pObject; }
		T * operator-> ( void ) { return  m_pObject; }
		operator bool  ( void ) { return (m_pObject != NULL); }
		operator OBJECTHANDLE ( void ) { return m_hObject; }
	};

	//------------------------------------------------------------------------
	// METHODS
	//------------------------------------------------------------------------

	//	for now maxObjectCount works best with square numbers.
	BOOL Init( MEMORYPOOL * memPool, UINT maxObjectCount );
	void Free( void );

	//	code should only hold smart pointers for as long as absolutely necessary,
	//		and much prefer referencing objects by handles.
	HOBJECTPTR_WRAPPER	NewObject( void );
	HOBJECTPTR_WRAPPER	GetObject( OBJECTHANDLE hObject );
	void				FreeObject( OBJECTHANDLE hObject );

	UINT LiveObjectCount( void )		{ return (UINT)m_liveObjectCount; }
	UINT FreeObjectCount( void )		{ return (UINT)m_freeObjectCount; }
	UINT PendingFreeObjectCount( void )	{ return (UINT)m_pendingFreeObjectCount; }
	UINT TotalObjectCount( void )		{ return m_totalObjectCount; }
	UINT MaxObjectCount( void )			{ return m_maxObjects; }

private:
	//------------------------------------------------------------------------
	// INTERNAL METHODS
	//------------------------------------------------------------------------
	friend class HOBJECTPTR;
	BOOL AddObjectRef( OBJECTHANDLE hObject );
	void ReleaseObjectRef( OBJECTHANDLE hObject );

	typedef typename _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::HOBJECTPTR _PTR;
};


//----------------------------------------------------------------------------
// retail HANDLE_OBJECT_POOL
//----------------------------------------------------------------------------
template<class T>
struct HANDLE_OBJECT_POOL :
	_HANDLE_OBJECT_POOL_BASE< T, _NON_DBG_HDL_MGR<T> > { };


//----------------------------------------------------------------------------
// debugging HANDLE_OBJECT_POOL
// NOTE: DBG_DATA_ACCESSOR class must implement the following interface:
//	struct MyDebugAccessor
//	{
//		typedef MyDebugDataType DBG_DATA;
//		static void OnPendingFreeAdded( OBJECTHANDLE hObject, UINT pendingFreeCount);
//		static void OnPendingFreeRemoved( OBJECTHANDLE hObject, UINT pendingFreeCount);
//		static void OnObjectDisposing( T * pObject, DBG_DATA * debugDataToFill );
//		static void OnFailedHandleAccessWithData( OBJECTHANDLE hObject, DBG_DATA * filledDebugData );
//		static void OnFailedHandleAccessWithoutData( OBJECTHANDLE hObject );
//	};
// accessor methods will be called by at most one thread per associated object pool.
// debug manager will keep at most HISTORY_COUNT number of DBG_DATA entries.
//----------------------------------------------------------------------------
template<class T,					//	type to pool and control access to
		 class DBG_DATA_ACCESSOR,	//	static type to receive debugging callbacks
		 UINT HISTORY_COUNT>		//	number of disposed data entries to be kept by the debug manager
struct HANDLE_OBJECT_POOL_DBG : 
	_HANDLE_OBJECT_POOL_BASE< T, _DBG_HDL_MGR<T,DBG_DATA_ACCESSOR,HISTORY_COUNT> > { };




//----------------------------------------------------------------------------
// HOBJECTPTR METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::HOBJECTPTR::HOBJECTPTR(
	_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR> * pool,
	OBJECTHANDLE hObject,
	T * pObject ) : 
		m_objectPool(pool),
		m_hObject(hObject),
		m_pObject(pObject)
{
	ASSERT_DO(pool != NULL)
	{
		m_hObject = INVALID_OBJECTHANDLE;
		m_pObject = NULL;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::HOBJECTPTR::HOBJECTPTR(
	const HOBJECTPTR_WRAPPER & wrapper  )
{
	m_objectPool = wrapper.m_objectPool;
	m_hObject    = wrapper.m_hObject;
	m_pObject    = wrapper.m_pObject;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::HOBJECTPTR::HOBJECTPTR(
	const HOBJECTPTR & other )
{
	m_objectPool = other.m_objectPool;
	m_hObject    = other.m_hObject;
	m_pObject    = other.m_pObject;

	if(m_objectPool && m_pObject)
	{
		if(!m_objectPool->AddObjectRef(m_hObject))
		{
			m_hObject = INVALID_OBJECTHANDLE;
			m_pObject = NULL;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::HOBJECTPTR::~HOBJECTPTR(
	void )
{
	if(m_objectPool && m_pObject)
	{
		m_objectPool->ReleaseObjectRef(m_hObject);
	}
}


//----------------------------------------------------------------------------
// HANDLE_OBJECT_POOL METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// public methods
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
BOOL _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::Init(
	MEMORYPOOL * memPool,
	UINT maxObjectCount )
{
#if DEBUG_NULL_AS_INVALID
	m_freeObjectIndex = 1;
#else
	m_freeObjectIndex = 0;
#endif

	m_freeObjectLock.Init();
	m_freeObjectList.Init();
	m_freeObjectMemPool = memPool;

	m_maxObjects = maxObjectCount;
	m_maxBuckets = (UINT)(sqrtf((float)maxObjectCount) + 0.9f);
	m_bucketSize = m_maxBuckets;

	DWORD bucketArrayMemSize = m_bucketSize * sizeof(OBJECT_CONTAINER<T>);
	DWORD upDiff = NEXTPOWEROFTWO(bucketArrayMemSize) - bucketArrayMemSize;
	DWORD downDiff = bucketArrayMemSize - (NEXTPOWEROFTWO(bucketArrayMemSize) >> 1);
	if (upDiff < downDiff)
	{
		m_bucketSize = NEXTPOWEROFTWO(bucketArrayMemSize) / sizeof(OBJECT_CONTAINER<T>);
	}
	else
	{
		m_bucketSize = (NEXTPOWEROFTWO(bucketArrayMemSize) >> 1) / sizeof(OBJECT_CONTAINER<T>);
	}
	m_maxBuckets = (m_maxObjects / m_bucketSize) + 1;

	m_liveObjectCount = 0;
	m_freeObjectCount = 0;
	m_pendingFreeObjectCount = 0;
	m_totalObjectCount = 0;

	m_bucketCount = 0;
	m_buckets = NULL;

	ASSERT_GOTO(m_debugManager.Init(memPool), _INIT_ERROR);

	m_buckets = (OBJECT_CONTAINER<T>**)MALLOCZ(memPool,sizeof(OBJECT_CONTAINER<T>*) * m_maxBuckets);
	ASSERT_GOTO(m_buckets != NULL, _INIT_ERROR);

	return TRUE;

_INIT_ERROR:
	Free();
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
void _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::Free(
	void )
{
	m_freeObjectLock.Free();
	m_debugManager.Free();
	if(!m_buckets)
		return;

	for(UINT ii = 0; ii < m_bucketCount; ++ii)
	{
		FREE(m_freeObjectMemPool, m_buckets[ii]);
		m_buckets[ii] = NULL;
	}
	FREE(m_freeObjectMemPool, m_buckets);
	m_buckets = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
typename _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::HOBJECTPTR_WRAPPER 
	_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::NewObject(
		void )
{
	ASSERT_DO(m_buckets != NULL)
	{
		return HOBJECTPTR_WRAPPER(this, INVALID_OBJECTHANDLE, NULL);
	}
	
	_OBJ_CONTAINER * freeObj = NULL;
	{
		HLCSAutoLock freeObjLock = &m_freeObjectLock;

		freeObj = (_OBJ_CONTAINER*)m_freeObjectList.Pop();
		if(!freeObj)
		{
			freeObj = AllocFreeObject();
			ASSERT_DO(freeObj != NULL)
			{
				return HOBJECTPTR_WRAPPER(this, INVALID_OBJECTHANDLE, NULL);
			}
		}

		--m_freeObjectCount;
		++m_liveObjectCount;
	}

	freeObj->ObjectDisposalFlag = FALSE;
	freeObj->ObjectLock.Init();
	freeObj->ObjectRefCount = 1;
	new (&freeObj->Object) T();

	OBJECTHANDLE hObject = GetHandle(freeObj->ObjectCounter,freeObj->ObjectIndex);
	return HOBJECTPTR_WRAPPER(this, hObject, &freeObj->Object);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
typename _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::HOBJECTPTR_WRAPPER 
	_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::GetObject(
		OBJECTHANDLE hObject )
{
	ASSERT_DO(m_buckets != NULL)
	{
		return HOBJECTPTR_WRAPPER(this, INVALID_OBJECTHANDLE, NULL);
	}

#if DEBUG_NULL_AS_INVALID
	ASSERT_DO(hObject != NULL)
	{
		return HOBJECTPTR_WRAPPER(this, INVALID_OBJECTHANDLE, NULL);
	}
#endif

	_OBJ_CONTAINER * container = GetLockAndRefObject(hObject,FALSE);
	if(!container)
	{
		return HOBJECTPTR_WRAPPER(this, INVALID_OBJECTHANDLE, NULL);
	}

	container->ObjectLock.Leave();

	return HOBJECTPTR_WRAPPER(this, hObject, &container->Object);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
void _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::FreeObject(
	OBJECTHANDLE hObject )
{
	ASSERT_RETURN(m_buckets);

#if DEBUG_NULL_AS_INVALID
	ASSERT_RETURN(hObject != NULL);
#endif

	_OBJ_CONTAINER * container = GetLockAndRefObject(hObject,FALSE);
	if(!container)
		return;

	//	undo the ref from the get
	DBG_ASSERT(container->ObjectRefCount > 0);
	--container->ObjectRefCount;

	container->ObjectDisposalFlag = TRUE;

	if(container->ObjectRefCount == 0)
	{
		AddObjectToFreePool(container);
	}
	else
	{
		InterlockedIncrement(&m_pendingFreeObjectCount);
	}

	container->ObjectLock.Leave();

	m_debugManager.FreeRequested(hObject);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
BOOL _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::AddObjectRef(
	OBJECTHANDLE hObject )
{
	ASSERT_RETFALSE(m_buckets);

	_OBJ_CONTAINER * container = GetLockAndRefObject(hObject,FALSE);
	if(!container)
		return FALSE;

	container->ObjectLock.Leave();

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
void _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::ReleaseObjectRef(
	OBJECTHANDLE hObject )
{
	ASSERT_RETURN(m_buckets);

	_OBJ_CONTAINER * container = GetLockAndRefObject(hObject,TRUE);
	if(!container)
		return;

	DBG_ASSERT(container->ObjectRefCount > 1);
	--container->ObjectRefCount;	//	one for the GetLockAndRefObject
	--container->ObjectRefCount;	//	one because that's what we're doing

	if(container->ObjectRefCount == 0 &&
	   container->ObjectDisposalFlag)
	{
		AddObjectToFreePool(container);
		InterlockedDecrement(&m_pendingFreeObjectCount);
	}

	container->ObjectLock.Leave();
}


//----------------------------------------------------------------------------
// private methods
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
void _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::InitObjectContainer(
	OBJECT_CONTAINER<T> * toInit,
	DWORD index)
{
	toInit->ObjectCounter = 0;
	toInit->ObjectIndex = index;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
typename _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::OBJECT_CONTAINER<T> *
_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::GetObjectAtIndex(
	DWORD index )
{
	if(index >= m_freeObjectIndex)
		return NULL;
	return &m_buckets[index/m_bucketSize][index%m_bucketSize];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
typename _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::OBJECT_CONTAINER<T> *
_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::AllocFreeObject(
	void )
{
	_OBJ_CONTAINER * toRet = NULL;

	while(m_freeObjectIndex >= (m_bucketCount * m_bucketSize) )
	{
		ASSERTX_RETNULL(m_bucketCount < m_maxBuckets, "Handle object pool at maxumum object count.");

		m_buckets[m_bucketCount] = (_OBJ_CONTAINER*)MALLOC(m_freeObjectMemPool,sizeof(_OBJ_CONTAINER) * m_bucketSize);
		ASSERT_RETNULL(m_buckets[m_bucketCount]);

		++m_bucketCount;
		m_freeObjectCount  += m_bucketSize;
		m_totalObjectCount += m_bucketSize;
	}

	++m_freeObjectIndex;
	toRet = GetObjectAtIndex(m_freeObjectIndex - 1);
	InitObjectContainer(toRet, m_freeObjectIndex - 1);

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
typename _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::OBJECT_CONTAINER<T> *
_HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::GetLockAndRefObject(
	OBJECTHANDLE hObject,
	BOOL bIgnoreDisposeFlag )
{
	if(hObject == INVALID_OBJECTHANDLE)
		return NULL;

	DWORD counter;
	DWORD index;
	SplitHandle(hObject, counter, index);

	_OBJ_CONTAINER * container = GetObjectAtIndex(index);
	ASSERT_RETNULL(container);

	container->ObjectLock.Enter();

	if(container->ObjectCounter != counter)
		goto _FAIL_GET;

	if(!bIgnoreDisposeFlag &&
		container->ObjectDisposalFlag)
		goto _FAIL_GET;

	++container->ObjectRefCount;

	return container;

_FAIL_GET:
	container->ObjectLock.Leave();
	m_debugManager.FailedHandleAccess(hObject);
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class _DBG_MGR>
void _HANDLE_OBJECT_POOL_BASE<T,_DBG_MGR>::AddObjectToFreePool(
	OBJECT_CONTAINER<T> * toFree)
{
	m_debugManager.DisposingObject(
		GetHandle(toFree->ObjectCounter,toFree->ObjectIndex),
		&toFree->Object);

	toFree->Object.~T();
	++toFree->ObjectCounter;

	HLCSAutoLock freeListLock = &m_freeObjectLock;

	m_freeObjectList.Push(toFree);

	++m_freeObjectCount;
	--m_liveObjectCount;
}
