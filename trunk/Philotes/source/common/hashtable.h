//----------------------------------------------------------------------------
// hashtable.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once 

#pragma warning(push)
#pragma warning(disable:4127)

namespace FSCommon
{

/////////////////////////////////////////////////////////////////////////////
//
// Class CHashTable
//
// Remarks
//	class I must have a "Next" member to work with the hash table.
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K = I*>
class CHashTable
{

	struct HASH_TABLE_ITEM_LIST
	{
		CCritSectLite			CS; // Taken when adding or removing things from the list
		I*						Head; // A pointer to the head item in the list
		I*						Tail;
		unsigned int			Count;
	};

	public:
	
		CHashTable();
		virtual ~CHashTable();
		
	public:
	
		I*						Get(bool front, unsigned int hash);
		I*						Get(K key);
		void					Put(bool front, unsigned int hash, I* item);
		void					Put(bool front, I* item);
		I*						Find(K key);
		unsigned int			GetCount();
		I*						operator[](unsigned int index);
		void					Lock();
		void					Unlock();
		
	protected:
	
		virtual K				GetKey(I* item) = 0;
		virtual unsigned int	GetHash(K key) = 0;
		
	private:
	
		void					RemoveItem(HASH_TABLE_ITEM_LIST& itemList, I* item, I* prevItem);

	private:
	
		unsigned int			m_Count;
	
		CCritSectLite			m_CS;
	
