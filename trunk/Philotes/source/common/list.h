//----------------------------------------------------------------------------
// list.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef	_LIST_H_
#define _LIST_H_


//----------------------------------------------------------------------------
// PList
// Simple doubly linked list implementation offering push, pop, and iteration
//		with removal.
// NOTE: full T member stored by value per list node. assumes small type or
//			pointer to actual data. does not support safe iteration by two or
//			more pointers when removing nodes.
//----------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
#define PListPushHeadPool(p, i)				PushHead(p, i, __FILE__, __LINE__)
#define PListPushTailPool(p, i)				PushTail(p, i, __FILE__, __LINE__)
#define PListPushHead(i)					PushHead(i, __FILE__, __LINE__)
#define PListPushTail(i)					PushTail(i, __FILE__, __LINE__)
#else
#define PListPushHeadPool(p, i)				PushHead(p, i)
#define PListPushTailPool(p, i)				PushTail(p, i)
#define PListPushHead(i)					PushHead(i)
#define PListPushTail(i)					PushTail(i)
#endif

template<class T>
struct PList
{
protected:
	//	container struct
	struct NODE
	{
		NODE *			pNext;
		NODE *			pPrev;
	};
public:
	//	user data container struct
	struct USER_NODE : NODE
	{
		T				Value;
	};

protected:
	//	private data members
	NODE				m_head;
	unsigned int		m_count;
	USER_NODE *			m_freeList;
	MEMORYPOOL *		m_pool;

	//------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
	USER_NODE * GetFreeNode(
		MEMORYPOOL * pool,
		const char * file,
		unsigned int line)
#else
	USER_NODE * GetFreeNode(
		MEMORYPOOL * pool )
#endif
	{
		USER_NODE * toRet = m_freeList;
		if (toRet)
		{
			m_freeList = (USER_NODE *)toRet->pNext;
		}
		else
		{
			toRet = (USER_NODE *)MALLOCZFL(pool, sizeof(USER_NODE), file, line);
		}
		return toRet;
	}

	//------------------------------------------------------------------------
	void AddNext(
		NODE * prev,
		NODE * toAdd)
	{
#if ISVERSION(DEVELOPMENT)
		ASSERT_RETURN(prev);
		ASSERT_RETURN(toAdd);
#endif
		//	link the node
		toAdd->pNext = prev->pNext;
		toAdd->pPrev = prev;
		toAdd->pNext->pPrev = toAdd;
		toAdd->pPrev->pNext = toAdd;

		//	update our count
		++m_count;
	}

