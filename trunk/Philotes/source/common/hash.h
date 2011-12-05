//----------------------------------------------------------------------------
// hash.h
//
// (C)Copyright 2004, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _HASH_H_
#define _HASH_H_


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#define HashInit(hash, pool, size)				(hash).Init(pool, size, __FILE__, __LINE__)
#define HashFree(hash)							(hash).Free()
#define HashGet(hash, id)						(hash).GetItem(int(id))
#define HashPrealloc(hash, count)				(hash).Prealloc(count, __FILE__, __LINE__)
#define HashAdd(hash, id)						(hash).AddItem(id, __FILE__, __LINE__)
#define HashAddSorted(hash, id)					(hash).AddItemSorted(id, __FILE__, __LINE__)
#define HashMove(hash, item, other)				(hash).MoveItem(item, other)
#define HashGetCount(hash)						(hash).GetCount()
#define HashRemove(hash, id)					(hash).RemoveItem(int(id))
#define HashClear(hash)							(hash).Clear()
#define HashGetFirst(hash)						(hash).GetFirstItem()
#define HashGetNext(hash, item)					(hash).GetNextItem(item)

#define HashInPlaceAdd(hash, item, id)			(hash).AddItem(item, id, __FILE__, __LINE__)
#define HashInPlaceAddSorted(hash, item, id)	(hash).AddItemSorted(item, id, __FILE__, __LINE__)



//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct HashMetrics
{
	int					nGet;
	int					nGetFirst;
	int					nGetNext;
	int					nAdd;
	int					nAddSort;
	int					nMove;
	int					nRemove;
	int					nBins;
	int					nItems;
};


//----------------------------------------------------------------------------
// CLASS
//----------------------------------------------------------------------------
template <class T, int ALIGN>
class CHashAligned
{
private:
	struct MEMORYPOOL *	m_pPool;
	T **				m_pHash;
	T *					m_pGarbage;
	CHashAligned<T,ALIGN> *	m_pOverrideGarbage;
	unsigned int		m_nSize;
	DWORD				m_dwKeyMask;
	unsigned int		m_nFirstKey;
	unsigned int		m_nTotalCount;

#if ISVERSION(DEVELOPMENT)
	unsigned int *		m_pBinCounts;
	unsigned int		m_nMaxBinCount;
	HashMetrics			m_tMetrics;
#endif

	unsigned int GetKey(
		int id)
	{
		return ((unsigned int)id & m_dwKeyMask);
	}

public:
	CHashAligned() :
		m_nSize(0),
		m_pHash(NULL),
		m_pGarbage(NULL),
		m_nFirstKey(UINT_MAX),
		m_pOverrideGarbage(NULL)
	{
	};

#if ISVERSION(DEVELOPMENT)
	void ResetMetrics()
	{
		structclear( m_tMetrics );
	}

	HashMetrics GetMetrics()
	{
		HashMetrics tMetrics = m_tMetrics;
		tMetrics.nBins = m_nSize;
		tMetrics.nItems = GetCount();
		ResetMetrics();
		return tMetrics;
	}
#endif // ISVERSION(DEVELOPMENT)

	inline T* GetGarbage()
	{
		return m_pOverrideGarbage ? m_pOverrideGarbage->GetGarbage() : m_pGarbage;
	}

	inline void SetGarbage(T* node)
	{
		if(m_pOverrideGarbage)
		{
			m_pOverrideGarbage->SetGarbage(node);
		}
		else
		{
			m_pGarbage = node;
		}
	}

	inline void AddToGarbage(T* node)
	{
		if(node)
		{
			node->pNext = GetGarbage();
			SetGarbage(node);
		}
	}

	inline T* RemoveFromGarbage()
	{
		T* node = GetGarbage();
		SetGarbage(node ? node->pNext : NULL);
		return node;
	}

	// WARNING!
	// Sharing the garbage list makes your CHash<> NOT THREAD SAFE!
	// BE VERY VERY CAREFUL!
	// Also note that if the garbage owner is freed, you will not be able to
	// do anything with those (other than freeing them)
	void ShareGarbageList(CHashAligned<T, ALIGN> * pGarbageOwner)
	{
		m_pOverrideGarbage = pGarbageOwner;
	}

	void Clear()
	{
		// move all hash nodes to garbage
		if (!m_pHash)
		{
			return;
		}
		for (unsigned int ii = 0; ii < m_nSize; ii++)
		{
			T * node = m_pHash[ii];
			while (node)
			{
				T * next = node->pNext;
				AddToGarbage(node);
				node = next;
			}
			m_pHash[ii] = NULL;
#if ISVERSION(DEVELOPMENT)
			m_pBinCounts[ii] = 0;
#endif
		}
		m_nFirstKey = m_nSize;
		m_nTotalCount = 0;
	}

	void Init(
		struct MEMORYPOOL * pool,
		unsigned int hash_size,
		const char * file,
		unsigned int line) 
	{
		MEMORY_NDEBUG_REF(file);
		MEMORY_NDEBUG_REF(line);

		ASSERT_RETURN(!m_pHash);
		ASSERT_RETURN(hash_size > 0);
		unsigned int bits = HIGHBIT(hash_size);
		m_nSize = (1 << bits);
		m_nFirstKey = m_nSize;
		m_dwKeyMask = (DWORD)(m_nSize - 1);
		m_pPool = pool;
		m_pHash = (T **)MALLOCZFL(m_pPool, m_nSize * sizeof(T *), file, line);
		m_pGarbage = NULL;
		m_pOverrideGarbage = NULL;
		m_nTotalCount = 0;
#if ISVERSION(DEVELOPMENT)
		m_pBinCounts = (unsigned int *)MALLOCZFL(m_pPool, m_nSize * sizeof(unsigned int), file, line);
		m_nMaxBinCount = 0;
#endif
	}

	void Free(
		void) 
	{
		if (!m_pHash)
		{
			return;
		}
		for (unsigned int ii = 0; ii < m_nSize; ii++)
		{
			T * node = m_pHash[ii];
			while (node)
			{
				T * next = node->pNext;
				FREE(m_pPool, node);
				node = next;
			}
		}
		FREE(m_pPool, m_pHash);
		m_pHash = NULL;
		m_nFirstKey = m_nSize;

		T * node = m_pGarbage;
		BOUNDED_WHILE(node, INT_MAX)
		{
			T * next = node->pNext;
			FREE(m_pPool, node);
			node = next;
		}
		m_pGarbage = NULL;

		m_nTotalCount = 0;
#if ISVERSION(DEVELOPMENT)
		if (m_pBinCounts)
		{
			FREE(m_pPool, m_pBinCounts);
			m_pBinCounts = NULL;
		}
		m_nMaxBinCount = 0;
#endif
	}

