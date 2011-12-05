//----------------------------------------------------------------------------
// array.h
//
// (C)Copyright 2004, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _ARRAY_H_
#define _ARRAY_H_

//-----------------------------------------------------------------------------
// INCLUDES
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------
#define ArrayInit(array, pool, size)					(array).Init(MEMORY_FUNC_FILELINE(pool, size))
#define ArrayInitFL(array, pool, size, file, line)		(array).Init(MEMORY_FUNC_PASSFL(pool, size))
#define ArrayCopy(array, src)							(array).Init(MEMORY_FUNC_FILELINE(src))
#define ArrayCopyFL(array, src, file, line)				(array).Init(MEMORY_FUNC_PASSFL(src))
#define ArrayAdd(array)									(array).Add(MEMORY_FUNC_FILELINE((int *)NULL))
#define ArrayAddGetIndex(array, iptr)					(array).Add(MEMORY_FUNC_FILELINE(iptr))
#define ArrayAddItem(array, data)						(array).Add(MEMORY_FUNC_FILELINE(data))
#define ArrayAddItemFL(array, data, file, line)			(array).Add(MEMORY_FUNC_PASSFL(data))
#define ArrayRemove(array, index)						(array).RemoveByIndex(MEMORY_FUNC_FILELINE(index))
#define ArrayRemoveByIndex(array, index)				(array).RemoveByIndex(MEMORY_FUNC_FILELINE(index))
#define ArrayRemoveByValue(array, value)				(array).RemoveByValue(MEMORY_FUNC_FILELINE(value))
#define ArrayClear(array)								(array).Clear(MEMORY_FUNC_FILELINE(0))
#define ArrayClearUpTo(index)							(array).Clear(MEMORY_FUNC_FILELINE(index))


//-----------------------------------------------------------------------------
// Name: ArrayListType
// Desc: Indicates how data should be stored in a CArrayList
//-----------------------------------------------------------------------------
enum ArrayListType
{
    AL_VALUE,       // entry data is copied into the list
    AL_REFERENCE,   // entry pointers are copied into the list
};


//-----------------------------------------------------------------------------
// Name: CArrayList
// Desc: A growable array - it moved memory on delete, and reallocs
//-----------------------------------------------------------------------------
class CArrayList
{
protected:
    ArrayListType	m_eArrayListType;
    void*			m_pData;
    UINT			m_nBytesPerEntry;
    UINT			m_nEntriesCount;
    UINT			m_nEntriesAllocated;

public:
    CArrayList(
		ArrayListType eType = AL_VALUE,
		UINT nBytesPerEntry = 0);

    ~CArrayList(
		void);

	void Init(
		ArrayListType eType = AL_VALUE,
		UINT nBytesPerEntry = 0);

	PRESULT Add(
		MEMORY_FUNCARGS(void* pEntry));

    void Remove(
		UINT nEntry);

	void* GetPtr(
		UINT nEntry);

    const void* GetPtr(
		UINT nEntry) const;

    UINT Count(
		void) const 
	{ 
		return m_nEntriesCount; 
	}

    BOOL Contains(
		void* pEntryData);

	void Clear(
		void)
	{ 
		m_nEntriesCount = 0; 
	}

    void Destroy(
		void);

	BOOL IsEmpty(
		void) const
	{
		return m_nEntriesCount == 0;	
	}

	BOOL Sort(
		int (__cdecl *fnCompare)(const void *, const void *));
};


//-----------------------------------------------------------------------------
// Name: CArrayIndexed
// Desc: Also called a static (double) linked list - which grows if necessary
// The Id of an entry is preserved.  We trade some memory for speed in erasing by doubly linking.
// We can also hold large structures since we don't do much memory moving
// This is all in the header file to help it compile properly - template classes like to be inlined.
//-----------------------------------------------------------------------------
template <class T>
class CArrayIndexed
{
protected:
	struct MEMORYPOOL *					m_pPool;
	T *									m_pData;
	int *								m_pnIdPrevNext;
#if DEBUG_MEMORY_ALLOCATIONS
	const char *						m_File;
	int									m_nLine;
#endif
	int									m_nIdFirst;
	int									m_nIdNextFree;
	int									m_nEntriesAllocated;

	int GetNewNode(
		void)
	{
		if (m_nIdNextFree >= m_nEntriesAllocated)
		{
			int nOldEntriesAllocated = m_nEntriesAllocated;
			m_nEntriesAllocated *= 2;
			m_pData = (T *)REALLOCFL(m_pPool, m_pData, sizeof(T) * m_nEntriesAllocated, m_File, m_nLine);
			m_pnIdPrevNext = (int *)REALLOCFL(m_pPool, m_pnIdPrevNext, sizeof(int) * 2 * m_nEntriesAllocated, m_File, m_nLine);
			for (int ii = 2 * nOldEntriesAllocated; ii < 2 * m_nEntriesAllocated; ii++)
			{
				m_pnIdPrevNext[ii] = INVALID_ID;
			}
		}

		int nNewId = m_nIdNextFree;

		// update where the next unused entry is located
		m_nIdNextFree = m_pnIdPrevNext[2 * m_nIdNextFree + 1];
		if (m_nIdNextFree == INVALID_ID)
		{
			m_nIdNextFree = nNewId + 1;
		}

		return nNewId;
	}

	inline void SetNext(
		int nCurr,
		int nNext)
	{
		m_pnIdPrevNext[nCurr * 2 + 1] = nNext;
	}

	inline void SetPrev(
		int nCurr,
		int nPrev)
	{
		m_pnIdPrevNext[nCurr * 2 + 0] = nPrev;
	}

public:
	CArrayIndexed() 
		: m_nEntriesAllocated(0),
		m_nIdNextFree(0),
		m_nIdFirst(INVALID_ID),
		m_pData(NULL),
		m_pnIdPrevNext(NULL),
		m_pPool(NULL)
	{
	}

	virtual ~CArrayIndexed(
		void)
	{
		Destroy();
	}

	void Destroy(
		void)
	{
		m_nIdFirst = INVALID_ID;
		if (m_pData)
		{
			FREE(m_pPool, m_pData);
		}
		m_pData = NULL;

		if (m_pnIdPrevNext)
		{
			FREE(m_pPool, m_pnIdPrevNext);
		}
		m_pnIdPrevNext = NULL;
		m_nEntriesAllocated = 0;
	}

	BOOL Initialized()
	{
		return BOOL(m_pData != NULL);
	}

	void Init(
		MEMORY_FUNCARGS(struct MEMORYPOOL * pool,
		int nInitSize))
	{
		ASSERT_RETURN(m_pData == NULL);

		m_pPool = pool;
#if DEBUG_MEMORY_ALLOCATIONS
		m_File = file;
		m_nLine = line;
#endif
		if (nInitSize > 0)
		{
			m_nEntriesAllocated = nInitSize;
		}
		ASSERT_RETURN(m_nEntriesAllocated > 0);
		m_nIdFirst = INVALID_ID;
		m_nIdNextFree = 0;
		m_pData = (T *)MALLOCFL(m_pPool, sizeof(T) * m_nEntriesAllocated, m_File, m_nLine);
		m_pnIdPrevNext = (int *)MALLOCFL(m_pPool, sizeof(int) * 2 * m_nEntriesAllocated, m_File, m_nLine);
		for (int ii = 0; ii < 2 * m_nEntriesAllocated; ii++)
		{
			m_pnIdPrevNext[ii] = INVALID_ID;
		}
	}