		// The hash table is an array of lists of items, where
		// the array is indexed by the hashed list key
		//
		HASH_TABLE_ITEM_LIST	m_ItemListArray[TABLE_SIZE];
};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CHashTable Constructor
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K>
CHashTable<I, TABLE_SIZE, MT_SAFE, K>::CHashTable() :
	m_Count(0)
{
	m_CS.Init();

	// Initialize the critical sections for all the lists
	//
	for (unsigned int i = 0; i < TABLE_SIZE; ++i)
	{
		m_ItemListArray[i].Head = NULL;
		m_ItemListArray[i].Tail = NULL;
		m_ItemListArray[i].Count = 0;
		
		if(MT_SAFE)
		{
			m_ItemListArray[i].CS.Init();
		}
	}
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CHashTable Destructor
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K>
CHashTable<I, TABLE_SIZE, MT_SAFE, K>::~CHashTable()
{
	// Terminate the critical sections for all lists
	//
	for (unsigned int i = 0; i < TABLE_SIZE; ++i)
	{
		if(MT_SAFE)
		{
			m_ItemListArray[i].CS.Free();
		}
	}
	
	m_CS.Free();
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Get
//
// Parameters
//	front : Specifies whether the item will be taken from the front or back
//		of the hash list
//	hash : The hash for which the first item for the hash will be retrieved
//
// Returns
//	A pointer to the first item found with the specified hash
// 
// Remarks
//	Removes the item from the list if an item with the hash was found
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K>
I* CHashTable<I, TABLE_SIZE, MT_SAFE, K>::Get(bool front, unsigned int hash)
{
	unsigned int hashIndex = hash % TABLE_SIZE;

	HASH_TABLE_ITEM_LIST& hashTableItemList = m_ItemListArray[hashIndex];
	
	// Lock the list for access
	//
	CSLAutoLock lock(MT_SAFE ? &hashTableItemList.CS : NULL);
	
	// If there are any items in the list
	//
	if(hashTableItemList.Head)
	{
		I* item = NULL;
	
		if(front)
		{
			item = hashTableItemList.Head;
		}
		else
		{
			item = hashTableItemList.Tail;
		}
		
		RemoveItem(hashTableItemList, item, NULL);
		
		
		return item;		
	}
	else
	{
		return NULL;
	}
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Get
//
// Parameters
//	key : The key for the item to find
//
// Returns
//	A pointer to the item if it was found
//
// Remarks
//	Removes the item from the list if found.
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K>
I* CHashTable<I, TABLE_SIZE, MT_SAFE, K>::Get(K key)
{
	unsigned int hashIndex = GetHash(key) % TABLE_SIZE;
	
	HASH_TABLE_ITEM_LIST& hashTableItemList = m_ItemListArray[hashIndex];
	
	// Lock the list for access
	//
	CSLAutoLock lock(MT_SAFE ? &hashTableItemList.CS : NULL);

	// Traverse the item list looking for the item
	//
	I* itrItemList = hashTableItemList.Head;
	I* prevItem = NULL;
	while(itrItemList)
	{
		// Check to see if the current item is the one we're looking for
		//
		if(GetKey(itrItemList) == key)
		{
			RemoveItem(hashTableItemList, itrItemList, prevItem);
			
			return itrItemList;
		}
	
		prevItem = itrItemList;
		itrItemList = itrItemList->Next;
	}
	
	return NULL;	
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Put
//
// Parameters
//	front : Specifies whether the item will be put on the front or back
//		of the hash list
//	hash : The hash that will be used for inserting the item
//	item : The pointer to the item to insert
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K>
void CHashTable<I, TABLE_SIZE, MT_SAFE, K>::Put(bool front, unsigned int hash, I* item)
{
	// Generate a hash index for the hash
	//
	unsigned int hashIndex = hash % TABLE_SIZE;
	
	HASH_TABLE_ITEM_LIST& hashTableItemList = m_ItemListArray[hashIndex];
	
	// Lock the list for access
	//
	CSLAutoLock lock(MT_SAFE ? &hashTableItemList.CS : NULL);
	
	// Add the item to the list
	//
	if(front)
	{
		// Adjust the item
		//
		item->Next = hashTableItemList.Head;
		
		// Adjust the head
		//
		hashTableItemList.Head = item;

		// Adjust the tail
		//
		if(hashTableItemList.Tail == NULL)
		{
			hashTableItemList.Tail = item;
		}
	}
	else
	{
		// Adjust the item
		//
		item->Next = NULL;
		
		// Adjust the prev
		//
		if(hashTableItemList.Tail)
		{
			hashTableItemList.Tail->Next = item;
		}
		
		// Adjust the tail
		//
		hashTableItemList.Tail = item;

		// Adjust the head
		//
		if(hashTableItemList.Head == NULL)
		{
			hashTableItemList.Head = item;
		}
	}	
		
	++hashTableItemList.Count;
	++m_Count;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Put
//
// Parameters
//	front : Specifies whether the item will be put on the front or back
//		of the hash list
//	item : A pointer to the item to insert into the hash table
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K>
void CHashTable<I, TABLE_SIZE, MT_SAFE, K>::Put(bool front, I* item)
{
	// Generate a hash for the item
	//
	K key = GetKey(item);	
	unsigned int hash = GetHash(key);
	
	return Put(front, hash, item);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Find
//
// Parameters
//	key : The key for the item to find
//
// Returns
//	A pointer to the item if it was found
//
// Remarks
//	It is the callers responsibility to lock the hash table during usage of this
//	method and use of the returned pointer, since concurrent access to the table
//	could invalidate the pointer.
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K>
I* CHashTable<I, TABLE_SIZE, MT_SAFE, K>::Find(K key)
{
	unsigned int hashIndex = GetHash(key) % TABLE_SIZE;
	
	HASH_TABLE_ITEM_LIST& hashTableItemList = m_ItemListArray[hashIndex];
	
	// Lock the list for access
	//
	CSLAutoLock lock(MT_SAFE ? &hashTableItemList.CS : NULL);

	// Traverse the item list looking for the item
	//
	I* itrItemList = hashTableItemList.Head;
	I* prevItem = NULL;
	while(itrItemList)
	{
		// Check to see if the current item is the one we're looking for
		//
		if(GetKey(itrItemList) == key)
		{			
			return itrItemList;
		}
	
		prevItem = itrItemList;
		itrItemList = itrItemList->Next;
	}
	
	return NULL;	
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetCount
//
// Returns
//	The number of items in the hash table
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K>
unsigned int CHashTable<I, TABLE_SIZE, MT_SAFE, K>::GetCount()
{
	return m_Count;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	operator []
//
// Parameters
//	index : The index of the item to return
//
// Returns
//	A pointer to the indexed item if it exists, NULL otherwise
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K>
I* CHashTable<I, TABLE_SIZE, MT_SAFE, K>::operator[](unsigned int index)
{
	if(index >= m_Count)
	{
		return NULL;
	}

	for(unsigned int i = 0; i < TABLE_SIZE; ++i)
	{
		if(m_ItemListArray[i].Count == 0)
		{
			continue;
		}
		else if(index >= m_ItemListArray[i].Count)
		{
			index -= m_ItemListArray[i].Count;
		}	
		else
		{
			I* item = m_ItemListArray[i].Head;
			
			for(unsigned int j = 0; j < index; ++j)
			{
				item = item->Next;	
			}
			
			return item;
		}
	}	
	
	DBG_ASSERT(0);
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Lock
//
// Remarks
//	Takes the global lock for the hash table, regardless of whether the
//	table was created with MT_SAFE
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K>
void CHashTable<I, TABLE_SIZE, MT_SAFE, K>::Lock()
{
	m_CS.Enter();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Unlock
//
// Remarks
//	Releases the global lock for the hash table, regardless of whether the
//	table was created with MT_SAFE
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K>
void CHashTable<I, TABLE_SIZE, MT_SAFE, K>::Unlock()
{
	m_CS.Leave();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	RemoveItem
//
// Parameters
//	itemList : A reference to the list to remove from
//	item : A pointer to the item to remove
//	prevItem : A pointer to the previous item if available.  NULL can be
//		specified in which case the entire table is traversed to find 
//		the previous item
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE, typename K>
void CHashTable<I, TABLE_SIZE, MT_SAFE, K>::RemoveItem(HASH_TABLE_ITEM_LIST& itemList, I* item, I* prevItem)
{
	if(item == NULL)
	{
		return;
	}
	
	I* prev = prevItem;
	
	if(prev == NULL)
	{
		// Do a list traversal until we find the previous item
		//
		I* itrItem = itemList.Head;
		
		while(itrItem)
		{
			if(itrItem == item)
			{
				break;
			}
		
			prev = itrItem;			
			itrItem = itrItem->Next;			
		}
	}
	
	// Adjust the previous item
	//
	if(prev)
	{
		prev->Next = item->Next;
	}
	
	// Adjust the head
	//
	if(itemList.Head == item)
	{
		itemList.Head = item->Next;
	}
	
	// Adjust the tail
	//
	if(itemList.Tail == item)
	{
		itemList.Tail = prev;
	}
	
	item->Next = NULL;
		
	--itemList.Count;
	--m_Count;	
}


/////////////////////////////////////////////////////////////////////////////
//
// Class CHashTable
//
// Remarks
//	class I must have a "Next" member to work with the hash table.  
//
//	Partial specialization for K = I*
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
class CHashTable<I, TABLE_SIZE, MT_SAFE, I*>
{

	struct HASH_TABLE_ITEM_LIST
	{
		CCritSectLite			CS; // Taken when adding or removing things from the list
		I*						Head; // A pointer to the head item in the list
		I*						Tail;
		unsigned int			Count;
	};

	public:
	
		CHashTable();
		virtual ~CHashTable();
		
	public:
	
		I*						Get(bool front, unsigned int hash);
		I*						Get(I* key);
		void					Put(bool front, unsigned int hash, I* item);
		void					Put(bool front, I* item);
		I*						Find(I* key);
		unsigned int			GetCount();		
		I*						operator[](unsigned int index);		
		void					Lock();
		void					Unlock();
		
	protected:
	
		virtual I*				GetKey(I* item);
		virtual unsigned int	GetHash(I* key);
		
	private:
	
		void					RemoveItem(HASH_TABLE_ITEM_LIST& itemList, I* item, I* prevItem);

	private:
	
		CCritSectLite			m_CS;
		
		unsigned int			m_Count;
	
		// The hash table is an array of lists of items, where
		// the array is indexed by the hashed list key
		//
		HASH_TABLE_ITEM_LIST	m_ItemListArray[TABLE_SIZE];
};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CHashTable Constructor
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::CHashTable() :
	m_Count(0)
{
	m_CS.Init();

	// Initialize the critical sections for all the lists
	//
	for (unsigned int i = 0; i < TABLE_SIZE; ++i)
	{
		m_ItemListArray[i].Head = NULL;
		m_ItemListArray[i].Tail = NULL;
		m_ItemListArray[i].Count = 0;
		
		if(MT_SAFE)
		{
			m_ItemListArray[i].CS.Init();
		}
	}
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CHashTable Destructor
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::~CHashTable()
{
	// Terminate the critical sections for all lists
	//
	for (unsigned int i = 0; i < TABLE_SIZE; ++i)
	{
		if(MT_SAFE)
		{
			m_ItemListArray[i].CS.Free();
		}
	}
	
	m_CS.Free();
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Get
//
// Parameters
//	front : Specifies whether the item will be taken from the front or back
//		of the hash list
//	hash : The hash for which the first item for the hash will be retrieved
//
// Returns
//	A pointer to the first item found with the specified hash
// 
// Remarks
//	Removes the item from the list if an item with the hash was found
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
I* CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::Get(bool front, unsigned int hash)
{
	unsigned int hashIndex = hash % TABLE_SIZE;

	HASH_TABLE_ITEM_LIST& hashTableItemList = m_ItemListArray[hashIndex];
	
	// Lock the list for access
	//
	CSLAutoLock lock(MT_SAFE ? &hashTableItemList.CS : NULL);
	
	// If there are any items in the list
	//
	if(hashTableItemList.Head)
	{
		I* item = NULL;
	
		if(front)
		{
			item = hashTableItemList.Head;
		}
		else
		{
			item = hashTableItemList.Tail;
		}
		
		RemoveItem(hashTableItemList, item, NULL);
		
		
		return item;		
	}
	else
	{
		return NULL;
	}
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Get
//
// Parameters
//	key : The key for the item to find
//
// Returns
//	A pointer to the item if it was found
//
// Remarks
//	Removes the item from the list if found.
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
I* CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::Get(I* key)
{
	unsigned int hashIndex = GetHash(key) % TABLE_SIZE;
	
	HASH_TABLE_ITEM_LIST& hashTableItemList = m_ItemListArray[hashIndex];
	
	// Lock the list for access
	//
	CSLAutoLock lock(MT_SAFE ? &hashTableItemList.CS : NULL);

	// Traverse the item list looking for the item
	//
	I* itrItemList = hashTableItemList.Head;
	I* prevItem = NULL;
	while(itrItemList)
	{
		// Check to see if the current item is the one we're looking for
		//
		if(GetKey(itrItemList) == key)
		{
			RemoveItem(hashTableItemList, itrItemList, prevItem);
			
			return itrItemList;
		}
	
		prevItem = itrItemList;
		itrItemList = itrItemList->Next;
	}
	
	return NULL;	
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Put
//
// Parameters
//	front : Specifies whether the item will be put on the front or back
//		of the hash list
//	hash : The hash that will be used for inserting the item
//	item : The pointer to the item to insert
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
void CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::Put(bool front, unsigned int hash, I* item)
{
	// Generate a hash index for the hash
	//
	unsigned int hashIndex = hash % TABLE_SIZE;
	
	HASH_TABLE_ITEM_LIST& hashTableItemList = m_ItemListArray[hashIndex];
	
	// Lock the list for access
	//
	CSLAutoLock lock(MT_SAFE ? &hashTableItemList.CS : NULL);
	
	// Add the item to the list
	//
	if(front)
	{
		// Adjust the item
		//
		item->Next = hashTableItemList.Head;
		
		// Adjust the head
		//
		hashTableItemList.Head = item;

		// Adjust the tail
		//
		if(hashTableItemList.Tail == NULL)
		{
			hashTableItemList.Tail = item;
		}
	}
	else
	{
		// Adjust the item
		//
		item->Next = NULL;
		
		// Adjust the prev
		//
		if(hashTableItemList.Tail)
		{
			hashTableItemList.Tail->Next = item;
		}
		
		// Adjust the tail
		//
		hashTableItemList.Tail = item;

		// Adjust the head
		//
		if(hashTableItemList.Head == NULL)
		{
			hashTableItemList.Head = item;
		}
	}	
	
	++hashTableItemList;
	++m_Count;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Put
//
// Parameters
//	front : Specifies whether the item will be put on the front or back
//		of the hash list
//	item : A pointer to the item to insert into the hash table
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
void CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::Put(bool front, I* item)
{
	// Generate a hash for the item
	//
	I* key = GetKey(item);	
	unsigned int hash = GetHash(key);
	
	return Put(front, hash, item);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Find
//
// Parameters
//	key : The key for the item to find
//
// Returns
//	A pointer to the item if it was found
//
// Remarks
//	It is the callers responsibility to lock the hash table during usage of this
//	method and use of the returned pointer, since concurrent access to the table
//	could invalidate the pointer.
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
I* CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::Find(I* key)
{
	unsigned int hashIndex = GetHash(key) % TABLE_SIZE;
	
	HASH_TABLE_ITEM_LIST& hashTableItemList = m_ItemListArray[hashIndex];
	
	// Lock the list for access
	//
	CSLAutoLock lock(MT_SAFE ? &hashTableItemList.CS : NULL);

	// Traverse the item list looking for the item
	//
	I* itrItemList = hashTableItemList.Head;
	I* prevItem = NULL;
	while(itrItemList)
	{
		// Check to see if the current item is the one we're looking for
		//
		if(GetKey(itrItemList) == key)
		{			
			return itrItemList;
		}
	
		prevItem = itrItemList;
		itrItemList = itrItemList->Next;
	}
	
	return NULL;	
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetCount
//
// Returns
//	The number of items in the hash table
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
unsigned int CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::GetCount()
{
	return m_Count;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	operator []
//
// Parameters
//	index : The index of the item to return
//
// Returns
//	A pointer to the indexed item if it exists, NULL otherwise
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
I* CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::operator[](unsigned int index)
{
	if(index >= m_Count)
	{
		return NULL;
	}

	for(unsigned int i = 0; i < TABLE_SIZE; ++i)
	{
		if(index >= m_ItemListArray[i].Count)
		{
			index -= m_ItemListArray[i].Count;
		}	
		else
		{
			I* item = m_ItemListArray[i].Head;
			
			for(unsigned int j = 0; j < index; ++j)
			{
				item = item->Next;	
			}
			
			return item;
		}
	}	
	
	DBG_ASSERT(0);
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Lock
//
// Remarks
//	Takes the global lock for the hash table, regardless of whether the
//	table was created with MT_SAFE
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
void CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::Lock()
{
	m_CS.Enter();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Unlock
//
// Remarks
//	Releases the global lock for the hash table, regardless of whether the
//	table was created with MT_SAFE
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
void CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::Unlock()
{
	m_CS.Leave();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetKey
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
I* CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::GetKey(I* item)
{
	return item;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetHash
//
// Parameters
//	item : A pointer to the item to retrieve a hash index for
//
// Returns
//	An integer hash value for the object
//
// Remarks
//	The value does not have to be within the range of the hash table.
//	It will be modified by the hash table size
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
unsigned int CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::GetHash(I* key)
{	
	unsigned int hash = 0;

	size_t keyValue = PTR_TO_SCALAR(key);
	
	hash = static_cast<unsigned int>(keyValue);

#ifdef _WIN64
	hash += static_cast<unsigned int>(keyValue >> 32);
#endif

	return hash;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	RemoveItem
//
// Parameters
//	itemList : A reference to the list to remove from
//	item : A pointer to the item to remove
//	prevItem : A pointer to the previous item if available.  NULL can be
//		specified in which case the entire table is traversed to find 
//		the previous item
//
/////////////////////////////////////////////////////////////////////////////
template<typename I, unsigned int TABLE_SIZE, bool MT_SAFE>
void CHashTable<I, TABLE_SIZE, MT_SAFE, I*>::RemoveItem(HASH_TABLE_ITEM_LIST& itemList, I* item, I* prevItem)
{
	if(item == NULL)
	{
		return;
	}
	
	I* prev = prevItem;
	
	if(prev == NULL)
	{
		// Do a list traversal until we find the previous item
		//
		I* itrItem = itemList.Head;
		
		while(itrItem)
		{
			if(itrItem == item)
			{
				break;
			}
		
			prev = itrItem;			
			itrItem = itrItem->Next;			
		}
	}
	
	// Adjust the previous item
	//
	if(prev)
	{
		prev->Next = item->Next;
	}
	
	// Adjust the head
	//
	if(itemList.Head == item)
	{
		itemList.Head = item->Next;
	}
	
	// Adjust the tail
	//
	if(itemList.Tail == item)
	{
		itemList.Tail = prev;
	}
	
	item->Next = NULL;
		
	--itemList.Count;
	--m_Count;
	
}

} // end namespace FSCommon

#pragma warning(pop)