	void Prealloc(
		unsigned int nCount,
		const char * file,
		unsigned int line)
	{
		MEMORY_DEBUG_REF(file);
		MEMORY_DEBUG_REF(line);

		// add nCount items to garbage list
		for (unsigned int ii = 0; ii < nCount; ii++)
		{
			// malloc a new node
			T * item;
#pragma warning(suppress:4127) //conditional expression is constant
			if ( ALIGN )
				item = (T *)ALIGNED_MALLOCZFL(m_pPool, sizeof(T), ALIGN, file, line);
			else
				item = (T *)MALLOCZFL(m_pPool, sizeof(T), file, line);
			AddToGarbage(item);
		}
	}

	T * GetItem(
		int id)
	{
		if (!m_pHash)
		{
			return NULL;
		}
		if (id == INVALID_ID)
		{
			return NULL;
		}

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nGet++;
#endif

		unsigned int key = GetKey(id);

		T * node = m_pHash[key];
		while (node)
		{
			if (node->nId == id)
			{
				return node;
			}
			node = node->pNext;
		}
		return NULL;
	}

	T * AddItem(
		int id,
		const char * file,
		unsigned int line)
	{
		MEMORY_NDEBUG_REF(file);
		MEMORY_NDEBUG_REF(line);

#if ISVERSION(DEVELOPMENT)
		ASSERT_RETNULL(GetItem(id) == NULL);
#endif
		ASSERT_RETNULL(m_pHash);

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nAdd++;
#endif

		unsigned int key = GetKey(id);
		T * item;

		// grab off the garbage list first
		if (GetGarbage())
		{
			item = RemoveFromGarbage();
			ZeroMemory(item, sizeof(T));
		}
		else
		{
#pragma warning(suppress:4127) //conditional expression is constant
			if ( ALIGN )
				item = (T *)ALIGNED_MALLOCZFL(m_pPool, sizeof(T), ALIGN, file, line);
			else
				item = (T *)MALLOCZFL(m_pPool, sizeof(T), file, line);
		}
		item->nId = id;
		item->pNext = m_pHash[key];
		m_pHash[key] = item;

		if (key < m_nFirstKey)
		{
			m_nFirstKey = key;
		}

		m_nTotalCount++;

#if ISVERSION(DEVELOPMENT)
		m_pBinCounts[key]++;
		if (m_pBinCounts[key] > m_nMaxBinCount)
		{
			m_nMaxBinCount = m_pBinCounts[key];
		}
#endif
		return item;
	}

	T * AddItemSorted(
		int id,
		const char * file,
		unsigned int line)
	{
#if ISVERSION(DEVELOPMENT)
		ASSERT_RETNULL(GetItem(id) == NULL);
#endif
		ASSERT_RETNULL(m_pHash);

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nAddSort++;
#endif
		MEMORY_DEBUG_REF(file);
		MEMORY_DEBUG_REF(line);

		unsigned int key = GetKey(id);
		T * item;

		// grab off the garbage list first
		if (GetGarbage())
		{
			item = RemoveFromGarbage();
			ZeroMemory(item, sizeof(T));
		}
		else
		{
			// malloc a new node
#pragma warning(suppress:4127) //conditional expression is constant
			if ( ALIGN )
				item = (T*)ALIGNED_MALLOCZFL(m_pPool, sizeof(T), ALIGN, file, line);
			else
				item = (T*)MALLOCZFL(m_pPool, sizeof(T), file, line);
		}
		item->nId = id;

		// insertion sort into hash bucket
		if (!m_pHash[key] || m_pHash[key]->nId >= id)
		{
			item->pNext = m_pHash[key];
			m_pHash[key] = item;
		} 
		else
		{
			T * head = m_pHash[key];
			while (head->pNext && head->pNext->nId < id)
			{
				head = head->pNext;
			}
			item->pNext = head->pNext;
			head->pNext = item;
		}

		if (key < m_nFirstKey)
		{
			m_nFirstKey = key;
		}

		m_nTotalCount++;
#if ISVERSION(DEVELOPMENT)
		m_pBinCounts[key]++;
		if (m_pBinCounts[key] > m_nMaxBinCount)
		{
			m_nMaxBinCount = m_pBinCounts[key];
		}
#endif
		return item;
	}

	T * MoveItem(
		T * item,
		CHashAligned<T,ALIGN>& hashfrom)
	{

#if ISVERSION(DEVELOPMENT)
		ASSERT_RETVAL(GetItem(item->nId) == NULL, item);
		ASSERT_RETVAL(hashfrom.GetItem(item->nId) != NULL, item);
#endif
		ASSERT_RETNULL(m_pHash);

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nMove++;
#endif

		ONCE
		{
			unsigned int key = hashfrom.GetKey(item->nId);
			T * prev = NULL;
			T * node = hashfrom.m_pHash[key];
			while (node && node->nId != item->nId)
			{
				prev = node;
				node = node->pNext;
			}
			ASSERT_BREAK(node);
			if (prev)
			{
				prev->pNext = node->pNext;
			}
			else
			{
				hashfrom.m_pHash[key] = node->pNext;
			}
			hashfrom.m_nTotalCount--;
#if ISVERSION(DEVELOPMENT)
			hashfrom.m_pBinCounts[key]--;
#endif
		}

		unsigned int key = GetKey(item->nId);
		item->pNext = m_pHash[key];
		m_pHash[key] = item;

		if (key < m_nFirstKey)
		{
			m_nFirstKey = key;
		}

		m_nTotalCount++;

#if ISVERSION(DEVELOPMENT)
		m_pBinCounts[key]++;
		if (m_pBinCounts[key] > m_nMaxBinCount)
		{
			m_nMaxBinCount = m_pBinCounts[key];
		}
#endif
		return item;
	}

	void RemoveItem(
		int id)
	{
		if (!m_pHash)
		{
			return;
		}

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nRemove++;
#endif

		unsigned int key = GetKey(id);

		T * prev = NULL;
		T * node = m_pHash[key];
		while (node && node->nId != id)
		{
			prev = node;
			node = node->pNext;
		}
		if (!node)
		{
			return;
		}
		if (prev)
		{
			prev->pNext = node->pNext;
		}
		else
		{
			m_pHash[key] = node->pNext;
		}

		//FREE(m_pPool, node);
		// move to garbage list
		AddToGarbage(node);

		m_nTotalCount--;
#if ISVERSION(DEVELOPMENT)
		m_pBinCounts[key]--;
#endif
	}