	T * Add(
		int* pnNewId,
		BOOL fAddEnd = FALSE)
	{
		int nNewId = GetNewNode();

		// set the next and previous indices for the new node
		if (fAddEnd)
		{
			int nIdLast = INVALID_ID;
			for (int nIdEnd = m_nIdFirst; nIdEnd != (int)INVALID_ID; nIdEnd = GetNextId(nIdEnd))
			{
				nIdLast = nIdEnd;
			}
			SetPrev(nNewId, nIdLast);
			SetNext(nNewId, INVALID_ID);
			// change the last entry
			if (nIdLast != INVALID_ID)
			{
				SetNext(nIdLast, nNewId);
			}
			else
			{
				m_nIdFirst = nNewId;	
			}
		} 
		else 
		{
			SetPrev(nNewId, INVALID_ID);
			SetNext(nNewId, m_nIdFirst);
			// update the first index
			if (m_nIdFirst != INVALID_ID)
			{
				SetPrev(m_nIdFirst, nNewId);
			}
			m_nIdFirst = nNewId;
		}

		if (pnNewId != NULL)
		{
			*pnNewId = nNewId;
		}

		return &m_pData[nNewId];
	}

	typedef int (*PFN_COMPARE_FUNCTION)(T * pFirst, void * pKey);
	T * InsertSorted(
		int * pnNewId,
		void * pKey,
		PFN_COMPARE_FUNCTION pfnLessThan)
	{
		ASSERT_RETNULL(pnNewId);
		*pnNewId = INVALID_ID;
		ASSERT_RETNULL(pfnLessThan);
		int nNewId = GetNewNode();
		*pnNewId = nNewId;

		int nNext = INVALID_ID;
		int nLast = INVALID_ID;
		for (int nTestId = m_nIdFirst; nTestId != INVALID_ID; nTestId = GetNextId(nTestId))
		{
			nLast = nTestId;
			int nCompare = pfnLessThan(Get(nTestId), pKey);
			if (nCompare >= 0)
			{
				nNext = nTestId;
				break;
			}
		}

		SetNext(nNewId, nNext);
		if (nNext != INVALID_ID)
		{	// insert in the middle or beginning
			int nPrev = GetPrevId(nNext);
			SetPrev(nNewId, nPrev);
			SetPrev(nNext, nNewId);
			if (nPrev != INVALID_ID)
			{
				SetNext(nPrev, nNewId);
			}
			else
			{
				m_nIdFirst = nNewId;
			}
		}
		else if (m_nIdFirst != INVALID_ID)
		{	// insert at the end
			ASSERT_RETNULL(nLast != INVALID_ID);
			SetNext(nLast, nNewId);
			SetPrev(nNewId, nLast);
		}
		else
		{	// insert only
			SetPrev(nNewId, INVALID_ID);
			m_nIdFirst = nNewId;
		}

		return &m_pData[nNewId];
	}

	// if you call remove on the same node twice, it should just move it within the unused list
	void Remove(
		int nId)
	{
		ASSERT_RETURN(nId >= 0 && nId < m_nEntriesAllocated);
		int nPrev = GetPrevId(nId);
		int nNext = GetNextId(nId);
		ASSERT_RETURN(nNext < m_nEntriesAllocated);
		ASSERT_RETURN(nPrev < m_nEntriesAllocated);

		// update neighboring indices
		if (nPrev != INVALID_ID)
		{
			SetNext(nPrev, nNext);
		}
		if (nNext != INVALID_ID)
		{
			SetPrev(nNext, nPrev);
		}

		// update the first
		if (m_nIdFirst == nId)
		{
			m_nIdFirst = GetNextId(nId);
		}

		// Add node to the list of unused nodes
		SetPrev(nId, INVALID_ID);
		SetNext(nId, m_nIdNextFree);
		m_nIdNextFree = nId;
	}

	T * Get(
		int nId)
	{
		if ( ! m_pData )
			return NULL;
		if (nId == INVALID_ID)
		{
			return NULL;
		}
		ASSERT_RETNULL(nId >= 0 && nId < m_nEntriesAllocated);
		return &m_pData[nId];
	}

	const T * Get(
		int nId) const
	{
		ASSERT_RETNULL(m_pData);
		if (nId == INVALID_ID)
		{
			return NULL;
		}
		ASSERT_RETNULL(nId >= 0 && nId < m_nEntriesAllocated);
		return &m_pData[nId];
	}

	int GetNextId(
		int nId) const
	{
		ASSERT_RETINVALID(nId >= 0 && nId < m_nEntriesAllocated);
		return m_pnIdPrevNext[2 * nId + 1];
	}

	int GetPrevId(
		int nId) const
	{
		ASSERT_RETINVALID(nId >= 0 && nId < m_nEntriesAllocated);
		return m_pnIdPrevNext[2 * nId + 0];
	}

	int GetFirst(
		void) const
	{ 
		return m_nIdFirst;
	}

	int GetLast(
		void) const
	{
		int nNext = INVALID_ID;
		int nCurr = GetFirst(); 
		while (nCurr != INVALID_ID)
		{
			nNext = GetNextId(nCurr);
			if (nNext == INVALID_ID)
			{
				return nCurr;
			}
			nCurr = nNext;
		}

		return INVALID_ID;
	}

	void Clear(
		void)
	{ 
		if (m_nIdFirst == INVALID_ID)
		{
			return;
		}
		// yes, this is a little slow.  
		// We could connect the used and unused lists together instead, but this is clean
		m_nIdFirst = INVALID_ID;
		for (int ii = 0; ii < 2 * m_nEntriesAllocated; ii++)
		{
			m_pnIdPrevNext[ii] = INVALID_ID;
		}
		m_nIdNextFree = 0;
	}

	UINT Count(
		void) const
	{ 
		int nCount = 0;
		for (int nCurr = GetFirst(); nCurr != INVALID_ID; nCurr = GetNextId(nCurr))
		{
			nCount++;
		}
		return nCount;
	}

	BOOL IsEmpty(
		void) const
	{ 
		return m_nIdFirst == INVALID_ID;
	}

	void OrderForSpeed(
		void)
	{ 
		if ( ! m_nEntriesAllocated )
			return;
		// mark the currently used
		for ( int nCurr = GetFirst(); nCurr != INVALID_ID; nCurr = GetNextId(nCurr) )
		{
			SetPrev( nCurr, -2 );
		}

		// link the currently used in order
		int * pCurrIndex = &m_pnIdPrevNext[ 0 ];
		int nPrev = INVALID_ID;
		int * pPrevIndex = NULL;
		for ( int nCurr = 0; nCurr < m_nEntriesAllocated; nCurr++ )
		{
			if ( *pCurrIndex == -2 )
			{
				*pCurrIndex = nPrev;
				if ( pPrevIndex )
					*pPrevIndex = nCurr;
				else
					m_nIdFirst = nCurr;
				pCurrIndex++;
				pPrevIndex = pCurrIndex;
				pCurrIndex++;
				nPrev = nCurr;
			} else {
				pCurrIndex++;
				pCurrIndex++;
			}
		}
		if ( pPrevIndex )
			*pPrevIndex = INVALID_ID;
	}
};


//-----------------------------------------------------------------------------
// Name: CArrayIndexedEx
// dynamic doubly linked list, doesn't reallocate already allocated items
//-----------------------------------------------------------------------------
template <class T, unsigned int ALLOC_SIZE>
class CArrayIndexedEx
{
protected:
	struct Node
	{
		unsigned int					m_Prev;
		unsigned int					m_Next;
		T								m_Item;
	};
	static const unsigned int ITEMS_PER_BUCKET = ALLOC_SIZE / sizeof(Node);

protected:
	struct MEMORYPOOL *					m_Pool;
	Node **								m_Buckets;			

#if DEBUG_MEMORY_ALLOCATIONS
	const char *						m_File;
	unsigned int						m_Line;
#endif

