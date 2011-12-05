//----------------------------------------------------------------------------
// stack.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _STACK_H_
#define _STACK_H_


//----------------------------------------------------------------------------
// CLASSES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// this is sort of special case, it's useful for storing a stack of handles
//----------------------------------------------------------------------------
class INTEGER_STACK
{
protected:
	int * m_pElements;
	int m_nSize;
	int m_nIncrement;
	int m_nCount;

public:
	inline INTEGER_STACK(int initial_size = 0, int increment_size = 16);
	inline ~INTEGER_STACK(void);

	inline void Destroy(void);

	inline void Push(const int value);
	inline int Pop(void);
	inline int GetCount(void) const;
	inline int Peek(int offset = 0) const;

	inline int Remove(int value);				// O(n)
};

//----------------------------------------------------------------------------
inline INTEGER_STACK::INTEGER_STACK(
	int initial_size,
	int increment_size)
{
	ASSERT(initial_size >= 0);
	ASSERT(increment_size >= 1);

	m_pElements = NULL;
	m_nCount = 0;

	if (initial_size > 0)
	{
		m_pElements = (int *)MALLOC(NULL, initial_size * sizeof(int));
	}
	m_nSize = initial_size;
	m_nIncrement = increment_size;
}

//----------------------------------------------------------------------------
inline INTEGER_STACK::~INTEGER_STACK(
	void)
{
	Destroy();
}

//----------------------------------------------------------------------------
inline void INTEGER_STACK::Destroy(
	void)
{
	if (m_pElements)
	{
		FREE(NULL, m_pElements);
	}
	memclear(this, sizeof(INTEGER_STACK));
}

//----------------------------------------------------------------------------
inline void INTEGER_STACK::Push(
	int value)
{
	if (m_nCount >= m_nSize)
	{
		m_nSize += m_nIncrement;
		m_pElements = (int *)REALLOC(NULL, m_pElements, m_nSize * sizeof(int));
	}
	m_pElements[m_nCount++] = value;
}

//----------------------------------------------------------------------------
inline int INTEGER_STACK::Pop(
	void)
{
	ASSERT(m_nCount > 0);
	if (m_nCount <= 0)
	{
		return -1;
	}
	return m_pElements[--m_nCount];
}

//----------------------------------------------------------------------------
inline int INTEGER_STACK::GetCount(
	void) const
{
	return m_nCount;
}

//----------------------------------------------------------------------------
inline int INTEGER_STACK::Peek(
	int offset) const
{
	ASSERT(offset >= 0 && offset < m_nCount);
	if (!(offset >= 0 && offset < m_nCount))
	{
		return -1;
	}
	return m_pElements[m_nCount-offset-1];
}

//----------------------------------------------------------------------------
inline int INTEGER_STACK::Remove(
	int value)
{
	int count = 0;

	for (int cursor = m_nCount - 1; cursor >= 0; cursor--)
	{
		if (m_pElements[cursor] == value)
		{
			int span = m_nCount - cursor - 1;
			if (span > 0)
			{
				MemoryMove(m_pElements + cursor, (m_nSize - cursor) * sizeof(int), m_pElements + cursor + 1, span * sizeof(int));
			}
			m_nCount--;
			count++;
		}
	}
	return count;
}


//----------------------------------------------------------------------------
// this is a stack which uses static memory, avoiding any sort of memory
// management
// warning: don't save pointers to objects returned by Add/Pop/Peek between
// commands since they all use the same memory space
//----------------------------------------------------------------------------
template <class T>
class STATIC_STACK
{
protected:
	T * m_pElements;
	int m_nSize;
	int m_nTop;
public:
	inline STATIC_STACK(T * buffer, int size);
	inline ~STATIC_STACK(void);

	inline void Clear(void);
	inline T * Add(void);
	inline T * Pop(void);
	inline int GetCount(void) const;
	inline T * Peek(int offset = 0) const;
};

//----------------------------------------------------------------------------
template <class T>
inline STATIC_STACK<T>::STATIC_STACK(
	T * buffer, int size)
{
	m_pElements = buffer;
	m_nSize = size;
	m_nTop = 0;
}

//----------------------------------------------------------------------------
template <class T>
inline STATIC_STACK<T>::~STATIC_STACK(
	void)
{
	memclear(this, sizeof(STATIC_STACK));
}

//----------------------------------------------------------------------------
template <class T>
inline void STATIC_STACK<T>::Clear(
	void)
{
	m_nTop = 0;
}

//----------------------------------------------------------------------------
template <class T>
inline T * STATIC_STACK<T>::Add(
	void)
{
	ASSERT(m_nTop < m_nSize - 1);
	if (m_nTop >= m_nSize - 1)
	{
		return NULL;
	}
	return m_pElements + m_nTop++;
}

//----------------------------------------------------------------------------
template <class T>
inline T * STATIC_STACK<T>::Pop(
	void)
{
	ASSERT(m_nTop > 0);
	if (m_nTop <= 0)
	{
		return NULL;
	}
	return m_pElements + (--m_nTop);
}

//----------------------------------------------------------------------------
template <class T>
inline int STATIC_STACK<T>::GetCount(
	void) const
{
	return m_nTop;
}

//----------------------------------------------------------------------------
template <class T>
inline T * STATIC_STACK<T>::Peek(
	int offset) const
{
	ASSERT(offset >= 0 && offset < m_nTop);
	if (!(offset >= 0 && offset < m_nTop))
	{
		return NULL;
	}
	return m_pElements[m_nTop-offset-1];
}


#endif // _STACK_H_
