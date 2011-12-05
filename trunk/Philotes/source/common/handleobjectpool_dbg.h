//----------------------------------------------------------------------------
//	handleobjectpool_dbg.h
//  Copyright 2006, Flagship Studios
//
//	Template handle system debugging managers.
//----------------------------------------------------------------------------

#pragma once


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//	non-debugging object handle system debug manager
//----------------------------------------------------------------------------
template<class T>
struct _NON_DBG_HDL_MGR
{
	BOOL Init( struct MEMORYPOOL * ) { return TRUE; }
	BOOL Init( struct MEMORYPOOL *, UINT, UINT ) { return TRUE; }
	void Free( void ) { }

	void FreeRequested( UINT64 ) { }
	void DisposingObject( UINT64, T * ) { }
	void FailedHandleAccess( UINT64 ) { }
};

//----------------------------------------------------------------------------
// entry lock
//----------------------------------------------------------------------------
DEF_HLOCK(DbgHdlObjPoolEntryLock,HLCriticalSection)
	HLOCK_LOCK_RESTRICTION(ALL_LOCKS_HLOCK_NAME)
END_HLOCK

//----------------------------------------------------------------------------
//	debugging handle debug manager
//----------------------------------------------------------------------------
template<class T, class DBG_DATA_ACCESSOR, UINT HIST_SIZE>
struct _DBG_HDL_MGR
{
private:
	//------------------------------------------------------------------------
	// TYPES
	//------------------------------------------------------------------------
	typedef typename DBG_DATA_ACCESSOR::DBG_DATA DBG_DATA_TYPE;

	template<class DATA>
	struct DBG_OBJECT_ENTRY
	{
		UINT64					id;		//	for hash
		DBG_OBJECT_ENTRY *		next;	//	for hash
		DBG_OBJECT_ENTRY *		m_next;	//	for list
		DBG_OBJECT_ENTRY *		m_prev; //	for list

		DATA					DebugData;
	};

	struct DBG_PENDING_OBJ_ENTRY
	{
		UINT64					id;		//	for hash
		DBG_PENDING_OBJ_ENTRY * next;	//	for hash
	};

	typedef HASH<	DBG_OBJECT_ENTRY<DBG_DATA_TYPE>,
					UINT64,
					HIST_SIZE >				ENTRY_HASH;

	typedef HASH<	DBG_PENDING_OBJ_ENTRY,
					UINT64,
					HIST_SIZE >				PENDING_FREE_HASH;

	typedef DOUBLE_LIST<
					DBG_OBJECT_ENTRY<DBG_DATA_TYPE> >
											ENTRY_LIST;


	//------------------------------------------------------------------------
	// DATA
	//------------------------------------------------------------------------
	struct MEMORYPOOL *		m_entryMemPool;
	ENTRY_HASH				m_entryHash;
	ENTRY_LIST				m_entryList;
	UINT					m_entryCount;
	PENDING_FREE_HASH		m_pendingEntryHash;
	UINT					m_pendingEntryCount;
	DbgHdlObjPoolEntryLock	m_entryLock;

public:
	//------------------------------------------------------------------------
	// PUBLIC METHODS
	//------------------------------------------------------------------------

	//------------------------------------------------------------------------
	BOOL Init(
		MEMORYPOOL * pool )
	{
		m_entryMemPool = pool;
		m_entryHash.Init();
		m_entryList.Init();
		m_entryCount = 0;
		m_entryLock.Init();
		m_pendingEntryHash.Init();
		m_pendingEntryCount = 0;
		return TRUE;
	}

	//------------------------------------------------------------------------
	void Free(
		void )
	{
		m_entryLock.Free();
		m_entryHash.Destroy(m_entryMemPool);
		m_entryList.Init();
		m_pendingEntryHash.Destroy(m_entryMemPool);
		m_entryCount = 0;
		m_pendingEntryCount = 0;
	}

	//------------------------------------------------------------------------
	void FreeRequested(
		UINT64 hObject )
	{
		HLCSAutoLock lock = &m_entryLock;

		DBG_PENDING_OBJ_ENTRY * added = m_pendingEntryHash.Add(m_entryMemPool, hObject);
		ASSERT_RETURN(added);

		++m_pendingEntryCount;

		DBG_DATA_ACCESSOR::OnPendingFreeAdded(hObject,m_pendingEntryCount);
	}

	//------------------------------------------------------------------------
	void DisposingObject(
		UINT64 hObject,
		T * pObject )
	{
		HLCSAutoLock lock = &m_entryLock;

		if(m_pendingEntryHash.Remove(hObject))
		{
			--m_pendingEntryCount;
			DBG_DATA_ACCESSOR::OnPendingFreeRemoved(hObject,m_pendingEntryCount);
		}

		if(m_entryCount >= HIST_SIZE)
		{
			DBG_OBJECT_ENTRY<DBG_DATA_TYPE> * oldest = m_entryList.RemoveTail();
			ASSERT_RETURN(oldest);
			ASSERT_RETURN(m_entryHash.Remove(oldest->id));
			--m_entryCount;
		}

		DBG_OBJECT_ENTRY<DBG_DATA_TYPE> * entry = m_entryHash.Add(m_entryMemPool,hObject);
		ASSERT_RETURN(entry);
		m_entryList.AddToHead(entry);
		++m_entryCount;

		DBG_DATA_ACCESSOR::OnObjectDisposing(pObject,&entry->DebugData);
	}

	//------------------------------------------------------------------------
	void FailedHandleAccess(
		UINT64 hObject )
	{
		HLCSAutoLock lock = &m_entryLock;

		DBG_OBJECT_ENTRY<DBG_DATA_TYPE> * entry = m_entryHash.Get(hObject);
		
		if(entry)
		{
			DBG_DATA_ACCESSOR::OnFailedHandleAccessWithData(hObject,&entry->DebugData);
		}
		else
		{
			DBG_DATA_ACCESSOR::OnFailedHandleAccessWithoutData(hObject);
		}
	}
};