	//------------------------------------------------------------------------
	void DeleteCurrent(
		USER_NODE * toFree )
	{
		//	unhook the node
		toFree->pPrev->pNext = toFree->pNext;
		toFree->pNext->pPrev = toFree->pPrev;
#if ISVERSION(DEVELOPMENT)
		toFree->pPrev = NULL;
#endif

		//	update our count
		--m_count;

		//	add it to the free list
		toFree->pNext = m_freeList;
		m_freeList	  = toFree;
	}

public:
	//------------------------------------------------------------------------
	void Init(
		struct MEMORYPOOL * pool = NULL )
	{
		m_head.pNext = m_head.pPrev = &m_head;
		m_count		 = 0;
		m_freeList	 = NULL;
		m_pool		 = pool;
	}
	//------------------------------------------------------------------------
	void Destroy(
		MEMORYPOOL * pool,
		void (*fpDestFunc)(
			struct MEMORYPOOL *,
			T & ) )
	{
		//	free the list
		NODE * itr = m_head.pNext;
		if (!itr)
			return;
		while( itr != &m_head )
		{
			USER_NODE * toFree = (USER_NODE*)itr;
			itr = itr->pNext;

			if( fpDestFunc ) {
				fpDestFunc( pool, toFree->Value );
			}

			FREE( pool, toFree );
		}

		//	free the free list
		USER_NODE * freeItr = m_freeList;
		while( freeItr )
		{
			USER_NODE * toFree = freeItr;
			freeItr = (USER_NODE*)freeItr->pNext;

			FREE( pool, toFree );
		}

		//	clear internal members
		Init( pool );
	}
	//------------------------------------------------------------------------
	void Destroy(
		MEMORYPOOL * pool )
	{
		Destroy( pool, NULL );
	}
	//------------------------------------------------------------------------
	void Destroy(
		void )
	{
		Destroy( m_pool, NULL );
	}
	//------------------------------------------------------------------------
	void Clear(
		void )
	{
		if( m_count == 0 ) {
			return;
		}

		//	unhook entire list
		USER_NODE * head = (USER_NODE*)m_head.pNext;
		USER_NODE * tail = (USER_NODE*)m_head.pPrev;
		m_head.pNext = m_head.pPrev = &m_head;

		//	move all to free list
#if ISVERSION( DEBUG_VERSION )
		head->pPrev = NULL;
#endif
		tail->pNext = m_freeList;
		m_freeList  = head;

		//	clear count
		m_count = 0;
	}
	//------------------------------------------------------------------------
	unsigned int Count(
		void ) const
	{
		return m_count;
	}
	//------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
	BOOL PushHead(
		MEMORYPOOL * pool,
		const T & value,
		const char * file,
		unsigned int line)
#else
	BOOL PushHead(
		MEMORYPOOL * pool,
		const T & value)
#endif
	{
		//	init the new node
#if DEBUG_MEMORY_ALLOCATIONS
		USER_NODE * toAdd = GetFreeNode(pool, file, line);
#else
		USER_NODE * toAdd = GetFreeNode(pool);
#endif
		ASSERT_RETFALSE( toAdd );
		toAdd->Value = value;

		//	link the node
		AddNext( &m_head, toAdd );
		return TRUE;
	}
	//------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
	BOOL PushHead(
		const T & value,
		const char * file,
		unsigned int line)
	{
		return PushHead(m_pool, value, file, line);
	}
#else
	BOOL PushHead(
		const T & value)
	{
		return PushHead(m_pool, value);
	}
#endif
	//------------------------------------------------------------------------
	// if returns TRUE, dest set to value of removed head node
	//------------------------------------------------------------------------
	BOOL PopHead(
		T & dest)
	{
		USER_NODE * head = (USER_NODE*)m_head.pNext;

		if( head != &m_head )
		{
			dest = head->Value;
			DeleteCurrent( head );
			return TRUE;
		}
		return FALSE;
	}
	//------------------------------------------------------------------------
	// Simply pops first element off the list. Nothing returned.
	//------------------------------------------------------------------------
	void PopHead(
		void)
	{
		USER_NODE * head = (USER_NODE*)m_head.pNext;

		if( head != &m_head )
			DeleteCurrent( head );
	}
	//------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
	BOOL PushTail(
		MEMORYPOOL * pool,
		const T & value,
		const char * file,
		unsigned int line)
#else
	BOOL PushTail(
		MEMORYPOOL * pool,
		const T & value)
#endif
	{
		//	init the new node
#if DEBUG_MEMORY_ALLOCATIONS
		USER_NODE * toAdd = GetFreeNode(pool, file, line);
#else
		USER_NODE * toAdd = GetFreeNode(pool);
#endif
		ASSERT_RETFALSE( toAdd );
		toAdd->Value = value;

		//	link the node
		AddNext( m_head.pPrev, toAdd );
		return TRUE;
	}
	//------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
	BOOL PushTail(
		const T & value,
		const char * file,
		unsigned int line)
	{
		return PushTail(m_pool, value, file, line);
	}
#else
	BOOL PushTail(
		const T & value)
	{
		return PushTail(m_pool, value);
	}
#endif
	//------------------------------------------------------------------------
	// if returns TRUE, dest set to value of removed tail node
	//------------------------------------------------------------------------
	BOOL PopTail(
		T & dest)
	{
		USER_NODE * tail = (USER_NODE*)m_head.pPrev;

		if( tail != &m_head )
		{
			dest = tail->Value;
			DeleteCurrent( tail );
			return TRUE;
		}
		return FALSE;
	}
	//------------------------------------------------------------------------
	// Simply pops last element off the list. Nothing returned.
	//------------------------------------------------------------------------
	void PopTail(
		void)
	{
		USER_NODE * tail = (USER_NODE*)m_head.pPrev;

		if( tail != &m_head )
			DeleteCurrent( tail );
	}
	//------------------------------------------------------------------------
	// pass NULL to get the head of the list
	//------------------------------------------------------------------------
	USER_NODE * GetNext(
		USER_NODE * itr ) const
	{
		USER_NODE * toRet = ( itr ) ? (USER_NODE*)itr->pNext : (USER_NODE*)m_head.pNext;
		return ( toRet == &m_head ) ? NULL : toRet;
	}
	//------------------------------------------------------------------------
	// pass NULL to get the tail of the list
	//------------------------------------------------------------------------
	USER_NODE * GetPrev(
		USER_NODE * itr ) const
	{
		USER_NODE * toRet = ( itr ) ? (USER_NODE*)itr->pPrev : (USER_NODE*)m_head.pPrev;
		return ( toRet == &m_head ) ? NULL : toRet;
	}
	//------------------------------------------------------------------------
	// removes the node pointed to by itr and returns the next node in the list or NULL if at end of list.
	//------------------------------------------------------------------------
	USER_NODE * RemoveCurrent(
		USER_NODE * itr )
	{
		DBG_ASSERT_RETNULL( itr );
		DBG_ASSERT_RETNULL( itr != &m_head );
		
		//	get the next node and remove the current node
		USER_NODE * toRet = (USER_NODE*)itr->pNext;
		DeleteCurrent( itr );

		//	validate return
		return ( toRet == &m_head ) ? NULL : toRet;
	}