	T * GetFirstItem(
		void)
	{
		if (!m_pHash)
		{
			return NULL;
		}

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nGetFirst++;
#endif

		for (unsigned int ii = m_nFirstKey; ii < m_nSize; ii++)
		{
			if (m_pHash[ii])
			{
				m_nFirstKey = ii;
				return m_pHash[ii];
			}
		}
		m_nFirstKey = m_nSize;
		return NULL;
	}

	T * GetNextItem(
		T * node)
	{
#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nGetNext++;
#endif

		if (!node)
		{
			return GetFirstItem();
		}

		if (node->pNext)
		{
			return node->pNext;
		}

		unsigned int key = GetKey(node->nId);
		for (unsigned int ii = key + 1; ii < m_nSize; ii++)
		{
			if (m_pHash[ii])
			{
				return m_pHash[ii];
			}
		}
		return NULL;
	}

	int GetCount(
		void) const
	{
		return m_nTotalCount;
	}
};


template <class T>
class CHash : public CHashAligned<T,0> {};


//----------------------------------------------------------------------------
// CHashInPlace
//   Like CHash, except doesn't malloc/free nodes -- assumes a node is in a
//   memory block allocated and managed elsewhere
//----------------------------------------------------------------------------
template <class T>
class CHashInPlace
{
private:
	struct MEMORYPOOL *	m_pPool;
	T **				m_pHash;
	unsigned int		m_nSize;
	DWORD				m_dwKeyMask;
	unsigned int		m_nFirstKey;
	unsigned int		m_nTotalCount;

#if ISVERSION(DEVELOPMENT)
	unsigned int *		m_pBinCounts;
	unsigned int		m_nMaxBinCount;
	HashMetrics			m_tMetrics;
#endif

	unsigned int GetKey(
		int id)
	{
		return ((unsigned int)id & m_dwKeyMask);
	}

public:
	CHashInPlace() :
		m_nSize(0),
		m_pHash(NULL),
		m_nFirstKey(UINT_MAX),
	{
	};

#if ISVERSION(DEVELOPMENT)
	void ResetMetrics()
	{
		structclear( m_tMetrics );
	}

	HashMetrics GetMetrics()
	{
		HashMetrics tMetrics = m_tMetrics;
		tMetrics.nBins = m_nSize;
		tMetrics.nItems = GetCount();
		ResetMetrics();
		return tMetrics;
	}
#endif // ISVERSION(DEVELOPMENT)

	void Clear()
	{
		// removes all nodes from the list and resets their IDs and pNext ptrs
		// Careful!  This will not free node memory, so you must track and free them outside!
		if (!m_pHash)
		{
			return;
		}
		for (unsigned int ii = 0; ii < m_nSize; ii++)
		{
			T * node = m_pHash[ii];
			while (node)
			{
				T * next = node->pNext;
				node->nId = INVALID_ID;
				node->pNext = NULL;
				node = next;
			}
			m_pHash[ii] = NULL;
#if ISVERSION(DEVELOPMENT)
			m_pBinCounts[ii] = 0;
#endif
		}
		m_nFirstKey = m_nSize;
		m_nTotalCount = 0;
	}

	void Init(
		struct MEMORYPOOL* pool,
		unsigned int hash_size) 
	{
		ASSERT_RETURN(!m_pHash);
		ASSERT_RETURN(hash_size > 0);
		unsigned int bits = HIGHBIT(hash_size);
		m_nSize = (1 << bits);
		m_nFirstKey = m_nSize;
		m_dwKeyMask = (DWORD)(m_nSize - 1);
		m_pPool = pool;
		m_pHash = (T **)MALLOCZ(m_pPool, m_nSize * sizeof(T *));
		m_nTotalCount = 0;
#if ISVERSION(DEVELOPMENT)
		m_pBinCounts = (unsigned int *)MALLOCZ(m_pPool, m_nSize * sizeof(unsigned int));
		m_nMaxBinCount = 0;
#endif
	}

	void Free(
		void) 
	{
		if (!m_pHash)
		{
			return;
		}

		Clear();

		FREE(m_pPool, m_pHash);
		m_pHash = NULL;
		m_nFirstKey = m_nSize;

		m_nTotalCount = 0;
#if ISVERSION(DEVELOPMENT)
		if (m_pBinCounts)
		{
			FREE(m_pPool, m_pBinCounts);
			m_pBinCounts = NULL;
		}
		m_nMaxBinCount = 0;
#endif
	}

	T * GetItem(
		int id)
	{
		if (!m_pHash)
		{
			return NULL;
		}
		if (id == INVALID_ID)
		{
			return NULL;
		}

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nGet++;
#endif

		unsigned int key = GetKey(id);

		T * node = m_pHash[key];
		while (node)
		{
			if (node->nId == id)
			{
				return node;
			}
			node = node->pNext;
		}
		return NULL;
	}

	void AddItem(
		T * item,
		int id,
		const char * file,
		unsigned int line)
	{
#if ISVERSION(DEVELOPMENT)
		ASSERT_RETURN(GetItem(id) == NULL);
#endif
		ASSERT_RETURN(m_pHash);
		ASSERT_RETURN(item);
		ASSERT_RETURN(item->nId == INVALID_ID);
		ASSERT_RETURN(item->pNext == NULL);

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nAdd++;
#endif
		MEMORY_DEBUG_REF(file);
		MEMORY_DEBUG_REF(line);

		unsigned int key = GetKey(id);

		item->nId = id;
		item->pNext = m_pHash[key];
		m_pHash[key] = item;

		if (key < m_nFirstKey)
		{
			m_nFirstKey = key;
		}

		m_nTotalCount++;

#if ISVERSION(DEVELOPMENT)
		m_pBinCounts[key]++;
		if (m_pBinCounts[key] > m_nMaxBinCount)
		{
			m_nMaxBinCount = m_pBinCounts[key];
		}
#endif
	}