	unsigned int						m_Head;				
	unsigned int						m_Tail;				
	unsigned int						m_Count;
	unsigned int						m_Garbage;			
		
	unsigned int						m_BucketCount;
	unsigned int						m_NumAllocated;

	void Expand(
		void)
	{
		m_Buckets = (Node **)REALLOCFL(m_Pool, m_Buckets, sizeof(Node *) * (m_BucketCount + 1), m_File, m_Line);
		Node * bucket = (Node *)MALLOCFL(m_Pool, sizeof(Node) * ITEMS_PER_BUCKET, m_File, m_Line);
		m_Buckets[m_BucketCount] = bucket;
		for (unsigned int ii = 0; ii < ITEMS_PER_BUCKET; ++ii)
		{
			Node * node = bucket + ii;
			node->m_Prev = INVALID_ID;
			node->m_Next = INVALID_ID;
		}
		++m_BucketCount;
	}

	Node * GetNode(
		unsigned int index)
	{
		DBG_ASSERT_RETNULL(index < m_BucketCount * ITEMS_PER_BUCKET);
		Node * bucket = m_Buckets[index / ITEMS_PER_BUCKET];
		DBG_ASSERT_RETNULL(bucket);
		return bucket + index % ITEMS_PER_BUCKET;
	}

	const Node * GetNode_Const(
		unsigned int index) const
	{
		DBG_ASSERT_RETNULL(index < m_BucketCount * ITEMS_PER_BUCKET);
		Node * bucket = m_Buckets[index / ITEMS_PER_BUCKET];
		DBG_ASSERT_RETNULL(bucket);
		return bucket + index % ITEMS_PER_BUCKET;
	}

	Node * GetNewNode(
		unsigned int & id)
	{
		if (m_Garbage != INVALID_ID)
		{
			id = m_Garbage;
			Node * node = GetNode(id);
			DBG_ASSERT_RETNULL(node);
			m_Garbage = node->m_Next;
			node->m_Prev = INVALID_ID;
			node->m_Next = INVALID_ID;
			++m_Count;
			return node;
		}
		if (m_NumAllocated >= m_BucketCount * ITEMS_PER_BUCKET)
		{
			Expand();
		}
		DBG_ASSERT_RETNULL(m_NumAllocated < m_BucketCount * ITEMS_PER_BUCKET);
		id = m_NumAllocated;
		Node * node = GetNode(id);
		DBG_ASSERT_RETNULL(node);
		node->m_Prev = INVALID_ID;
		node->m_Next = INVALID_ID;
		++m_NumAllocated;
		++m_Count;
		return node;
	}

public:
	//-------------------------------------------------------------------------
	CArrayIndexedEx(
		void) : m_Pool(NULL), m_Buckets(NULL), m_Head(INVALID_ID), m_Tail(INVALID_ID), m_Count(0),
		m_Garbage(INVALID_ID), m_BucketCount(INVALID_ID), m_NumAllocated(INVALID_ID)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		m_File = NULL;
		m_Line = 0;
#endif
		DBG_ASSERT(ITEMS_PER_BUCKET >= 8);
	}

	//-------------------------------------------------------------------------
	virtual ~CArrayIndexedEx(
		void)
	{
		Destroy();
	}

	//-------------------------------------------------------------------------
	BOOL Initialized(
		void)
	{
		return BOOL(m_BucketCount != INVALID_ID);
	}

	//-------------------------------------------------------------------------
	void Destroy(
		void)
	{
		if (!Initialized())
		{
			return;
		}
		if (m_Buckets)
		{
			for (unsigned int ii = 0; ii < m_BucketCount; ++ii)
			{
				Node * bucket = m_Buckets[ii];
				ASSERT_CONTINUE(bucket);			
				FREE(m_Pool, bucket);
			}
			FREE(m_Pool, m_Buckets);
			m_Buckets = NULL;
		}
		m_BucketCount = INVALID_ID;
		m_Head = INVALID_ID;
		m_Tail = INVALID_ID;
		m_Count = 0;
		m_Garbage = INVALID_ID;
		m_NumAllocated = INVALID_ID;
	}

	//-------------------------------------------------------------------------
	void Init(
		MEMORY_FUNCARGS(struct MEMORYPOOL * pool,
		unsigned int initsize))
	{
		ASSERT_RETURN(!Initialized());

		m_Pool = pool;
#if DEBUG_MEMORY_ALLOCATIONS
		m_File = file;
		m_Line = line;
#endif
		m_Count = 0;
		m_BucketCount = 0;
		m_NumAllocated = 0;

		// allocate initial elements...
		while (m_BucketCount * ITEMS_PER_BUCKET < initsize)
		{
			Expand();
		}
	}

	//-------------------------------------------------------------------------
	T * Add(
		int * newid)
	{
		unsigned int id;
		Node * node = GetNewNode(id);
		DBG_ASSERT(node);

		node->m_Next = m_Head;
		if (m_Head == INVALID_ID)
		{
			ASSERT(m_Tail == INVALID_ID);
			m_Head = id;
			m_Tail = id;
		}
		else
		{
			Node * head = GetNode(m_Head);
			DBG_ASSERT(head);
			head->m_Prev = id;
			m_Head = id;
		}

		if (newid)
		{
			*newid = (int)id;
		}
		return &node->m_Item;
	}

	//-------------------------------------------------------------------------
	T * Add(
		int * newid,
		BOOL fAddToTail)
	{
		if (!fAddToTail)
		{
			return Add(newid);
		}
		unsigned int id;
		Node * node = GetNewNode(id);
		DBG_ASSERT(node);

		node->m_Prev = m_Tail;
		if (m_Head == INVALID_ID)
		{
			DBG_ASSERT(m_Tail == INVALID_ID);
			m_Head = id;
			m_Tail = id;
		}
		else
		{
			Node * tail = GetNode(m_Tail);
			DBG_ASSERT(tail);
			tail->m_Next = id;
			m_Tail = id;
		}
		
		if (newid)
		{
			*newid = (int)id;
		}
		return &node->m_Item;
	}

	//-------------------------------------------------------------------------
	typedef int (* PFN_COMPARE_FUNCTION)(T * item, void * key);
	T * InsertSorted(
		int * newid,
		void * key,
		PFN_COMPARE_FUNCTION fpLessThan)
	{
		if (newid)
		{
			*newid = INVALID_ID;
		}
		DBG_ASSERT_RETNULL(fpLessThan);

		unsigned int id;
		Node * node = GetNewNode(id);
		DBG_ASSERT_RETNULL(node);

		Node * prev = NULL;
		Node * next = NULL;
		unsigned int previd = INVALID_ID;
		unsigned int nextid = m_Head; 
		while (nextid != INVALID_ID)
		{
			next = GetNode(nextid);
			DBG_ASSERT(next);
			int compare = fpLessThan(&next->m_Item, key);
			if (compare >= 0)
			{
				break;
			}
			prev = next;
			previd = nextid;
			nextid = next->m_Next;
		}

		node->m_Prev = previd;
		node->m_Next = nextid;
		if (prev)
		{
			prev->m_Next = id;
		}
		else
		{
			m_Head = id;
		}
		if (next)
		{
			next->m_Prev = id;
		}
		else
		{
			m_Tail = id;
		}

		if (newid)
		{
			*newid = (int)id;
		}
		return &node->m_Item;
	}

	//-------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
	BOOL NodeIsNotOnGarbage(
		const Node * node,
		unsigned int id) const
	{
		while (node->m_Prev != INVALID_ID)
		{
			id = node->m_Prev;
			node = GetNode_Const(id);
			ASSERT(node);	
		}
		return (id == m_Head);
	}