	//------------------------------------------------------------------------
	// searches the entire list for the item using the == operator, return TRUE if found
	//------------------------------------------------------------------------
	BOOL Find(
		const T & toFind )
	{
		USER_NODE * itr = (USER_NODE*)m_head.pNext;
		while( itr != (USER_NODE*)&m_head )
		{
			if( toFind == itr->Value )
			{
				return TRUE;
			}
			itr = (USER_NODE*)itr->pNext;
		}
		return FALSE;
	}
	//------------------------------------------------------------------------
	// searches the entire list for the item using the == operator, deletes it and returns TRUE if found
	//------------------------------------------------------------------------
	BOOL FindAndDelete(
		const T & toFind )
	{
		USER_NODE * itr = (USER_NODE*)m_head.pNext;
		while( itr != (USER_NODE*)&m_head )
		{
			if( toFind == itr->Value )
			{
				DeleteCurrent( itr );
				return TRUE;
			}
			itr = (USER_NODE*)itr->pNext;
		}
		return FALSE;
	}
};



//-----------------------------------------------------------------------------
// CItemList
// KCK: OK, I give up. Since the PList class is only half written in C++ and
// the structures and classes that use it often don't use it in a C++ way (ie:
// constructors aren't always called and so on), I've decided just to add
// another class that's actually written in C++. Users of this should conform
// to C++ conventions such as making sure containing classes call constructors
// and making sure new (MALLOC_NEW) is used to allocate memory.
// NOTE that there is also a CItemPtrList class, which is specialized to handle
// lists of pointers, which in most cases is probably what we want to do unless
// the items are pretty simple and small (like a list on integers)
//-----------------------------------------------------------------------------

// Macros to deal with our memory manager. Note that an overridden new operator
// would solve this much more cleanly.
#if DEBUG_MEMORY_ALLOCATIONS
#define CItemListPushHead(i)				PushHead(i, __FILE__, __LINE__)
#define CItemListPushTail(i)				PushTail(i, __FILE__, __LINE__)
#define CItemListPushHeadPool(i, p)			PushHead(i, __FILE__, __LINE__, p)
#define CItemListPushTailPool(i, p)			PushTail(i, __FILE__, __LINE__, p)
#else
#define CItemListPushHead(i)				PushHead(i)
#define CItemListPushTail(i)				PushTail(i)
#define CItemListPushHeadPool(i, p)			PushHead(i, p)
#define CItemListPushTailPool(i, p)			PushTail(i, p)
#endif