	void AddItemSorted(
		T * item,
		int id,
		const char * file,
		unsigned int line)
	{
#if ISVERSION(DEVELOPMENT)
		ASSERT_RETURN(GetItem(id) == NULL);
#endif
		MEMORY_DEBUG_REF(file);
		MEMORY_DEBUG_REF(line);

		ASSERT_RETURN(m_pHash);
		ASSERT_RETURN(item);
		ASSERT_RETURN(item->nId == INVALID_ID);
		ASSERT_RETURN(item->pNext == NULL);

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nAddSort++;
#endif

		unsigned int key = GetKey(id);

		item->nId = id;

		// insertion sort into hash bucket
		if (!m_pHash[key] || m_pHash[key]->nId >= id)
		{
			item->pNext = m_pHash[key];
			m_pHash[key] = item;
		} 
		else
		{
			T * head = m_pHash[key];
			while (head->pNext && head->pNext->nId < id)
			{
				head = head->pNext;
			}
			item->pNext = head->pNext;
			head->pNext = item;
		}

		if (key < m_nFirstKey)
		{
			m_nFirstKey = key;
		}

		m_nTotalCount++;
#if ISVERSION(DEVELOPMENT)
		m_pBinCounts[key]++;
		if (m_pBinCounts[key] > m_nMaxBinCount)
		{
			m_nMaxBinCount = m_pBinCounts[key];
		}
#endif
	}

	void MoveItem(
		T * item,
		CHashAligned<T,0>& hashfrom,
		const char * file,
		unsigned int line)
	{
#if ISVERSION(DEVELOPMENT)
		ASSERT_RETURN(GetItem(item->nId) == NULL);
		ASSERT_RETURN(hashfrom.GetItem(item->nId) != NULL);
#endif
		MEMORY_DEBUG_REF(file);
		MEMORY_DEBUG_REF(line);

		ASSERT_RETURN(m_pHash);

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nMove++;
#endif

		ONCE
		{
			unsigned int key = hashfrom.GetKey(item->nId);
			T * prev = NULL;
			T * node = hashfrom.m_pHash[key];
			while (node && node->nId != item->nId)
			{
				prev = node;
				node = node->pNext;
			}
			ASSERT_BREAK(node);
			if (prev)
			{
				prev->pNext = node->pNext;
			}
			else
			{
				hashfrom.m_pHash[key] = node->pNext;
			}
			hashfrom.m_nTotalCount--;
#if ISVERSION(DEVELOPMENT)
			hashfrom.m_pBinCounts[key]--;
#endif
		}

		unsigned int key = GetKey(item->nId);
		item->pNext = m_pHash[key];
		m_pHash[key] = item;

		if (key < m_nFirstKey)
		{
			m_nFirstKey = key;
		}

		m_nTotalCount++;

#if ISVERSION(DEVELOPMENT)
		m_pBinCounts[key]++;
		if (m_pBinCounts[key] > m_nMaxBinCount)
		{
			m_nMaxBinCount = m_pBinCounts[key];
		}
#endif
	}

	void RemoveItem(
		int id)
	{
		if (!m_pHash)
		{
			return;
		}

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nRemove++;
#endif

		unsigned int key = GetKey(id);

		T * prev = NULL;
		T * node = m_pHash[key];
		while (node && node->nId != id)
		{
			prev = node;
			node = node->pNext;
		}
		if (!node)
		{
			return;
		}
		if (prev)
		{
			prev->pNext = node->pNext;
		}
		else
		{
			m_pHash[key] = node->pNext;
		}

		node->nId = INVALID_ID;
		node->pNext = NULL;

		m_nTotalCount--;
#if ISVERSION(DEVELOPMENT)
		m_pBinCounts[key]--;
#endif
	}

	T * GetFirstItem(
		void)
	{
		if (!m_pHash)
		{
			return NULL;
		}

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nGetFirst++;
#endif

		for (unsigned int ii = m_nFirstKey; ii < m_nSize; ii++)
		{
			if (m_pHash[ii])
			{
				m_nFirstKey = ii;
				return m_pHash[ii];
			}
		}
		m_nFirstKey = m_nSize;
		return NULL;
	}

	T * GetNextItem(
		T * node)
	{
#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nGetNext++;
#endif

		if (!node)
		{
			return GetFirstItem();
		}

		if (node->pNext)
		{
			return node->pNext;
		}

		unsigned int key = GetKey(node->nId);
		for (unsigned int ii = key + 1; ii < m_nSize; ii++)
		{
			if (m_pHash[ii])
			{
				return m_pHash[ii];
			}
		}
		return NULL;
	}

	int GetCount(
		void)
	{
		return m_nTotalCount;
	}
};


//----------------------------------------------------------------------------
// HARR CONSTANTS & MACROS
//----------------------------------------------------------------------------
#define MAX_HARR_SIZE						(1 << 14)
#define MAX_HARR_ITER						((1 << 18) - 2)
#define HARR_INVALID_ITER					(MAX_HARR_ITER + 1)
#define HARR_MAKE_ID(idx, iter)				(((iter) << 18) + (idx & (MAX_HARR_SIZE - 1)))
#define HARR_GET_IDX(id)					((id) % MAX_HARR_SIZE)
#define HARR_GET_ITER(id)					((id) >> 18)


//----------------------------------------------------------------------------
// array stores items by id generated by self
// each T needs members:
//	unsigned int nId;
//  T * pPrev;
//	T * pNext;
//----------------------------------------------------------------------------
template <class T, unsigned int SIZE>
class CHarr
{
private:
	struct MEMORYPOOL *	m_pPool;
	T *					m_pItems[SIZE];
	T *					m_pHead;
	T *					m_pTail;
	T *					m_pGarbage;
	unsigned int		m_nCount;
	unsigned int		m_nIterator;
#if ISVERSION(DEVELOPMENT)
	HashMetrics			m_tMetrics;
#endif

	// get next iterator
	unsigned int GetNextIterator(
		void)
	{
		unsigned int iter = m_nIterator;
		++m_nIterator;
		if (m_nIterator > MAX_HARR_ITER)
		{
			m_nIterator = 0;
		}
		return iter;
	}

	// get an item from the garbage list, it still has the id
	T * GetItemFromGarbage(
		void)
	{
		if (!m_pGarbage)
		{
			return NULL;
		}
		T * item = m_pGarbage;
		m_pGarbage = item->pNext;
		return item;
	}

	// add an item to the garbage list, maintain its id
	void AddItemToGarbage(
		T * item)
	{
		ASSERT_RETURN(item);
		unsigned int idx = HARR_GET_IDX(item->nId);
		ASSERT_RETURN(idx < SIZE);
		memclear(item, sizeof(T));
		item->nId = HARR_MAKE_ID(idx, HARR_INVALID_ITER);
		item->m_pNext = m_pGarbage;
		m_pGarbage = item;
	}

public:
	CHarr() :
		m_pPool(NULL),
		m_nCount(0),
		m_nIterator(0),
		m_pHead(NULL),
		m_pTail(NULL),
		m_pGarbage(NULL)
	{
	};

#if ISVERSION(DEVELOPMENT)
	void ResetMetrics(
		void)
	{
		structclear(&m_tMetrics);
	}