#endif

	//-------------------------------------------------------------------------
	void Remove(
		unsigned int id)
	{
		ASSERT_RETURN(id < m_NumAllocated);
		Node * node = GetNode(id);
		DBG_ASSERT_RETURN(node);

#if ISVERSION(DEVELOPMENT)
		DBG_ASSERT_RETURN(NodeIsNotOnGarbage(node, id));
#endif
		if (node->m_Prev != INVALID_ID)
		{
			Node * prev = GetNode(node->m_Prev);
			DBG_ASSERT(prev);
			prev->m_Next = node->m_Next;
		}
		else
		{
			DBG_ASSERT(m_Head == id);
			m_Head = node->m_Next;
		}
		if (node->m_Next != INVALID_ID)
		{
			Node * next = GetNode(node->m_Next);
			DBG_ASSERT(next);
			next->m_Prev = node->m_Prev;
		}
		else
		{
			DBG_ASSERT(m_Tail == id);
			m_Tail = node->m_Prev;
		}
		--m_Count;

		// add the node to the garbage
		node->m_Prev = INVALID_ID;
		node->m_Next = m_Garbage;
		m_Garbage = id;
	}

	//-------------------------------------------------------------------------
	T * Get(
		unsigned int id)
	{
		if (id == INVALID_ID)
		{
			return NULL;
		}
		ASSERT_RETNULL(id < m_NumAllocated);
		Node * node = GetNode(id);
		DBG_ASSERT_RETNULL(node);
		return &node->m_Item;
	}

	//-------------------------------------------------------------------------
	const T * Get(
		unsigned int id) const
	{
		if (id == INVALID_ID)
		{
			return NULL;
		}
		ASSERT_RETNULL(id < m_NumAllocated);
		const Node * node = GetNode_Const(id);
		DBG_ASSERT_RETNULL(node);
		return &node->m_Item;
	}

	//-------------------------------------------------------------------------
	unsigned int GetNextId(
		unsigned int id) const
	{
		ASSERT_RETINVALID(id < m_NumAllocated);
		const Node * node = GetNode_Const(id);
		DBG_ASSERT_RETNULL(node);
		return node->m_Next;
	}

	//-------------------------------------------------------------------------
	unsigned int GetPrevId(
		unsigned int id) const
	{
		ASSERT_RETINVALID(id < m_NumAllocated);
		const Node * node = GetNode_Const(id);
		DBG_ASSERT_RETNULL(node);
		return node->m_Prev;
	}

	//-------------------------------------------------------------------------
	int GetFirst(
		void) const
	{ 
		return m_Head;
	}

	//-------------------------------------------------------------------------
	int GetLast(
		void) const
	{
		return m_Tail;
	}

	//-------------------------------------------------------------------------
	void Clear(
		void)
	{ 
		while (m_Head)
		{
			Remove(m_Head);
		}
	}

	//-------------------------------------------------------------------------
	unsigned int Count(
		void) const
	{ 
		return m_Count;
	}

	//-------------------------------------------------------------------------
	BOOL IsEmpty(
		void) const
	{ 
		return (m_Head == INVALID_ID);
	}

	//-------------------------------------------------------------------------
	// reorder the list by id
	void OrderForSpeed(
		void)
	{ 
		if (!Initialized())
		{
			return;
		}
		// there's no obvious way to order this type of list for speed that
		// wouldn't probably be slower than whatever gains would occur for being ordered
		ASSERT(0);
	}
};


//-----------------------------------------------------------------------------
// A straightforward array list, auto-resizing.  It holds full copies of the object passed to Add().
// The ID of an entry is NOT preserved.  On remove, the array is shifted.  Order is preserved.
// Add always appends to the end.
//-----------------------------------------------------------------------------
template <class T, int ALIGN>
class SIMPLE_DYNAMIC_ARRAY_ALIGNED
{
protected:
	MEMORYPOOL *				pool;
	T *							list;
	unsigned int				count;
	unsigned int				size;

	void Grow(MEMORY_FUNCNOARGS())
	{
		MEMORY_DEBUG_REF(file);
		MEMORY_DEBUG_REF(line);
		
		if (list == NULL)
		{		
			ASSERT(size > 0);
#pragma warning(suppress:4127) //conditional expression is constant
			if ( ALIGN )	
				list = (T *)ALIGNED_MALLOCPASSFL(pool, size * sizeof(T), ALIGN);
			else
				list = (T *)MALLOCPASSFL(pool, size * sizeof(T));
		}
		else if (count >= size)
		{
			size *= 2;
#pragma warning(suppress:4127) //conditional expression is constant
			if ( ALIGN )					
				list = (T *)ALIGNED_REALLOCPASSFL(pool, list, size * sizeof(T), ALIGN);
			else
				list = (T *)REALLOCPASSFL(pool, list, size * sizeof(T));
		}
	}

	void Shrink(MEMORY_FUNCNOARGS())
	{
		if (count < size / 2)
		{
			size /= 2;
#pragma warning(suppress:4127) //conditional expression is constant
			if ( ALIGN )
				list = (T *)ALIGNED_REALLOCPASSFL(pool, list, size * sizeof(T), ALIGN);
			else
				list = (T *)REALLOCPASSFL(pool, list, size * sizeof(T));
		}
	}

public:
	~SIMPLE_DYNAMIC_ARRAY_ALIGNED(
		void)
	{
		Destroy();
	}

	BOOL IsInitialized(
		void)
	{
		return (list != NULL);
	}

	void Init(
		MEMORY_FUNCARGS(MEMORYPOOL * _pool,
		unsigned int _size))
	{
		MEMORY_DEBUG_REF(file);
		MEMORY_DEBUG_REF(line);

		pool = _pool;
		count = 0;
		size = max(_size, 1);
		list = NULL;
	}

	void Init( 
		MEMORY_FUNCARGS(const SIMPLE_DYNAMIC_ARRAY_ALIGNED<T, ALIGN> & _array))
	{
		pool = _array.pool;
		count = _array.count;
		size = _array.size;
		if (_array.list)
		{
#pragma warning(suppress:4127) //conditional expression is constant
			if ( ALIGN )
				list = (T *)ALIGNED_MALLOCPASSFL(pool, size * sizeof(T), ALIGN);
			else
				list = (T *)MALLOCPASSFL(pool, size * sizeof(T));
			memcpy(list, _array.list, count * sizeof(T));
		}
		else
		{
			list = NULL;
		}
	}

	void Resize(
		MEMORY_FUNCARGS(unsigned int newSize))
	{
		MEMORY_DEBUG_REF(file);
		MEMORY_DEBUG_REF(line);

		ASSERT(newSize);
		
		size = newSize;
		count = size;

#pragma warning(suppress:4127) //conditional expression is constant
		if ( ALIGN )					
			list = (T *)ALIGNED_REALLOCPASSFL(pool, list, size * sizeof(T), ALIGN);
		else
			list = (T *)REALLOCPASSFL(pool, list, size * sizeof(T));
	}

	// slow!!!
	int GetIndex(
		const T & key)
	{
		for (unsigned int ii = 0; ii < count; ++ii)
		{
			if (list[ii] == key)
			{
				return (int)ii;
			}
		}
		return INVALID_ID;
	}

