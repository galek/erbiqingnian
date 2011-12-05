//----------------------------------------------------------------------------
// fixedarray.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once 

namespace FSCommon
{

/////////////////////////////////////////////////////////////////////////////
//
// Class CFixedArray
//
// Parameters
//	T : The type of the item contained in the array
//	MAX_COUNT : The number of items in the array
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, unsigned int MAX_COUNT, bool MT_SAFE>
class CFixedArray
{

	public:
	
		CFixedArray();
		~CFixedArray();
		
	public:
	
		bool						InsertAt(const T& value, unsigned int index);
		bool						PushBack(const T& value);
		bool						RemoveAt(const unsigned int index);
		void						RemoveAll();
		T*							Find(const T& value, unsigned int* index = NULL);
		unsigned int				GetCount();
		bool						IsFull();
		T*							operator[](unsigned int index);
	
	private:

		T							m_Items[MAX_COUNT];
			
		CReaderWriterLock_NS_FAIR	m_CS;
		unsigned int				m_Count;
};


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CFixedArray Constructor
// 
/////////////////////////////////////////////////////////////////////////////	
template<typename T, unsigned int MAX_COUNT, bool MT_SAFE>
CFixedArray<T, MAX_COUNT, MT_SAFE>::CFixedArray() :
	m_Count(0)
{
	if(MT_SAFE)
	{
		m_CS.Init();
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CFixedArray Destructor
//
/////////////////////////////////////////////////////////////////////////////	
template<typename T, unsigned int MAX_COUNT, bool MT_SAFE>
CFixedArray<T, MAX_COUNT, MT_SAFE>::~CFixedArray()
{
	if(MT_SAFE)
	{
		m_CS.Free();
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	InsertAt
// 
// Parameters
//	value : A reference to the value to insert
//	index : The index to insert at
//
// Returns
//	true if the method succeeds, false otherwise
//
// Remarks
//	If there are valid items at a higher index, they will be moved down unless
//	there is no room.  If there is no room, the method returns false
//
/////////////////////////////////////////////////////////////////////////////	
template<typename T, unsigned int MAX_COUNT, bool MT_SAFE>
bool CFixedArray<T, MAX_COUNT, MT_SAFE>::InsertAt(const T& value, unsigned int index)
{
	RWFairWriteAutoLock lock(MT_SAFE ? &m_CS : NULL);
	
	// Don't insert if the index is out of bounds or the array is full
	//
	if(index >= MAX_COUNT || m_Count >= MAX_COUNT)
	{
		DBG_ASSERT(0);
		return false;		
	}
	else
	{
		// Create some space for the item by moving all the items
		// after it by one.
		//
		if(index < m_Count)
		{
			memmove(m_Items + index + 1, m_Items + index, sizeof(T) * MIN<unsigned int>((m_Count - index), MAX_COUNT - (index + 1)));
		}

		++m_Count;
	
		m_Items[index] = value;
		
		return true;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	PushBack
// 
// Parameters
//	value : A reference to the value to insert
//
// Returns
//	true if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////	
template<typename T, unsigned int MAX_COUNT, bool MT_SAFE>
bool CFixedArray<T, MAX_COUNT, MT_SAFE>::PushBack(const T& value)
{
	return InsertAt(value, m_Count);	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	RemoveAt
//
// Parameters
//	index : The index of the item to remove
//
/////////////////////////////////////////////////////////////////////////////	
template<typename T, unsigned int MAX_COUNT, bool MT_SAFE>
bool CFixedArray<T, MAX_COUNT, MT_SAFE>::RemoveAt(const unsigned int index)
{
	RWFairWriteAutoLock lock(MT_SAFE ? &m_CS : NULL);

	// If the index is out of bounds
	//
	if(index >= MAX_COUNT)
	{
		return false;
	}

	// If the node isn't the last node
	//
	if(index < m_Count - 1)
	{
		// Move all the nodes above it down one
		//
		memmove(m_Items + index, m_Items + index + 1, sizeof(T) * (m_Count - index - 1));
	}
	
	--m_Count;
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	RemoveAll
//
/////////////////////////////////////////////////////////////////////////////	
template<typename T, unsigned int MAX_COUNT, bool MT_SAFE>
void CFixedArray<T, MAX_COUNT, MT_SAFE>::RemoveAll()
{
	RWFairWriteAutoLock lock(MT_SAFE ? &m_CS : NULL);
	
	m_Count = 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Find
//
// Parameters
//	value : The value for the item to find
//  index : [out/optional] A pointer that will be set to the index of the item
//	 in the array if found.  Note that for multi-thread use, the caller must
//	 synchronize access to the array for the index to not change
//
/////////////////////////////////////////////////////////////////////////////	
template<typename T, unsigned int MAX_COUNT, bool MT_SAFE>
T* CFixedArray<T, MAX_COUNT, MT_SAFE>::Find(const T& value, unsigned int* index = NULL)
{
	RWFairReadAutoLock lock(MT_SAFE ? &m_CS : NULL);

	for(unsigned int i = 0; i < m_Count; ++i)
	{				
		if(m_Items[i] == value)
		{
			if(index)
			{
				*index = i;
			}			
		
			return &m_Items[i];
		}
	}
	
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetCount
//
// Returns
//	The number of items in the array
//
/////////////////////////////////////////////////////////////////////////////	
template<typename T, unsigned int MAX_COUNT, bool MT_SAFE>
unsigned int CFixedArray<T, MAX_COUNT, MT_SAFE>::GetCount()
{
	return m_Count;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	IsFull
//
// Returns
//	True if the array is full, false otherwise
//
/////////////////////////////////////////////////////////////////////////////	
template<typename T, unsigned int MAX_COUNT, bool MT_SAFE>
bool CFixedArray<T, MAX_COUNT, MT_SAFE>::IsFull()
{
	return (m_Count == MAX_COUNT);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	operator []
//
// Returns
//	A pointer to the item at the specified position or NULL if the index
//	is out of bounds.
//
/////////////////////////////////////////////////////////////////////////////	
template<typename T, unsigned int MAX_COUNT, bool MT_SAFE>
T* CFixedArray<T, MAX_COUNT, MT_SAFE>::operator[](unsigned int index)
{
	RWFairReadAutoLock lock(MT_SAFE ? &m_CS : NULL);

	if(index < m_Count)
	{
		return &m_Items[index];
	}
	else
	{
		return NULL;
	}
}

} // end namespace FSCommon
