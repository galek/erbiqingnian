// ---------------------------------------------------------------------------
// FILE:	rotlist.h
// DESC:	implements a fixed-size FIFO list using an array
//
//  Copyright 2003, Flagship Studios
// ---------------------------------------------------------------------------
#ifndef _ROTLIST_H_
#define _ROTLIST_H_


// ---------------------------------------------------------------------------
// TEMPLATE
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// DESC:	implements a fixed size FIFO list using an array
//			this is the thread-safe version
// ---------------------------------------------------------------------------
template <typename T, unsigned int size, T invalid_value>
struct CRotList_MT
{
	CCritSectLite					m_cs;							// critical section
	T *								m_pList;						// array of pointers to items
	unsigned int					m_nStart;						// first item index
	unsigned int					m_nCount;						// # of items

	void Init(
		struct MEMORYPOOL * pool)
	{
		m_cs.Init();
		m_pList = (T *)MALLOCZ(pool, sizeof(T) * size);
		for (int ii = 0; ii < size; ii++)
		{
			m_pList[ii] = invalid_value;
		}
		m_nStart = 0;
		m_nCount = 0;
	}

	void Free(
		struct MEMORYPOOL * pool)
	{
		m_cs.Free();
		if (m_pList)
		{
			FREE(pool, m_pList);
			m_pList = NULL;
		}
		m_nStart = 0;
		m_nCount = 0;
	}

	BOOL Push(
		T item)
	{
		ASSERT_RETFALSE(m_pList);
		BOOL retval = FALSE;

		m_cs.Enter();
		ONCE
		{
			ASSERT_BREAK(m_nCount < size);
			int idx = (m_nStart + m_nCount) % size;
			m_pList[idx] = item;
			m_nCount++;
			retval = TRUE;
		}
		m_cs.Leave();
		return retval;
	}

	T Pop(
		void)
	{
		ASSERT_RETFALSE(m_pList);
		T item = invalid_value;

		m_cs.Enter();
		ONCE
		{
			if (m_nCount == 0)
			{
				break;
			}
			item = m_pList[m_nStart];
			m_nStart = (m_nStart + 1) % size;
			m_nCount--;
		}
		m_cs.Leave();
		return item;
	}

	T Peek(
		unsigned int idx=0)
	{
		ASSERT_RETFALSE(m_pList);
		T item = invalid_value;

		m_cs.Enter();
		ONCE
		{
			if (m_nCount == 0 || idx >= m_nCount)
			{
				break;
			}
			item = m_pList[(m_nStart + idx) % size];
		}
		m_cs.Leave();
		return item;
	}
};


#endif // _ROTLIST_H_