template<typename T>
class CItemList
{
public:
	class USER_NODE;
	class NODE
	{
	public:
		USER_NODE *		m_pNext;
		USER_NODE *		m_pPrev;
	};
	class USER_NODE : public NODE
	{
	public:
		T				m_Value;
	};

protected:
	USER_NODE *			m_pHead;
	USER_NODE *			m_pTail;
	USER_NODE *			m_pFree;
	MEMORYPOOL *		m_pPool;
	unsigned int		m_nCount;

public:
	CItemList(struct MEMORYPOOL * pool = NULL)	{ Init(pool); }
	virtual	~CItemList(void)					{ Destroy(); }

	//------------------------------------------------------------------------
	// Public methods:
	//------------------------------------------------------------------------
	//  void SetMemPool(MEMORYPOOL*)	// Sets memory pool to use
	//	void Destroy(void)				// Destroy list, freeing memory
	// 	void Clear(void)				// Clear list, does not free memory
	//  unsigned int Count(void)		// Returns list size
	//  BOOL PushHead(T&, MEMORYPOOL*)	// Push item onto list front
	//  BOOL PushTail(T&, MEMORYPOOL*)	// Push item onto list end
	//	void PopHead(T&)				// Pop item off list front and return
	//	void PopHead()					// Pop item off list front
	//	void PopTail(T&)				// Pop item off list end and return
	//  void PopTail()					// Pop item off list end
	//  USER_NODE * GetNext(USER_NODE*)	// Iterate the list
	//  USER_NODE * GetPrev(USER_NODE*)	// Iterate the list
	//  BOOL FindAndRemove(T&)			// Finds matching item and removes it
	//------------------------------------------------------------------------
	void			SetMemPool(struct MEMORYPOOL * pool = NULL);
	virtual void	Destroy(void);
	void			Clear(void);
	unsigned int	Count(void) const			{ return m_nCount; }
	BOOL			PushHead(const T & value,
#if DEBUG_MEMORY_ALLOCATIONS
							 const char * file,
							 unsigned int line,
#endif
					   		 MEMORYPOOL * pool = NULL);
	BOOL			PushTail(const T & value,
#if DEBUG_MEMORY_ALLOCATIONS
							 const char * file,
							 unsigned int line,
#endif
							 MEMORYPOOL * pool = NULL);
	void			PopHead(T& dest);
	void			PopHead(void);
	void			PopTail(T& dest);
	void			PopTail(void);
	USER_NODE *		GetNext(USER_NODE * itr) const	{ return (itr) ? itr->m_pNext : m_pHead; }
	USER_NODE *		GetPrev(USER_NODE * itr) const	{ return (itr) ? itr->m_pPrev : m_pTail; }
	BOOL			FindAndRemove(const T & toFind);

protected:
	void			Init(struct MEMORYPOOL * pool = NULL);
	void			PushNext(USER_NODE * prev, USER_NODE * toAdd);
	void			RemoveCurrent(USER_NODE * toFree);

	USER_NODE *		GetFreeNode(
#if DEBUG_MEMORY_ALLOCATIONS
		MEMORYPOOL * pool,
		const char * file,
		unsigned int line)
#else
		MEMORYPOOL * pool)
#endif
	{
		USER_NODE * toRet = m_pFree;
		if (toRet) 	m_pFree = toRet->m_pNext;
		else 		toRet = MALLOC_NEWFL(pool, USER_NODE, file, line);
		return toRet;
	}
};