	// yeah, a little dangerous, but much faster to use
	T * GetPointer(
		unsigned int index) const
	{
		ASSERT(index < count);		// not assert return for speed
		ASSERT(list);
		if(list)
		{
			return &list[index];
		}
		else
		{
			return NULL;
		}
	}

	unsigned int Add(
		MEMORY_FUNCARGS(const T & item))
	{
		Grow(MEMORY_FUNCNOARGS_PASSFL());
		list[count] = item;
		return count++;
	}

	T * Add(
		MEMORY_FUNCARGS(unsigned int * newid))
	{
		Grow(MEMORY_FUNCNOARGS_PASSFL());
		T * item = &list[count];
		if (newid)
		{
			*newid = count;
		}
		++count;
		return item;
	}

	T * Add(
		MEMORY_FUNCARGS(int * newid))
	{
		Grow(MEMORY_FUNCNOARGS_PASSFL());
		T * item = &list[count];
		if (newid)
		{
			*newid = (int)count;
		}
		++count;
		return item;
	}

	BOOL RemoveByIndex(
		MEMORY_FUNCARGS(unsigned int index))
	{
		ASSERT_RETFALSE(index < count);
		count--;
		ASSERT_RETFALSE(MemoryMove(&list[index], (count - index) * sizeof(T), &list[index + 1], sizeof(T) * (count - index)));
		Shrink(MEMORY_FUNCNOARGS_PASSFL());
		return TRUE;
	}

	// slow!!!!
	BOOL RemoveByValue(
		MEMORY_FUNCARGS(const T & key))
	{
		int ii = GetIndex(key);
		if (ii == INVALID_ID)
		{
			return FALSE;
		}
		return RemoveByIndex(MEMORY_FUNC_PASSFL((unsigned int)ii));
	}

	void Clear(
		MEMORY_FUNCARGS(unsigned int upto))
	{
		count = upto;
		Shrink(MEMORY_FUNCNOARGS_PASSFL());
	}

	void QSort(
		int (__cdecl *pfnCompare)(const void *, const void *))
	{
		if (list)
		{
			qsort(list, count, sizeof(T), pfnCompare);
		}
	}

	void QSortEx(
		unsigned int start,
		unsigned int _count, 
		int (__cdecl *pfnCompare)( const void *, const void *))
	{
		ASSERT(start + _count <= count);
		if (list)
		{
			qsort(GetPointer(start), _count, sizeof(T), pfnCompare);
		}
	}

	void Sort(
		int (__cdecl *pfnCompare)(const void *, const void *))
	{
		return QSort(pfnCompare);
	}

	const T & operator[](
		unsigned int index) const
	{
		ASSERT(index < count);			// not assert return for speed
		ASSERT(list);
#pragma warning(suppress:6011)
		return list[index];
	}

	T & operator[](
		unsigned int index)
	{
		ASSERT(index < count);			// not assert return for speed
		ASSERT(list);
#pragma warning(suppress:6011)
		return list[index];
	}

	const T & operator[](
		int index) const
	{
		ASSERT((unsigned int)index < count);		// not assert return for speed
		ASSERT(list);
		return list[index];
	}

	T & operator[](
		int index)
	{
		ASSERT((unsigned int)index < count);		// not assert return for speed
		ASSERT(list);
#pragma warning(suppress:6011) // de-ref NULL
		return list[index];
	}

	operator const T *(
		void) const
	{
		return list;
	}

	unsigned int Count(
		void) const
	{
		return count;
	}

	BOOL IsEmpty(
		void) const
	{
		return count == 0;
	}

	void Destroy(
		void)
	{
		if (list)
		{
			FREE(pool, list);
			list = NULL;
		}
		count = 0;
		size = 0;
	}
};

template <class T>
class SIMPLE_DYNAMIC_ARRAY : public SIMPLE_DYNAMIC_ARRAY_ALIGNED<T,0> {};


//-----------------------------------------------------------------------------
// like SIMPLE_DYNAMIC_ARRAY but adds indexing method
//-----------------------------------------------------------------------------
enum ARRAY_INDEXING_METHOD
{
	ARRAY_INDEXING_METHOD_FORWARD,
	ARRAY_INDEXING_METHOD_REVERSE
};

template <class T>
class SIMPLE_DYNAMIC_ARRAY_EX
{
protected:

	unsigned int m_nElementCount;
	unsigned int m_nAllocCount;
	ARRAY_INDEXING_METHOD m_nIndexingMethod;
	T * m_pElements;
	MEMORYPOOL * m_pPool;

public:
	virtual ~SIMPLE_DYNAMIC_ARRAY_EX( void)
	{
		Destroy();
	}
	void SetIndexingMethod( ARRAY_INDEXING_METHOD nMethod )
	{
		ASSERT( IsInitialized() );
		m_nIndexingMethod = nMethod;
	}
	BOOL IsInitialized()
	{
		return m_nAllocCount != 0;
	}
	void Init( MEMORY_FUNCARGS( MEMORYPOOL *pool, unsigned int nStartCount ) )
	{
		m_pPool = pool;
		m_nAllocCount   = nStartCount;
		m_nElementCount = 0;
		m_nIndexingMethod = ARRAY_INDEXING_METHOD_FORWARD;
		m_pElements     = (T*) MALLOCPASSFL( m_pPool, m_nAllocCount * sizeof(T));
		ASSERT( m_pElements );
	}
	void Init( MEMORY_FUNCARGS( const SIMPLE_DYNAMIC_ARRAY<T> & tArr, MEMORYPOOL *pool ) )
	{
		m_pPool = pool;
		m_nAllocCount   = tArr.m_nAllocCount;
		m_nElementCount = tArr.m_nElementCount;
		m_nIndexingMethod = ARRAY_INDEXING_METHOD_FORWARD;
		m_pElements     = (T*) MALLOCPASSFL( m_pPool, m_nAllocCount * sizeof(T));
		ASSERT( m_pElements );
		ASSERT( tArr.m_pElements );
		memcpy(m_pElements, tArr.m_pElements, tArr.m_nElementCount * sizeof(T));
	}
	int Add( MEMORY_FUNCARGS( const T & tValue ) )
	{
		ASSERT( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD );
		ASSERT( m_nAllocCount != 0 );
		if ( m_nElementCount >= m_nAllocCount )
		{
			// double size each realloc
			m_nAllocCount *= 2;
			m_pElements = (T*) REALLOCPASSFL( m_pPool, m_pElements, m_nAllocCount * sizeof(T));
			//trace( "### REALLOC on room array, new count = %d\n", m_nAllocCount );
			ASSERT_RETINVALID( m_pElements );
		}
		m_pElements[ m_nElementCount ] = tValue;
		m_nElementCount++;
		return m_nElementCount - 1;
	}
	T* Add( MEMORY_FUNCARGS( int* pnNewId ) )
	{
		ASSERT( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD );
		ASSERT( m_nAllocCount != 0 );
		if ( m_nElementCount >= m_nAllocCount )
		{
			// double size each realloc
			m_nAllocCount *= 2;
			m_pElements = (T*) REALLOCPASSFL( m_pPool, m_pElements, m_nAllocCount * sizeof(T));
			//trace( "### REALLOC on room array, new count = %d\n", m_nAllocCount );
			ASSERT_RETNULL( m_pElements );
		}
		T* pNewElement = &m_pElements[ m_nElementCount ];
		if ( pnNewId )
			*pnNewId = m_nElementCount;
		m_nElementCount++;
		return pNewElement;
	}
	BOOL RemoveByIndex( unsigned int nIndex )
	{
		ASSERT( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD );
		ASSERT_RETFALSE( nIndex < m_nElementCount );
		m_nElementCount--;
		//for ( int i = nIndex; (unsigned int)i < m_nElementCount; i++ )
		//{
		//	m_pElements[ i ] = m_pElements[ i + 1 ];
		//}
		ASSERT_RETFALSE(MemoryMove( &m_pElements[ nIndex ], (m_nAllocCount - nIndex) * sizeof(T), &m_pElements[ nIndex + 1 ], sizeof(T) * ( m_nElementCount - nIndex ) ));
		return TRUE;
	}
	BOOL RemoveByValue( MEMORY_FUNCARGS(T & tValue) )
	{
		ASSERT( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD );
		int i = GetIndex( tValue );
		if ( i == INVALID_ID )
			return FALSE;
		return RemoveByIndex( i );
	}
	void Clear( MEMORY_FUNCARGS(unsigned int nToCount) )
	{
		ASSERT( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD );
		m_nElementCount = nToCount;
	}
	int GetIndex( T & tValue )
	{
		ASSERT( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD );
		int i = 0;                  
		for ( ; (unsigned int)i < m_nElementCount; i++ )
		{
			if ( m_pElements[ i ] == tValue )
				break;
		}
		if ( (unsigned int)i >= m_nElementCount )
		{
			return INVALID_ID;
		}
		return i;
	}
	void QSort( int (__cdecl *pfnCompare)( const void *, const void *) )
	{
		ASSERT( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD );
		qsort( m_pElements, m_nElementCount, sizeof(T), pfnCompare );
	}
	void QSortEx( int start, int count, int (__cdecl *pfnCompare)( const void *, const void *) )
	{
		ASSERT( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD );
		qsort( GetPointer( start ), count, sizeof(T), pfnCompare );
	}
	// yeah, a little dangerous, but much faster to use
	T * GetPointer(int nIndex)
	{
		ASSERT( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD );
		ASSERT( nIndex >= 0 && (unsigned int)nIndex < m_nElementCount );
		ASSERT( m_pElements );
		return & m_pElements[ nIndex ];
	}
	const T& operator[](int nIndex) const
	{
		ASSERT( nIndex >= 0 && (unsigned int)nIndex < m_nElementCount );
		ASSERT( m_pElements );
		if ( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD )
			return m_pElements[ nIndex ];
		else
			return m_pElements[ ( m_nElementCount - 1 ) - nIndex ];
	}
	T& operator[](int nIndex)
	{
		ASSERT( nIndex >= 0 && (unsigned int)nIndex < m_nElementCount );
		ASSERT( m_pElements );
		if ( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD )
			return m_pElements[ nIndex ];
		else
#pragma warning(suppress : 6011) // null pointer m_pElements. 
			return m_pElements[ ( m_nElementCount - 1 ) - nIndex ];
	}
	const T& operator[](unsigned int nIndex) const
	{
		ASSERT( nIndex < m_nElementCount );
		ASSERT( m_pElements );
		if ( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD )
			return m_pElements[ nIndex ];
		else
#pragma warning(suppress : 6011) // null pointer m_pElements. 
			return m_pElements[ ( m_nElementCount - 1 ) - nIndex ];
	}
	T& operator[](unsigned int nIndex)
	{
		ASSERT( nIndex < m_nElementCount );
		ASSERT( m_pElements );
		if ( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD )
			return m_pElements[ nIndex ];
		else
			return m_pElements[ ( m_nElementCount - 1 ) - nIndex ];
	}
	operator const T*() const
	{
		ASSERT( m_nIndexingMethod == ARRAY_INDEXING_METHOD_FORWARD );
		return m_pElements;
	}
	unsigned int Count() const
	{
		return m_nElementCount;
	}
	void Destroy()
	{
		if ( m_pElements )
		{
			FREE( m_pPool, m_pElements );
			m_pElements = NULL;
		}
		m_nAllocCount   = 0;
		m_nElementCount = 0;
	}
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class T, unsigned int SIZE>
class SIMPLE_ARRAY
{
public:
	T *				list;
	unsigned int	count;

	// initialize
	void Init(
		void)
	{
		list = NULL;
		count = 0;
	};

	// free
	void Destroy(
		MEMORYPOOL * pool)
	{
		FREE(pool, list);
		list = NULL;
		count = 0;
	};

	// alias for free
	void Clear(
		MEMORYPOOL * pool)
	{
		Destroy(pool);
	};

	// free with callback for each item (in case the item needs to be cleaned up)
	void Destroy(
		MEMORYPOOL * pool,
		void fnDelete(MEMORYPOOL * pool, T & a))
	{
		for (unsigned int ii = 0; ii < count; ii++)
		{
			fnDelete(pool, list[ii]);
		}
		Destroy(pool);
	}

	// free with callback for each item (in case the item needs to be cleaned up)
	void Destroy(
		MEMORYPOOL * pool,
		void fnDelete(MEMORYPOOL * pool, T & a, void * data),
		void * data)
	{
		for (unsigned int ii = 0; ii < count; ii++)
		{
			fnDelete(pool, list[ii], data);
		}
		Destroy(pool);
	}

	// insert an item at end
	void InsertEnd(MEMORY_FUNCARGS(
		MEMORYPOOL * pool, 
		const T & item))
	{
		unsigned int ii = count;

		// reallocate if we need the space (every time count would exceed SIZE)
		if (count % SIZE == 0)
		{
			list = (T *)REALLOCPASSFL(pool, list, (count + SIZE) * sizeof(T));
		}

		++count;
		list[ii] = item;
	};

	// insert an item at index (moves everyting index + 1)
	void InsertAtIndex(MEMORY_FUNCARGS(
		MEMORYPOOL * pool,
		unsigned int index,
		const T & item))
	{
		ASSERT_RETURN(index <= count);

		// reallocate if we need the space (every time count would exceed SIZE)
		if (count % SIZE == 0)
		{
			list = (T *)REALLOCPASSFL(pool, list, (count + SIZE) * sizeof(T));
		}
		if (index < count)
		{
			ASSERT_RETURN(MemoryMove(list + index + 1, (count - index) * sizeof(T), list + index, (count - index) * sizeof(T)));
		}
		list[index] = item;
		++count;
	}

	// insert an item at index (overwrites anything previously in that position)
	void OverwriteIndex(MEMORY_FUNCARGS(
		MEMORYPOOL * pool, 
		unsigned int index,
		const T & item))
	{
		if (index >= count)
		{
			unsigned int cursize = (count + SIZE - 1) / SIZE * SIZE;
			if (index >= cursize)
			{
				unsigned int newsize = (index + SIZE) / SIZE * SIZE;
				list = (T *)REALLOCZPASSFL(pool, list, newsize * sizeof(T));
			}
			count = index + 1;
		}

		list[index] = item;
	};

	// delete an item by index
	void Delete(MEMORY_FUNCARGS(
		MEMORYPOOL * pool,
		unsigned int index))
	{
		DBG_ASSERT_RETURN(index < count);
		
		if (index + 1 < count)
		{
			ASSERT_RETURN(MemoryMove(list + index, (count - index) * sizeof(T), list + index + 1, (count - index - 1) * sizeof(T)));
		}

		--count;

		// shrink if needed
		if (count % SIZE == 0)
		{
			if (count > 0)
			{
				list = (T *)REALLOCPASSFL(pool, list, count * sizeof(T));
			}
			else
			{
				FREE(pool, list);
				list = NULL;
			}
		}
	};

