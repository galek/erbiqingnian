//----------------------------------------------------------------------------
// inorderhash.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _INORDERHASH_H_
#define _INORDERHASH_H_


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#define InOrderHashInit(hash, pool, size)			(hash).Init(pool, size)
#define InOrderHashFree(hash)						(hash).Free()
#define InOrderHashGet(hash, id)					(hash).GetItem(id)
#define InOrderHashAdd(hash, id)					(hash).AddItem(id,__FILE__,__LINE__)
#define InOrderHashRemove(hash, id)					(hash).RemoveItem(id)
#define InOrderHashGetNext(hash, item, id)			(hash).GetNextItem(item, id)
#ifdef _DEBUG
#define InOrderHashGetCount(hash)					(hash).GetCount()
#else
#define InOrderHashGetCount(hash)					-1
#endif
#define InOrderHashGetFirst(hash)					(hash).GetFirstItem()
#define InOrderHashGetNextInHash(hash, item)		(hash).GetNextItemInHash(item)


//----------------------------------------------------------------------------
// CLASS
//----------------------------------------------------------------------------
template <class T>
class CInOrderHash
{
private:
	struct MEMORYPOOL *	m_pPool;
	unsigned int		m_nSize;
	DWORD				m_dwKeyMask;
	T **				m_pHash;
#ifdef _DEBUG
	unsigned int		m_nTotalCount;
	unsigned int *		m_pBinCounts;
	unsigned int		m_nMaxBinCount;
#endif

	unsigned int GetKey(
		int id)
	{
		return ((unsigned int)id & m_dwKeyMask);
	}

public:
	CInOrderHash() :
		m_nSize(0),
		m_pHash(NULL)
	{
	};
	
	void Init(
		struct MEMORYPOOL * pool,
		int hash_size) 
	{
		ASSERT_RETURN(!m_pHash);
		ASSERT_RETURN(hash_size > 0);
		unsigned int bits = HIGHBIT(hash_size);
		m_nSize = (1 << bits);
		m_dwKeyMask = m_nSize - 1;
		m_pPool = pool;
		m_pHash = (T **)MALLOCZ(m_pPool, m_nSize * sizeof(T *));
#ifdef _DEBUG
		m_nTotalCount = 0;
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

#ifdef _DEBUG
		m_nTotalCount = 0;
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
		ASSERT_RETNULL(m_pHash);
		MEMORY_NDEBUG_REF(file);
		MEMORY_NDEBUG_REF(line);

		unsigned int key = GetKey(id);
		T * curr = m_pHash[key];
		T * last = NULL;
		while(curr && curr->nId <= id)
		{
			last = curr;
			curr = curr->pNext;
		}

		T * item = (T *)MALLOCZFL(m_pPool, sizeof(T), file, line);
		item->nId = id;
		item->pNext = last ? last->pNext : m_pHash[key];
		if (last)
		{
			last->pNext = item;
		}
		else
		{
			m_pHash[key] = item;
		}

#ifdef _DEBUG
		m_nTotalCount++;
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
		ASSERT_RETURN(m_pHash);
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
		FREE(m_pPool, node);

#ifdef _DEBUG
		m_nTotalCount--;
		m_pBinCounts[key]--;
#endif

		RemoveItem(id);
	}

	T * GetNextItem(
		T * node,
		int id)
	{
		if (!node)
		{
			return GetItem(id);
		}

		if (node->pNext && node->pNext->nId == id)
		{
			return node->pNext;
		}
		return NULL;
	}

	T * GetFirstItem(
		void)
	{
		ASSERT_RETNULL(m_pHash);
		for (unsigned int ii = 0; ii < m_nSize; ii++)
		{
			if (m_pHash[ii])
			{
				return m_pHash[ii];
			}
		}
		return NULL;
	}

	T * GetNextItemInHash(
		T * node)
	{
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

#ifdef _DEBUG
	int GetCount(
		void)
	{
		return m_nTotalCount;
	}
#endif
};


#endif // _INORDERHASH_H_