	HashMetrics GetMetrics(
		void)
	{
		HashMetrics tMetrics = m_tMetrics;
		tMetrics.nBins = SIZE;
		tMetrics.nItems = GetCount();
		ResetMetrics();
		return tMetrics;
	}
#endif 

	// clear the array, but if T requires cleanup, should create templated clear
	void Clear(
		void)
	{
		// iterate through the list and remove all nodes
		T * next = m_pHead;
		while (T * cur = next)
		{
			next = cur->pNext;

			AddItemToGarbage(cur);
		}

		// clear array
		memclear(m_pItems, sizeof(T *) * SIZE);
		m_pHead = NULL;
		m_pTail = NULL;
		m_nCount = 0;
		m_nIterator = 0;
	}

	// initialize
	void Init(
		struct MEMORYPOOL * pool) 
	{
		m_pPool = pool;
		Clear();
	}

	// free all nodes (provide template version if T requires cleanup)
	void Free(
		void) 
	{
		Clear();

		T * next = m_pGarbage;
		while (T * cur = next)
		{
			next = cur->pNext;

			FREE(m_pPool, cur);
		}
		m_pGarbage = NULL;
	}

	// add count items to garbage list
	void Prealloc(
		int count,
		const char * file,
		unsigned int line)
	{
		MEMORY_DEBUG_REF(file);
		MEMORY_DEBUG_REF(line);

		ASSERT_RETURN(m_pPool);
		ASSERT_RETURN(count <= SIZE);
		ASSERT_RETURN(m_nCount == 0);
		for (unsigned int ii = 0; ii < count; ++ii)
		{
			// malloc a new node
			T * item = (T *)MALLOCZFL(m_pPool, sizeof(T), file, line);
			item->nId = HARR_MAKE_ID(count, HARR_INVALID_ITER);
			item->pNext = m_pGarbage;
			m_pGarbage = item;
		}
	}

	// get an item by id
	T * GetItem(
		int id)
	{
#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nGet++;
#endif

		unsigned int idx = HARR_GET_IDX(id);
		if (!m_pItems[idx])
		{
			return NULL;
		}
		if (HARR_GET_ITER(m_pItems[idx]->nId) != HARR_GET_ITER(id))
		{
			return NULL;
		}
		return m_pItems[idx];
	}

	// get a new node from the harr
	T * AddItem(
		const char * file,
		unsigned int line)
	{
		MEMORY_DEBUG_REF(file);
		MEMORY_DEBUG_REF(line);

		ASSERT_RETNULL(m_nCount < SIZE);

#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nAdd++;
#endif
		
		T * item = GetItemFromGarbage();
		if (!item)
		{
			item = (T *)MALLOC(m_pPool, sizeof(T));
			item->nId = HARR_MAKE_ID(m_nCount, 0);
		}

		// set the new id and clear it
		unsigned int oldidx = HARR_GET_IDX(item->nId);
		memclear(item, sizeof(T));
		item->nId = HARR_MAKE_ID(oldidx, GetNextIterator());

		// add the item to the tail
		if (m_pTail)
		{
			item->pPrev = m_pTail;
			m_pTail->pNext = item;
			m_pTail = item;
		}
		else
		{
			m_pHead = item;
			m_pTail = item;
		}

		m_nCount++;

		return item;
	}

	// remove item from harr
	BOOL RemoveItem(
		int id)
	{
#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nRemove++;
#endif

		unsigned int idx = HARR_GET_IDX(id);
		if (!m_pItems[idx])
		{
			return FALSE;
		}

		T * item = m_pItems[idx];
		if (HARR_GET_ITER(item->nId) != HARR_GET_ITER(id))
		{
			return FALSE;
		}

		// remove the item from the list
		if (item->pPrev)
		{
			item->pPrev->pNext = item->pNext;
		}
		else
		{
			m_pHead = item->pNext;
		}
		if (item->pNext)
		{
			item->pNext->pPrev = item->pPrev;
		}
		else
		{
			m_pTail = item->pPrev;
		}

		AddItemToGarbage(m_pItems[idx]);
		m_pItems[idx] = NULL;
		m_nCount--;
		return TRUE;
	}

	// get the first item in the harr (in no particular order)
	T * GetFirstItem(
		void)
	{
#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nGetFirst++;
#endif
		return m_pHead;
	}

	// get the next item in the harr (in no particular order)
	T * GetNextItem(
		T * item)
	{
#if ISVERSION(DEVELOPMENT)
		m_tMetrics.nGetNext++;
#endif

		if (!item)
		{
			return GetFirstItem();
		}
		return item->pNext;
	}

	// get number of active items in harr
	int GetCount(
		void)
	{
		return m_nCount;
	}
};


//-----------------------------------------------------------------------------
// HASH
// ID is the id on which the hash occurs
// assumes that operator%, operator==, operator= are all defined for ID
// T is a node, needs to have members [ID id] and [T * next]
// note this doesn't prevent you from adding the same id more than once
//-----------------------------------------------------------------------------
template <class T, class ID, unsigned int SIZE>
class HASH
{
protected:
	T *			hash[SIZE];
	T *			garbage;

	T *	GetFromGarbage(
		void)
	{
		if (!garbage)
		{
			return NULL;
		}
		T * node = garbage;
		garbage = garbage->next;
		memclear(node, sizeof(T));
		return node;
	}

	T * GetNewNode(
		MEMORYPOOL * pool)
	{
		T * node = GetFromGarbage();
		if (!node)
		{
			node = (T *)MALLOCZ(pool, sizeof(T));
		}
		return node; 
	}

	void Recycle(
		T * item)
	{
		item->next = garbage;
		garbage = item;
	}

public:
	void Init(
		void)
	{
		memclear(hash, sizeof(hash));
		garbage = NULL;
	}

	void Destroy(
		MEMORYPOOL * pool)
	{
		for (unsigned int ii = 0; ii < SIZE; ++ii)
		{
			T * item = NULL;
			T * next = hash[ii];
			while ((item = next) != NULL)
			{
				next = item->next;
				FREE(pool, item);
			}
		}
		memclear(hash, sizeof(hash));

		T * item = NULL;
		T * next = garbage;
		while ((item = next) != NULL)
		{
			next = item->next;
			FREE(pool, item);
		}
		garbage = NULL;
	}

