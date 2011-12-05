//----------------------------------------------------------------------------
// list.hpp
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// CItemList
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------
// Sets memory pool to use
// Must be called BEFORE use.
//------------------------------------------------------------------------
template <typename T> void CItemList<T>::SetMemPool(struct MEMORYPOOL * pool = NULL)
{
	ASSERT_RETURN(!m_pHead && !m_pTail && !m_pFree);
	m_pPool = pool;
}

//------------------------------------------------------------------------
// Cleans up all memory. Called automatically upon destruction.
//------------------------------------------------------------------------
template <typename T> void CItemList<T>::Destroy(void)
{
	//	free the list
	USER_NODE * itr = m_pHead;
	while (itr)
	{
		USER_NODE * toFree = itr;
		itr = itr->m_pNext;

		FREE_DELETE( m_pPool, toFree, USER_NODE );
	}

	//	free the free list
	itr = m_pFree;
	while( itr )
	{
		USER_NODE * toFree = itr;
		itr = itr->m_pNext;

		FREE_DELETE( m_pPool, toFree, USER_NODE );
	}

	//	clear internal members
	Init( m_pPool );
}

//------------------------------------------------------------------------
// Empties list. Does not clean up memory
//------------------------------------------------------------------------
template <typename T> void CItemList<T>::Clear(void)
{
	if (m_nCount == 0)
		return;

	// Clear count
	m_nCount = 0;

	// Move list to free
	if (m_pTail)
		m_pTail->m_pNext = m_pFree;
	m_pFree = m_pHead;

	m_pHead = m_pTail = NULL;
}

//------------------------------------------------------------------------
template <typename T> BOOL CItemList<T>::PushHead(
	const T & value,
#if DEBUG_MEMORY_ALLOCATIONS
	const char * file,
	unsigned int line,
#endif
	MEMORYPOOL * pool = NULL)
{
	if (!pool)
		pool = m_pPool;

	// init the new node
#if DEBUG_MEMORY_ALLOCATIONS
	USER_NODE * toAdd = GetFreeNode(pool, file, line);
#else
	USER_NODE * toAdd = GetFreeNode(pool);
#endif
	ASSERT_RETFALSE( toAdd );
	toAdd->m_Value = value;

	PushNext( NULL, toAdd );
	return TRUE;
}

//------------------------------------------------------------------------
template <typename T> BOOL CItemList<T>::PushTail(
	const T & value,
#if DEBUG_MEMORY_ALLOCATIONS
	const char * file,
	unsigned int line,
#endif
	MEMORYPOOL * pool = NULL)
{
	if (!pool)
		pool = m_pPool;

	// init the new node
#if DEBUG_MEMORY_ALLOCATIONS
	USER_NODE * toAdd = GetFreeNode(pool, file, line);
#else
	USER_NODE * toAdd = GetFreeNode(pool);
#endif
	ASSERT_RETFALSE( toAdd );
	toAdd->m_Value = value;

	//	link the node
	PushNext( m_pTail, toAdd );
	return TRUE;
}

//------------------------------------------------------------------------
// Will pop the first pointer off the list and return it by value
//------------------------------------------------------------------------
template <typename T> void CItemList<T>::PopHead(T& dest)
{
	USER_NODE * head = m_pHead;

	if (head)
	{
		dest = head->m_Value;
		RemoveCurrent( head );
	}
}

template <typename T> void CItemList<T>::PopHead(void)
{
	if (m_pHead)
		RemoveCurrent(m_pHead);
}

//------------------------------------------------------------------------
// Will pop the last pointer off the list and return it by value
//------------------------------------------------------------------------
template <typename T> void CItemList<T>::PopTail(T& dest)
{
	USER_NODE * head = m_pHead;

	if (head && head->m_pPrev)
	{
		dest = head->m_pPrev->m_Value;
		RemoveCurrent( head->m_pPrev );
	}
}

template <typename T> void CItemList<T>::PopTail(void)
{
	if (head && head->m_pPrev)
		RemoveCurrent( head->m_pPrev );
}

//------------------------------------------------------------------------
// Searches the list and deletes any item matched using the == operator
//------------------------------------------------------------------------
template <typename T> BOOL CItemList<T>::FindAndRemove(const T & toFind)
{
	USER_NODE * itr = m_pHead;
	while (itr)
	{
		if (itr->m_Value == toFind)
		{
			RemoveCurrent(itr);
			return TRUE;
		}
		itr = itr->m_pNext;
	}
	return FALSE;
}