//-----------------------------------------------------------------------------
// CItemPtrList
// A specialized version of CList designed to deal better with pointer, which 
// in most cases is probably what we want to do unless the items are pretty 
// simple and small (like a list on integers).
// Note that this will clean up the items POINTED TO upon destruction and
// upon Pop calls if the bDelete parameter is set to true.
//-----------------------------------------------------------------------------
template<typename T>
class CItemPtrList : public CItemList<T*>
{
friend CItemList;
public:
	CItemPtrList(struct MEMORYPOOL * pool = NULL) : CItemList(pool) {}
	virtual	~CItemPtrList(void)					{ Destroy(); }

	//------------------------------------------------------------------------
	// Public methods:
	//------------------------------------------------------------------------
	//	void Destroy(void)				// Destroy list, freeing memory
	// 	T* PopHead(bool)				// Either returns ptr OR deletes it.
	// 	T* PopTail(bool)				// Either returns ptr OR deletes it.
	//  BOOL FindAndDelete(T*)			// Finds matching pointer and deletes
	//  BOOL IsInList(T*)				// Returns true if pointer is in list
	//------------------------------------------------------------------------
	virtual void	Destroy(void);
	T*				PopHead (bool bDelete);
	T*				PopTail (bool bDelete);
	BOOL			FindAndDelete(const T* toFind);
	BOOL			IsInList(const T* toFind);
};




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct LIST_SL_NODE
{
	LIST_SL_NODE *		m_List_Next;

	void SLNodeInit(void) { m_List_Next = NULL; }
};

struct LIST_DL_NODE : LIST_SL_NODE
{
	LIST_DL_NODE *		m_List_Prev;