	void Destroy(
		MEMORYPOOL * pool,
		void fnDelete(MEMORYPOOL * pool, T * a))
	{
		for (unsigned int ii = 0; ii < SIZE; ++ii)
		{
			T * item = NULL;
			T * next = hash[ii];
			while ((item = next) != NULL)
			{
				next = item->next;
				fnDelete(pool, item);
				FREE(pool, item);
			}
		}
		memclear(hash, sizeof(hash));

		T * item = NULL;
		T * next = garbage;
		while ((item = next) != NULL)
		{
			next = item->next;
			FREE(pool, item);
		}
		garbage = NULL;
	}

	T * Add(
		MEMORYPOOL * pool,
		const ID & id)
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);

		T * item = GetNewNode(pool);
		ASSERT_RETNULL(item);
		item->id = id;
		item->next = hash[hashindex];
		hash[hashindex] = item;
		return item;
	}

	T * Get(
		const ID & id)
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);
		T * item = NULL;
		T * next = hash[hashindex];
		while ((item = next) != NULL)
		{
			next = item->next;
			if (item->id == id)
			{
				return item;
			}
		}
		return NULL;
	}

	BOOL Remove(
		const ID & id)
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);
		T * prev = NULL;
		T * item = hash[hashindex];
		while (item)
		{
			if (item->id == id)
			{
				if (prev)
				{
					prev->next = item->next;
				}
				else
				{
					hash[hashindex] = item->next;
				}
				Recycle(item);
				return TRUE;
			}
			prev = item;
			item = item->next;
		}
		return FALSE;
	}

	BOOL Remove(
		MEMORYPOOL * pool,
		const ID & id,
		void fnDelete(MEMORYPOOL * pool, T & a))
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);
		T * prev = NULL;
		T * item = hash[hashindex];
		while (item)
		{
			if (item->id == id)
			{
				if (prev)
				{
					prev->next = item->next;
				}
				else
				{
					hash[hashindex] = item->next;
				}
				fnDelete(pool, item);
				Recycle(item);
				return TRUE;
			}
			prev = item;
			item = item->next;
		}
		return FALSE;
	}
};


//-----------------------------------------------------------------------------
// HASHEX
// extension of HASH allows for extended ID types for which we define:
// GETKEY should return an evenly distributed unsigned int from 0 to UINT_MAX
// the class will automatically normalize it to the hash size
// and COMPARE
// note this doesn't prevent you from adding the same id more than once
//-----------------------------------------------------------------------------
template <class T, class ID, unsigned int SIZE, unsigned int GETKEY(const ID & id), int COMPARE(const ID & a, const ID & b)>
class HASHEX : public HASH<T, ID, SIZE>
{
public:
	T * Add(
		MEMORYPOOL * pool,
		const ID & id)
	{
		unsigned int hashindex = GETKEY(id) % SIZE;

		T * item = GetNewNode(pool);
		ASSERT(item);
		item->id = id;
		item->next = hash[hashindex];
		hash[hashindex] = item;
		return item;
	}

	T * Get(
		const ID & id)
	{
		unsigned int hashindex = GETKEY(id) % SIZE;
		T * item = NULL;
		T * next = hash[hashindex];
		while (item = next)
		{
			next = item->next;
			if (COMPARE(item->id, id) == 0)
			{
				return item;
			}
		}
		return NULL;
	}

	BOOL Remove(
		const ID & id)
	{
		unsigned int hashindex = GETKEY(id) % SIZE;
		T * prev = NULL;
		T * item = hash[hashindex];
		while (item)
		{
			if (COMPARE(item->id, id) == 0)
			{
				if (prev)
				{
					prev->next = item->next;
				}
				else
				{
					hash[hashindex] = item->next;
				}
				Recycle(item);
				return TRUE;
			}
			prev = item;
			item = item->next;
		}
		return FALSE;
	}

	BOOL Remove(
		MEMORYPOOL * pool,
		const ID & id,
		void fnDelete(MEMORYPOOL * pool, T & a))
	{
		unsigned int hashindex = GETKEY(id) % SIZE;
		T * prev = NULL;
		T * item = hash[hashindex];
		while (item)
		{
			if (COMPARE(item->id, id) == 0)
			{
				if (prev)
				{
					prev->next = item->next;
				}
				else
				{
					hash[hashindex] = item->next;
				}
				fnDelete(pool, item);
				Recycle(item);
				return TRUE;
			}
			prev = item;
			item = item->next;
		}
		return FALSE;
	}
};


//-----------------------------------------------------------------------------
// same as HASH, but provides functions for iterating all nodes
// WARNING: make sure you know what you're doing if you insert/remove nodes
// while you're iterating
//-----------------------------------------------------------------------------
template <class T, class ID, unsigned int SIZE>
class HASHITER : public HASH<T, ID, SIZE>
{
protected:
	unsigned int first;

public:
	void Init(
		void)
	{
		memclear(hash, sizeof(hash));
		garbage = NULL;
		first = SIZE;
	}

	T * Add(
		MEMORYPOOL * pool,
		const ID & id)
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);

		T * item = GetNewNode(pool);
		ASSERT_RETNULL(item);
		item->id = id;
		item->next = hash[hashindex];
		hash[hashindex] = item;

		first = MIN(hashindex, first);

		return item;
	}

	T * GetFirst(
		void)
	{
		for (unsigned int ii = first; ii < SIZE; ++ii)
		{
			if (hash[ii])
			{
				first = ii;
				return hash[ii];
			}
		}
		first = SIZE;
		return NULL;
	}

	T * GetNext(
		T * prev)
	{
		if (!prev)
		{
			return GetFirst();
		}
		if (prev->next)
		{
			return prev->next;
		}

		unsigned int hashindex = (unsigned int)(prev->id % SIZE);
		for (unsigned int ii = hashindex + 1; ii < SIZE; ++ii)
		{
			if (hash[ii])
			{
				return hash[ii];
			}
		}
		return NULL;
	}
};