	// delete an item by index
	void Delete(MEMORY_FUNCARGS(
		MEMORYPOOL * pool,
		unsigned int index,
		void fnDelete(MEMORYPOOL * pool, T & a)))
	{
		DBG_ASSERT_RETURN(index < count);

		fnDelete(pool, list[index]);
		
		Delete(MEMORY_FUNC_PASSFL(pool, index));
	}
	
	// accessor for size
	unsigned int Count(
		void) const
	{
		return count;
	};

	// accessor for items array
	T * GetPointer(
		void) const
	{
		return list;
	}

	// [] accessor
	const T & operator[](
		unsigned int index) const
	{
		DBG_ASSERT(index < count);
		return list[index];
	}

	// [] accessor
	T & operator[](
		unsigned int index)
	{
		DBG_ASSERT(index < count);
		return list[index];
	}

	// [] accessor
	T & operator[](
		int index)
	{
		DBG_ASSERT(index >= 0 && (unsigned int)index < count);
		return list[index];
	}

	// [] accessor
	const T & operator[](
		int index) const
	{
		DBG_ASSERT(index >= 0 && (unsigned int)index < count);
		return list[index];
	}
};


//-----------------------------------------------------------------------------
// SORTED_ARRAY
// vector class, grows by SIZE interval
// operator< and operator> must be defined for T
//-----------------------------------------------------------------------------
template <class T, unsigned int SIZE>
class SORTED_ARRAY : public SIMPLE_ARRAY<T, SIZE>
{
public:
	// find the exact item, returns NULL if not found
	T * FindExact(
		const T & item) const
	{
		T * min = list;
		T * max = list + count;
		T * ii = min + (max - min) / 2;

		while (max > min)
		{
			if (*ii > item)
			{
				max = ii;
			}
			else if (*ii < item)
			{
				min = ii + 1;
			}
			else
			{
				return ii;
			}
			ii = min + (max - min) / 2;
		}
		return NULL;
	}

	// find the exact item, returns index, returns MAX_UINT if not found
	unsigned int FindExactIndex(
		const T & item) const
	{
		T * find = FindExact(item);
		if (!find)
		{
			return (unsigned int)-1;
		}
		return (unsigned int)(find - list);
	}

	// find an item, returns the next item if it isn't found, 
	// sets match to true if the returned index is an exact match
	unsigned int Find(
		const T & item, 
		BOOL & match) const
	{
		T * min = list;
		T * max = list + count;
		T * ii = min + (max - min) / 2;

		while (max > min)
		{
			if (*ii > item)
			{
				max = ii;
			}
			else if (*ii < item)
			{
				min = ii + 1;
			}
			else
			{
				match = TRUE;
				return (unsigned int)(max - ii);
			}
			ii = min + (max - min) / 2;
		}
		match = FALSE;
		return (unsigned int)(max - list);
	}

	// insert an item sorted, returns FALSE if the item already exists
	BOOL Insert(
		MEMORYPOOL * pool, 
		const T & item)
	{
		BOOL match;
		unsigned int ii = Find(item, match);
		if (match)
		{
			return FALSE;		// already exists in list (error)
		}

		// reallocate if we need the space (every time count would exceed SIZE)
		if (count % SIZE == 0)
		{
			list = (T *)REALLOC(pool, list, (count + SIZE) * sizeof(T));
		}

		// move stuff if we're not inserting at the end
		if (ii < count)
		{
			ASSERT_RETFALSE(MemoryMove(list + ii + 1, (count - ii) * sizeof(T), list + ii, (count - ii) * sizeof(T)));
		}

		++count;
		list[ii] = item;

		return TRUE;
	};

	// delete an item by index
	BOOL DeleteByIndex(MEMORY_FUNCARGS(
		MEMORYPOOL * pool,
		UINT32 index))
	{
		if (index >= count) {
			return FALSE;
		}
		SIMPLE_ARRAY<T,SIZE>::Delete(MEMORY_FUNC_PASSFL(pool, index));
		return TRUE;
	}

	// delete an item by finding it
	BOOL Delete(MEMORY_FUNCARGS(
		MEMORYPOOL * pool,
		const T & item))
	{
		unsigned int index = FindExactIndex(item);
		return DeleteByIndex(MEMORY_FUNC_PASSFL(pool, index));
	}
};


//-----------------------------------------------------------------------------
// SORTED_ARRAYEX
// vector class, grows by SIZE interval
// provide a compare function (useful if you have multiple keys for T)
//-----------------------------------------------------------------------------
template <class T, int COMPARE(const T & a, const T & b), unsigned int SIZE>
class SORTED_ARRAYEX : public SORTED_ARRAY<T, SIZE>
{
public:
	// find the exact item, returns NULL if not found
	T * FindExact(
		const T & item) const
	{
		T * min = list;
		T * max = list + count;
		T * ii = min + (max - min) / 2;

		while (max > min)
		{
			int comp = COMPARE(*ii, item);
			if (comp > 0)
			{
				max = ii;
			}
			else if (comp < 0)
			{
				min = ii + 1;
			}
			else
			{
				return ii;
			}
			ii = min + (max - min) / 2;
		}
		return NULL;
	}

	// find the exact item, returns index, returns MAX_UINT if not found
	unsigned int FindExactIndex(
		const T & item) const
	{
		T * find = FindExact(item);
		if (!find)
		{
			return (unsigned int)-1;
		}
		return (unsigned int)(find - list);
	}

	// find an item, returns the next item if it isn't found, 
	// sets match to true if the returned index is an exact match
	unsigned int Find(
		const T & item, 
		BOOL & match) const
	{
		T * min = list;
		T * max = list + count;
		T * ii = min + (max - min) / 2;

		while (max > min)
		{
			int comp = COMPARE(*ii, item);
			if (comp > 0)
			{
				max = ii;
			}
			else if (comp < 0)
			{
				min = ii + 1;
			}
			else
			{
				match = TRUE;
				return (unsigned int)(max - ii);
			}
			ii = min + (max - min) / 2;
		}
		match = FALSE;
		return (unsigned int)(max - list);
	}

	// insert an item sorted, returns FALSE if the item already exists
	BOOL Insert(
		MEMORYPOOL * pool, 
		const T & item)
	{
		BOOL match;
		unsigned int ii = Find(item, match);
		if (match)
		{
			return FALSE;		// already exists in list (error)
		}

		// reallocate if we need the space (every time count would exceed SIZE)
		if (count % SIZE == 0)
		{
			list = (T *)REALLOC(pool, list, (count + SIZE) * sizeof(T));
		}

		// move stuff if we're not inserting at the end
		if (ii < count)
		{
			ASSERT_RETFALSE(MemoryMove(list + ii + 1, (count - ii) * sizeof(T), list + ii, (count - ii) * sizeof(T)));
		}

		++count;
		list[ii] = item;

		return TRUE;
	};

	void InsertAllowDuplicate(
		MEMORYPOOL * pool, 
		const T & item)
	{
		BOOL match;
		unsigned int ii = Find(item, match);

		// reallocate if we need the space (every time count would exceed SIZE)
		if (count % SIZE == 0)
		{
			list = (T *)REALLOC(pool, list, (count + SIZE) * sizeof(T));
		}

		// move stuff if we're not inserting at the end
		if (ii < count)
		{
			ASSERT_RETURN(MemoryMove(list + ii + 1, (count - ii) * sizeof(T), list + ii, (count - ii) * sizeof(T)));
		}

		++count;
		list[ii] = item;
	};