//------------------------------------------------------------------------
// Called internally from constructor and Destroy functions.
// Be very, very careful if you want to make this function virtual
//------------------------------------------------------------------------
template <typename T> void CItemList<T>::Init(struct MEMORYPOOL * pool = NULL)
{
	m_pHead = m_pTail = m_pFree = NULL;
	m_pPool = pool;
	m_nCount = 0;
}

//------------------------------------------------------------------------
template <typename T> void CItemList<T>::PushNext(USER_NODE * prev, USER_NODE * toAdd)
{
	// link in the node
	toAdd->m_pPrev = prev;
	if (prev)
	{
		toAdd->m_pNext = prev->m_pNext;
		if (prev->m_pNext)
			prev->m_pNext->m_pPrev = toAdd;
		prev->m_pNext = toAdd;
	}
	else
	{
		toAdd->m_pNext = m_pHead;
		m_pHead = toAdd;
	}
	if (prev == m_pTail)
		m_pTail = toAdd;

	//	update our count
	++m_nCount;
}

//------------------------------------------------------------------------
template <typename T> void CItemList<T>::RemoveCurrent(USER_NODE * toFree)
{
	//	unhook the node
	if (toFree->m_pPrev)
		toFree->m_pPrev->m_pNext = toFree->m_pNext;
	else
		m_pHead = toFree->m_pNext;
	if (toFree->m_pNext)
		toFree->m_pNext->m_pPrev = toFree->m_pPrev;
	else
		m_pTail = toFree->m_pPrev;

	//	update our count
	--m_nCount;

	//	add it to the free list
	toFree->m_pNext = m_pFree;
	m_pFree	  = toFree;
}


//-----------------------------------------------------------------------------
// CItemPtrList
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------
// Cleans up all memory including items pointed to.
// Called automatically upon destruction.
//------------------------------------------------------------------------
template <typename T> void CItemPtrList<T>::Destroy(void)
{
	// Delete items pointed to
	USER_NODE * head = m_pHead;
	while (head)
	{
		FREE_DELETE(m_pPool, head->m_Value, T);
		head->m_Value = NULL;
		head = head->m_pNext;
	}

	// Now clean up the list
	CItemList::Destroy();
}

//------------------------------------------------------------------------
// Will either pop the first item off the list and delete it OR
// pop the first item off the list and return it.
//------------------------------------------------------------------------
template <typename T> T* CItemPtrList<T>::PopHead (bool bDelete)
{
	USER_NODE * head = m_pHead;
	T * toRet = NULL;

	if (head)
	{
		if (bDelete)
		{
			FREE_DELETE(m_pPool, head->m_Value, T);
			head->m_Value = NULL;
		}
		else
		{
			toRet = head->m_Value;
		}
		RemoveCurrent(head);
	}
	return toRet;
}

//------------------------------------------------------------------------
// Will either pop the last item off the list and delete it OR
// pop the first last off the list and return it.
//------------------------------------------------------------------------
template <typename T> T* CItemPtrList<T>::PopTail (bool bDelete)
{
	USER_NODE * tail = m_pTail;
	T * toRet = NULL;

	if (tail)
	{
		if (bDelete)
		{
			FREE_DELETE(m_pPool, tail->m_Value, T);
			tail->m_Value = NULL;
		}
		else
		{
			toRet = tail->m_Value;
		}
		RemoveCurrent(tail);
	}
	return toRet;
}

//------------------------------------------------------------------------
// Searches the list and deletes any item matched using the == operator
//------------------------------------------------------------------------
template <typename T> BOOL CItemPtrList<T>::FindAndDelete(const T* toFind)
{
	USER_NODE * itr = m_pHead;
	while (itr)
	{
		if (itr->m_Value == toFind)
		{
			FREE_DELETE(m_pPool,itr->m_Value, T);
			itr->m_Value = NULL;
			RemoveCurrent(itr);
			return TRUE;
		}
		itr = itr->m_pNext;
	}
	return FALSE;
}

//------------------------------------------------------------------------
// Searches the list and returns true if the pointer is in the list
//------------------------------------------------------------------------
template <typename T> BOOL CItemPtrList<T>::IsInList(const T* toFind)
{
	USER_NODE * itr = m_pHead;
	while (itr)
	{
		if (itr->m_Value == toFind)
			return TRUE;
		itr = itr->m_pNext;
	}
	return FALSE;
}