//-----------------------------------------------------------------------------
// same as HASHEX, but provides functions for iterating all nodes
// WARNING: make sure you know what you're doing if you insert/remove nodes
// while you're iterating
//-----------------------------------------------------------------------------
template <class T, class ID, unsigned int SIZE, unsigned int GETKEY(const ID & id), int COMPARE(const ID & a, const ID & b)>
class HASHEXITER : public HASHEX<T, ID, SIZE, GETKEY, COMPARE>
{
protected:
	unsigned int first;

public:
	T * Add(
		MEMORYPOOL * pool,
		const ID & id)
	{
		unsigned int hashindex = GETKEY(id) % SIZE;

		T * item = GetNewNode(pool);
		ASSERT(item);
		item->id = id;
		item->next = hash[hashindex];
		hash[hashindex] = item;

		first = MIN(hashindex, first);

		return item;
	}

	T * GetFirst(
		void)
	{
		for (unsigned int ii = first; ii < SIZE; ++ii)
		{
			if (hash[ii])
			{
				first = ii;
				return hash[ii];
			}
		}
		first = SIZE;
		return NULL;
	}

	T * GetNext(
		T * prev)
	{
		if (!prev)
		{
			return GetFirst();
		}
		if (prev->next)
		{
			return prev->next;
		}

		unsigned int hashindex = (unsigned int)(prev->id % SIZE);
		for (unsigned int ii = hashindex + 1; ii < SIZE; ++ii)
		{
			if (hash[ii])
			{
				return hash[ii];
			}
		}
		return NULL;
	}
};


//-----------------------------------------------------------------------------
// CS_HASH
// mt-safe extension of HASH which has per-key critical section locking
//-----------------------------------------------------------------------------
template <class T, class ID, unsigned int SIZE>
class CS_HASH
{
protected:
	T *				hash[SIZE];
	T *				garbage;
	CCritSectLite	cs[SIZE];
	CCritSectLite	cs_garbage;

	T *	GetFromGarbage(
		void)
	{
		T * node = NULL;
		cs_garbage.Enter();
		ONCE
		{
			if (!garbage)
			{
				break;
			}
			node = garbage;
			garbage = garbage->next;
		}
		cs_garbage.Leave();

		if (node)
		{
			memclear(node, sizeof(T));
		}
		return node;
	}

	T * GetNewNode(
		MEMORYPOOL * pool)
	{
		T * node = GetFromGarbage();
		if (!node)
		{
			node = (T *)MALLOCZ(pool, sizeof(T));
		}
		return node; 
	}

	void Recycle(
		T * item)
	{
		cs_garbage.Enter();
		item->next = garbage;
		garbage = item;
		cs_garbage.Leave();
	}

	void DestroyCS(
		void)
	{
		for (unsigned int ii = 0; ii < SIZE; ++ii)
		{
			cs[ii].Free();
		}
		cs_garbage.Free();
	}

public:
	void Init(
		void)
	{
		memclear(hash, sizeof(hash));
		garbage = NULL;
		for (unsigned int ii = 0; ii < SIZE; ++ii)
		{
			cs[ii].Init();
		}
		cs_garbage.Init();
	}

	void Destroy(
		MEMORYPOOL * pool)
	{
		for (unsigned int ii = 0; ii < SIZE; ++ii)
		{
			T * item = NULL;
			T * next = hash[ii];
			item = next;
			while (item)
			{
				next = item->next;
				FREE(pool, item);
				item = next;
			}
		}
		memclear(hash, sizeof(hash));

		T * item = NULL;
		T * next = garbage;
		item = next;
		while (item)
		{
			next = item->next;
			FREE(pool, item);
			item = next;
		}
		garbage = NULL;

		DestroyCS();
	}

	void Destroy(
		MEMORYPOOL * pool,
		void fnDelete(MEMORYPOOL * pool, T * a))
	{
		for (unsigned int ii = 0; ii < SIZE; ++ii)
		{
			T * item = NULL;
			T * next = hash[ii];
			while (item = next)
			{
				next = item->next;
				fnDelete(pool, item);
				FREE(pool, item);
			}
		}
		memclear(hash, sizeof(hash));

		T * item = NULL;
		T * next = garbage;
		while (item = next)
		{
			next = item->next;
			FREE(pool, item);
		}
		garbage = NULL;

		DestroyCS();
	}

	T * Add(
		MEMORYPOOL * pool,
		const ID & id)
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);

		T * item = GetNewNode(pool);
		ASSERT(item);
		item->id = id;

		cs[hashindex].Enter();
		item->next = hash[hashindex];
		hash[hashindex] = item;
		cs[hashindex].Leave();

		return item;
	}

	T * Get(
		const ID & id)
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);
		T * item = NULL;

		cs[hashindex].Enter();

		ONCE
		{
			T * next = hash[hashindex];
			item = next;
			while (item)
			{
				next = item->next;
				if (item->id == id)
				{
					break;
				}
				item = next;
			}
		}

		cs[hashindex].Leave();

		return item;
	}

	BOOL Remove(
		const ID & id)
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);
		T * prev = NULL;

		BOOL result = FALSE;

		cs[hashindex].Enter();
		ONCE
		{
			T * item = hash[hashindex];
			while (item)
			{
				if (item->id == id)
				{
					if (prev)
					{
						prev->next = item->next;
					}
					else
					{
						hash[hashindex] = item->next;
					}
					Recycle(item);
					result = TRUE;
					break;
				}
				prev = item;
				item = item->next;
			}
		}
		cs[hashindex].Leave();

		return result;
	}

	BOOL Remove(
		MEMORYPOOL * pool,
		const ID & id,
		void fnDelete(MEMORYPOOL * pool, T & a))
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);
		T * prev = NULL;

		BOOL result = FALSE;

		cs[hashindex].Enter();
		ONCE
		{
			T * item = hash[hashindex];
			while (item)
			{
				if (item->id == id)
				{
					if (prev)
					{
						prev->next = item->next;
					}
					else
					{
						hash[hashindex] = item->next;
					}
					fnDelete(pool, item);
					Recycle(item);
					result = TRUE;
					break;
				}
				prev = item;
				item = item->next;
			}
		}
		cs[hashindex].Leave();

		return result;
	}
};

//-----------------------------------------------------------------------------
// HASH_NOALLOC_NOREMOVE
// A hash designed to be declared on the stack.  Requires no freeing beyond
// stack destruction.  Any added nodes are gotten from a fixed array; no
// allocation.  No removing nodes once added, though we can organize removed
// nodes into a freelist if this is desired.  Intentionally not including
// that garbage handling code to get a miniscule performance improvement
// and major code complexity reduction.
//-----------------------------------------------------------------------------
template <class T, class ID, unsigned int SIZE, unsigned int MAX_ELEMENTS>
class HASH_NOALLOC_NOREMOVE
{
protected:
	T *				hash[SIZE];
	T				elements[MAX_ELEMENTS];
	unsigned int	nElements;