	// delete an item by index
	void Delete(
		MEMORYPOOL * pool,
		unsigned int index)
	{
		DBG_ASSERT_RETURN(index < count);
		
		if (index + 1 < count)
		{
			ASSERT_RETURN(MemoryMove(list + index, (count - index) * sizeof(T), list + index + 1, (count - index - 1) * sizeof(T)));
		}

		--count;

		// shrink if needed
		if (count % SIZE == 0)
		{
			if (count > 0)
			{
				list = (T *)REALLOC(pool, list, count * sizeof(T));
			}
			else
			{
				FREE(pool, list);
				list = NULL;
			}
		}
	};

	// delete an item by finding it
	BOOL Delete(
		MEMORYPOOL * pool,
		const T & item)
	{
		unsigned int index = FindExactIndex(item);
		if (index >= count)
		{
			return FALSE;
		}
		Delete(pool, index);
		return TRUE;
	}
};


//-----------------------------------------------------------------------------
// something that exists in an LRU list should inherit LRUNode
//-----------------------------------------------------------------------------
class LRUNode
{
	friend class LRUList;

protected:
	DWORD		m_LRU_Tick;
	LRUNode *	m_LRU_Prev;
	LRUNode *	m_LRU_Next;
	DWORD		m_LRU_Cost;

public:
	LRUNode(
		void) : m_LRU_Tick(0), m_LRU_Prev(NULL), m_LRU_Next(NULL), m_LRU_Cost(0)
	{
	}

	DWORD GetTime() { return m_LRU_Tick; }
	LRUNode* GetPrev() { return m_LRU_Prev; }
	LRUNode* GetNext() { return m_LRU_Next; }
	BOOL IsTracked() { return m_LRU_Tick != 0 && m_LRU_Prev != NULL && m_LRU_Next != NULL && m_LRU_Cost != 0; }
};


//-----------------------------------------------------------------------------
// time-ordered list of items, w/ touch updating position in list, 
// call update once per tick so we don't keep getting game time
// needs some sort of metric to count per item, such that we can clean up
// items on add if needed
//-----------------------------------------------------------------------------
class LRUList
{
protected:
	LRUNode		m_LRU_HeadTail;		// empty node, used to store time & total cost

public:
	void		Init(void);
	void		Free(void) {};
	void		Add(LRUNode * item, DWORD cost);
	void		Remove(LRUNode * item);
	void		Touch(LRUNode * item);
	void		Update(DWORD tick);
	LRUNode *	GetHeadTail();
	LRUNode *	GetFirstNode();
	DWORD		GetTotal(void);
};


//----------------------------------------------------------------------------
// bucket array
//----------------------------------------------------------------------------
#define ArrayResize(array, newsize)										(array).Resize(MEMORY_FUNC_FILELINE(newsize))

template <typename T, unsigned int BUCKET_SIZE>
struct BUCKET_ARRAY
{
	T **				m_Buckets;
	unsigned int		m_BucketCount;
	MEMORYPOOL*			m_Allocator;
	unsigned int		m_Count;

	BUCKET_ARRAY()
	{
		Init(NULL);
	}

	~BUCKET_ARRAY()
	{
		Free();
	}

	void Init(MEMORYPOOL* allocator)
	{
		m_Buckets = NULL;
		m_BucketCount = 0;
		m_Allocator = allocator;
		m_Count = 0;
	}

	void Free()
	{
		for (unsigned int i = 0; i < m_BucketCount; ++i)
		{
			FREE(m_Allocator, m_Buckets[i]);
		}
		FREE(m_Allocator, m_Buckets);
		m_Buckets = NULL;
		m_BucketCount = 0;
		m_Allocator = NULL;
		m_Count = 0;
	}

	unsigned int GetCount() const
	{
		return m_Count;
	}

	unsigned int GetSize() const
	{
		return m_BucketCount * BUCKET_SIZE;
	}

	unsigned int PushBack(MEMORY_FUNCARGS(const T& item))
	{
		// Allocate another bucket if necessary
		//
		if(m_Count + 1 > BUCKET_SIZE * m_BucketCount)
		{
			Resize(MEMORY_FUNC_PASSFL((BUCKET_SIZE * m_BucketCount) + 1));
		}

		// Add the item to the array
		//
		(*this)[m_Count] = item;
		++m_Count;

		return m_Count - 1;
	}

	void Resize(MEMORY_FUNCARGS(unsigned int newsize))
	{
		unsigned int newBucketCount = (newsize + BUCKET_SIZE - 1) / BUCKET_SIZE;
		
		if (newBucketCount <= m_BucketCount)
		{
			return;
		}

		m_Buckets = (T **)REALLOCPASSFL(m_Allocator, m_Buckets, newBucketCount * sizeof(T *));

		while(m_BucketCount < newBucketCount)
		{
			m_Buckets[m_BucketCount] = (T *)MALLOCPASSFL(m_Allocator, BUCKET_SIZE * sizeof(T));
			++m_BucketCount;
		}
	}

	T & operator[](
		unsigned int ii)
	{
		DBG_ASSERT(ii < GetSize());
		return m_Buckets[ii / BUCKET_SIZE][ii % BUCKET_SIZE];
	}

	const T & operator[](
		unsigned int ii) const
	{
		DBG_ASSERT(ii < GetSize());
		return m_Buckets[ii / BUCKET_SIZE][ii % BUCKET_SIZE];
	}
};


// Simple resizable flat array -
// Meant to replace local stack arrays, you keep one around and call Resize() before you use it
// (where normally you would declare a local stack array of a fixed size).  Then you just use it
// as a normal flat 1D array.
// If you call resize with a byte value smaller than currently allocated and bigger than (MAX_MEMORY_BYTES
template <class T>
struct resizable_array
{
private:
	static const int MAX_MEMORY_BYTES = 16 * 1024;

	T* m_Buf;
	unsigned int m_nAllocElements;
	MEMORYPOOL * m_pPool;

public:
	operator T*() { return m_Buf; }

	PRESULT Resize( unsigned int nCount )
	{
		// if m_Buf is null, this function will allocate it

		if ( nCount == 0 )
		{
			Destroy();
			return S_OK;
		}

		unsigned int nNextPow2 = (unsigned int)NEXTPOWEROFTWO( (DWORD)nCount );

		if ( nNextPow2 == m_nAllocElements )
		{
			return S_FALSE;
		}

		size_t nCurSize  = m_nAllocElements * sizeof(T);
		size_t nNextSize = nNextPow2 * sizeof(T);
		if ( nNextSize < nCurSize && nNextSize < MAX_MEMORY_BYTES )
		{
			// it's smaller than we currently have, but less than the "shrink" size, so leave it be
			return S_FALSE;
		}

		m_nAllocElements = nNextPow2;
		m_Buf = REALLOC_TYPE( T, m_pPool, m_Buf, m_nAllocElements );

		ASSERT_DO( m_Buf || ( m_nAllocElements == 0 ) )
		{
			m_nAllocElements = 0;
			return E_OUTOFMEMORY;
		}

		return S_OK;
	}

	DWORD GetAllocElements()
	{
		return m_nAllocElements;
	}

	void Init( MEMORYPOOL * pPool = NULL )
	{
		m_Buf = NULL;
		m_nAllocElements = 0;
		m_pPool = pPool;
	}

	void Destroy()
	{
		if ( m_Buf )
		{
			FREE_DELETE( m_pPool, m_Buf, T );
			m_Buf = NULL;
		}
		m_nAllocElements = 0;
	}
};


//-----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//-----------------------------------------------------------------------------
void testArrayIndexed(
	void);


#endif // _ARRAY_H_