	void DLNodeInit(void) { SLNodeInit(); m_List_Prev = NULL; }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct LIST_SING
{
	LIST_SL_NODE *		m_Head;

	void Init(
		void)
	{
		m_Head = NULL;
	}

	LIST_SING(
		void) : m_Head(NULL) {};

	void Push(
		LIST_SL_NODE * item)
	{
		ASSERT_RETURN(item);
		item->m_List_Next = m_Head;
		m_Head = item;
	}

	LIST_SL_NODE * Pop(
		void)
	{
		LIST_SL_NODE * item = m_Head;
		if (item)
		{
			m_Head = m_Head->m_List_Next;
			item->m_List_Next = NULL;
		}
		return item;
	}

	void PopList(
		LIST_SL_NODE * & head)
	{
		head = m_Head;
		m_Head = NULL;
	}
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct LIST_SING_MT : LIST_SING
{
	CCritSectLite		cs;

	void Init(
		void)
	{
		cs.Init();
		LIST_SING::Init();
	}

	void Push(
		LIST_SL_NODE * item)
	{
		cs.Enter();
		{
			LIST_SING::Push(item);
		}
		cs.Leave();
	}

	LIST_SL_NODE * Pop(
		void)
	{
		LIST_SL_NODE * item;

		cs.Enter();
		{
			item = LIST_SING::Pop();
		}
		cs.Leave();

		return item;
	}

	void PopList(
		LIST_SL_NODE * & head)
	{
		cs.Enter();
		{
			LIST_SING::PopList(head);
		}
		cs.Leave();
	}
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct LIST_CIRC
{
	LIST_DL_NODE		m_List;
	DWORD				m_count;

	void Init(
		void)
	{
		m_List.m_List_Prev = &m_List;
		m_List.m_List_Next = &m_List;
		m_count = 0;
	}

	LIST_CIRC(
		void) 
	{
		Init();
	}

	void InsertHead(
		LIST_DL_NODE * item)
	{
		ASSERT_RETURN(m_List.m_List_Prev && m_List.m_List_Next);
		ASSERT_RETURN(item);
		DBG_ASSERT(item->m_List_Prev == NULL && item->m_List_Next == NULL);
		item->m_List_Prev = &m_List;
		item->m_List_Next = m_List.m_List_Next;
		((LIST_DL_NODE *)(item->m_List_Next))->m_List_Prev = item;
		m_List.m_List_Next = item;
		++m_count;
	}

	void InsertTail(
		LIST_DL_NODE * item)
	{
		ASSERT_RETURN(m_List.m_List_Prev && m_List.m_List_Next);
		ASSERT_RETURN(item);
		DBG_ASSERT(item->m_List_Prev == NULL && item->m_List_Next == NULL);
		item->m_List_Next = &m_List;
		item->m_List_Prev = m_List.m_List_Prev;
		item->m_List_Prev->m_List_Next = item;
		m_List.m_List_Prev = item;
		++m_count;
	}

	void Remove(
		LIST_DL_NODE * item)
	{
		ASSERT_RETURN(m_List.m_List_Prev && m_List.m_List_Next);
		ASSERT_RETURN(item);
		if (!item->m_List_Prev || !item->m_List_Next)
			return;
		item->m_List_Prev->m_List_Next = item->m_List_Next;
		((LIST_DL_NODE *)(item->m_List_Next))->m_List_Prev = item->m_List_Prev;
		item->m_List_Prev = NULL;
		item->m_List_Next = NULL;
		--m_count;
	}

	LIST_DL_NODE * Pop(
		void)
	{
		ASSERT_RETNULL(m_List.m_List_Prev && m_List.m_List_Next);
		if (m_List.m_List_Prev == &m_List)
		{
			ASSERT(m_List.m_List_Next == &m_List);
			return NULL;
		}
		ASSERT_RETNULL(m_List.m_List_Prev != &m_List);

		LIST_DL_NODE * item = (LIST_DL_NODE *)m_List.m_List_Next;
		Remove(item);
		return item;
	}

	void PopList(
		LIST_DL_NODE * & head,
		LIST_DL_NODE * & tail)
	{
		ASSERT_RETURN(m_List.m_List_Prev && m_List.m_List_Next);
		if (m_List.m_List_Prev == &m_List)
		{
			DBG_ASSERT(m_List.m_List_Next == &m_List);
			head = NULL;
			tail = NULL;
			return;
		}
		DBG_ASSERT(m_List.m_List_Prev != &m_List);
		head = (LIST_DL_NODE *)m_List.m_List_Next;
		tail = m_List.m_List_Prev;
		m_List.m_List_Prev = &m_List;
		m_List.m_List_Next = &m_List;
		if (head)
		{
			head->m_List_Prev = NULL;
		}
		if (tail)
		{
			tail->m_List_Next = NULL;
		}
		m_count = 0;
	}

	DWORD Count(
		void )
	{
		return m_count;
	}

	LIST_DL_NODE * GetNext(
		LIST_DL_NODE * itr )
	{
		if (itr == NULL)
			itr = &m_List;
		if (itr->m_List_Next == &m_List)
			return NULL;
		return (LIST_DL_NODE *)itr->m_List_Next;
	}
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct LIST_CIRC_MT : LIST_CIRC
{
	CCritSectLite		cs;

	void Init(
		void)
	{
		cs.Init();
		LIST_CIRC::Init();
	}

	void InsertHead(
		LIST_DL_NODE * item)
	{
		cs.Enter();
		{
			LIST_CIRC::InsertHead(item);
		}
		cs.Leave();
	}

	void InsertTail(
		LIST_DL_NODE * item)
	{
		cs.Enter();
		{
			LIST_CIRC::InsertTail(item);
		}
		cs.Leave();
	}

	void Remove(
		LIST_DL_NODE * item)
	{
		cs.Enter();
		{
			LIST_CIRC::Remove(item);
		}
		cs.Leave();
	}

	LIST_DL_NODE * Pop(
		void)
	{
		LIST_DL_NODE * item = NULL;
		cs.Enter();
		{
			item = LIST_CIRC::Pop();
		}
		cs.Leave();
		return item;
	}

	void PopList(
		LIST_DL_NODE * & head,
		LIST_DL_NODE * & tail)
	{
		cs.Enter();
		{
			LIST_CIRC::PopList(head, tail);
		}
		cs.Leave();
	}
};

//----------------------------------------------------------------------------
// doubly linked list
//----------------------------------------------------------------------------
template <typename T>
struct DOUBLE_LIST
{
	T *					m_head;
	T *					m_tail;

	void Init(
		void)
	{
		m_head = m_tail = NULL;
	}

	void Free(
		void)
	{
		m_head = m_tail = NULL;
	}

	T * Head(
		void) const
	{
		return m_head;
	}

	void AddToTail(
		T * item)
	{
		ASSERT_RETURN(item->m_prev == NULL && item->m_next == NULL && item != m_head && item != m_tail)

		item->m_prev = m_tail;
		item->m_next = NULL;
		if (m_tail)
		{
			m_tail->m_next = item;
			m_tail = item;
		}
		else
		{
			m_head = m_tail = item;
		}
	}

	void AddToHead(
		T * item)
	{
		ASSERT_RETURN(item->m_prev == NULL && item->m_next == NULL && item != m_head && item != m_tail)

		item->m_prev = NULL;
		item->m_next = m_head;
		if (m_head)
		{
			m_head->m_prev = item;
			m_head = item;
		}
		else
		{
			m_head = m_tail = item;
		}
	}

	void Add(
		T * item)
	{
		AddToTail(item);
	}

	void Remove(
		T * item)
	{
		if (item->m_next)
		{
			item->m_next->m_prev = item->m_prev;
		}
		else
		{
			m_tail = item->m_prev;
		}
		if (item->m_prev)
		{
			item->m_prev->m_next = item->m_next;
		}
		else
		{
			m_head = item->m_next;
		}
		item->m_prev = item->m_next = NULL;
	}

	T * RemoveHead(
		void)
	{
		T * item = m_head;
		if (item)
		{
			Remove(item);
		}
	}

	T * RemoveTail(
		void)
	{
		T * item = m_tail;
		if (item)
		{
			Remove(item);
		}
		return item;
	}
};

//----------------------------------------------------------------------------
// threadsafe version of doubly linked list
//----------------------------------------------------------------------------
template <typename T>
struct DOUBLE_LIST_MT : DOUBLE_LIST<T>
{
	CCritSectLite		cs;

	void Init(
		void)
	{
		cs.Init();
		DOUBLE_LIST::Init();
	}

	void Free(
		void)
	{
		DOUBLE_LIST::Free();
		cs.Free();
	}

	T* Head(
		void)
	{
		CSLAutoLock lock(&cs);
		return DOUBLE_LIST::Head();
	}

	T CopyOfHead(
		void)
	{
		CSLAutoLock lock(&cs);
		T *pHead = DOUBLE_LIST::Head();
		if(pHead) return *pHead;
		else
		{
			T toRet;
			return toRet; //return the default constructed object
		}
	}

	void AddToTail(
		T * item)
	{
		CSLAutoLock lock(&cs);
		DOUBLE_LIST::AddToTail(item);		
	}

	void AddToHead(
		T * item)
	{
		CSLAutoLock lock(&cs);
		DOUBLE_LIST::AddToHead(item);
	}

	inline void Add(
		T * item)
	{
		AddToTail(item);
	}

	void Remove(
		T* item)
	{
		CSLAutoLock lock(&cs);
		DOUBLE_LIST::Remove(item);
	}

	T * RemoveHead(
		void)
	{
		CSLAutoLock lock(&cs);
		return DOUBLE_LIST::RemoveHead();
	}

	T* RemoveTail(
		void)
	{
		CSLAutoLock lock(&cs);
		return DOUBLE_LIST::RemoveTail();
	}
};
//----------------------------------------------------------------------------
// singly linked list
//----------------------------------------------------------------------------
template <typename T>
struct SINGLE_LIST
{
	T *				m_head;

	void Init(
		void)
	{
		m_head = NULL;
	}

	void Free(
		void)
	{
		m_head = NULL;
	}

	void Add(
		T * item)
	{
		item->m_next = m_head;
		m_head = item;
	}

	T * Remove(
		void)
	{
		T * item = m_head;
		if (item)
		{
			m_head = item->m_next;
		}
		return item;
	}
};


//----------------------------------------------------------------------------
// Includes template body definitions
//----------------------------------------------------------------------------
#include "list.hpp"

#endif	//	_LIST_H_