	T * GetNewNode()
	{
		if(nElements >= MAX_ELEMENTS) return NULL;
		nElements++;
		return &elements[nElements - 1]; 
	}

public:
	void Init(
		void)
	{
		memclear(this, sizeof(*this) );
		nElements = 0;
	}

	T * Add(const ID & id)
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);

		T * item = GetNewNode();
		ASSERT_RETNULL(item);
		item->id = id;
		item->next = hash[hashindex];
		hash[hashindex] = item;
		return item;
	}

	T * Get(
		const ID & id)
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);
		T * item = NULL;
		T * next = hash[hashindex];
		while ((item = next) != NULL)
		{
			next = item->next;
			if (item->id == id)
			{
				return item;
			}
		}
		return NULL;
	}
};

//-----------------------------------------------------------------------------
// CS_HASH_LOCKING
// mt-safe extension of HASH which delivers entries locked, requiring manual
// unlocking once we're done.  Autocreates entries that are not found.
// Unlocks entry on a delete (assumes you have the lock.)
//-----------------------------------------------------------------------------
template <class T, class ID, unsigned int SIZE>
class CS_HASH_LOCKING
{
protected:
	MEMORYPOOL *		pool;
	T *					hash[SIZE];
	T *					garbage;
	CCriticalSection	cs[SIZE];
	CCriticalSection	cs_garbage;

	T *	GetFromGarbage(
		void)
	{
		T * node = NULL;
		cs_garbage.Enter();
		ONCE
		{
			if (!garbage)
			{
				break;
			}
			node = garbage;
			garbage = garbage->next;
		}
		cs_garbage.Leave();

		if (node)
		{
			memclear(node, sizeof(T));
		}
		return node;
	}

	T * GetNewNode()
	{
		T * node = GetFromGarbage();
		if (!node)
		{
			node = (T *)MALLOCZ(pool, sizeof(T));
		}
		return node; 
	}

	void Recycle(
		T * item)
	{
		cs_garbage.Enter();
		item->next = garbage;
		garbage = item;
		cs_garbage.Leave();
	}

	void DestroyCS(
		void)
	{
		for (unsigned int ii = 0; ii < SIZE; ++ii)
		{
			cs[ii].Free();
		}
		cs_garbage.Free();
	}

public:
	void Init(
		MEMORYPOOL *pPool)
	{
		memclear(hash, sizeof(hash));
		garbage = NULL;
		for (unsigned int ii = 0; ii < SIZE; ++ii)
		{
			cs[ii].Init();
		}
		cs_garbage.Init();
		pool = pPool;
	}

	void Destroy()
	{
		for (unsigned int ii = 0; ii < SIZE; ++ii)
		{
			T * item;
			T * next = item = hash[ii];
			while (item)
			{
				next = item->next;
				FREE(pool, item);
				item = next;
			}
		}
		memclear(hash, sizeof(hash));

		T * item;
		T * next = item = garbage;
		while (item)
		{
			next = item->next;
			FREE(pool, item);
			item = next;
		}
		garbage = NULL;

		DestroyCS();
	}

	void Destroy(
		void fnDelete(MEMORYPOOL * pool, T * a))
	{
		for (unsigned int ii = 0; ii < SIZE; ++ii)
		{
			T * item;
			T * next = item = hash[ii];
			while (item)
			{
				next = item->next;
				fnDelete(pool, item);
				FREE(pool, item);
				item = next;
			}
		}
		memclear(hash, sizeof(hash));

		T * item;
		T * next = garbage;
		item = garbage;
		while (item)
		{
			next = item->next;
			FREE(pool, item);
			item = next;
		}
		garbage = NULL;

		DestroyCS();
	}

	T * Add(
		const ID & id)
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);

		T * item = GetNewNode();
		ASSERT(item);
		item->id = id;

		cs[hashindex].Enter();
		item->next = hash[hashindex];
		hash[hashindex] = item;

		return item;
	}

	T * Get(
		const ID & id)
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);
		T * item = NULL;

		cs[hashindex].Enter();
		ONCE
		{
			T * next = hash[hashindex];
			item = next;
			while (item)
			{
				next = item->next;
				if (item->id == id)
				{
					break;
				}
				item = next;
			}
		}

		return item;
	}

	T * GetOrAdd(
		const ID & id)
	{//If the item isn't found, we create it.
		unsigned int hashindex = (unsigned int)(id % SIZE);
		T * item = NULL;

		cs[hashindex].Enter();
		ONCE
		{
			T * next = hash[hashindex];
			item = next;
			while (item)
			{
				next = item->next;
				if (item->id == id)
				{
					break;
				}
				item = next;
			}
		}
		if(!item)
		{
			item = GetNewNode();
			ASSERT_RETNULL(item);
			item->id = id;

			item->next = hash[hashindex];
			hash[hashindex] = item;
		}

		return item;
	}

	void Unlock(
		const ID & id)
	{
		unsigned int hashindex = (unsigned int)(id % SIZE);
		cs[hashindex].Leave();
	}

	BOOL Remove(
		const ID & id)
	{	//Assumes we already have the lock.
		unsigned int hashindex = (unsigned int)(id % SIZE);
		T * prev = NULL;

		BOOL result = FALSE;

		ONCE
		{
			T * item = hash[hashindex];
			while (item)
			{
				if (item->id == id)
				{
					if (prev)
					{
						prev->next = item->next;
					}
					else
					{
						hash[hashindex] = item->next;
					}
					Recycle(item);
					result = TRUE;
					break;
				}
				prev = item;
				item = item->next;
			}
		}
		cs[hashindex].Leave();

		return result;
	}

	BOOL Remove(
		const ID & id,
		void fnDelete(MEMORYPOOL * pool, T & a))
	{	//Assumes we already have the lock.
		unsigned int hashindex = (unsigned int)(id % SIZE);
		T * prev = NULL;

		BOOL result = FALSE;

		ONCE
		{
			T * item = hash[hashindex];
			while (item)
			{
				if (item->id == id)
				{
					if (prev)
					{
						prev->next = item->next;
					}
					else
					{
						hash[hashindex] = item->next;
					}
					fnDelete(pool, item);
					Recycle(item);
					result = TRUE;
					break;
				}
				prev = item;
				item = item->next;
			}
		}
		cs[hashindex].Leave();

		return result;
	}
};

#endif // _HASH_H_