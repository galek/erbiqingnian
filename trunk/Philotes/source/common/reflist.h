//----------------------------------------------------------------------------
//	reflist.h
//  Copyright 2006, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _REFERENCELIST_H_
#define _REFERENCELIST_H_



//-----------------------------------------------------------------------------
// REFERENCE_LIST
// Element reference counting list. When a key is added, either a new entry
// is added to the list or an existing entries count is incremented.
// When a key is removed, the entries count is decremented and iff it becomes
// 0 the entry is removed completely.
// NOTE: T must have the operators <, > and = defined.
//-----------------------------------------------------------------------------
template<class T>
struct REFERENCE_LIST
{
private:

	//	key struct
	template<class T>
	struct REF_ENTRY
	{
		int		count;
		T		key;

		REF_ENTRY(
			void )
		{
			count = 0;
		}
		bool operator<(
			const REF_ENTRY<T>& toComp ) const
		{
			return ( key < toComp.key );
		}
		bool operator>(
			const REF_ENTRY<T>& toComp ) const
		{
			return ( key > toComp.key );
		}
		const REF_ENTRY<T>& operator=(
			const REF_ENTRY<T>& val )
		{
			key = val.key;
			return *this;
		}
	};

	SORTED_ARRAY<REF_ENTRY<T>,2>	m_list;				//	sorted list of entries
	unsigned int					m_TotalRefCount;	//	total number of refs to keys

public:
	//-------------------------------------------------------------------------
	void Init(
		void )
	{
		m_list.Init();
		m_TotalRefCount = 0;
	}
	//-------------------------------------------------------------------------
	void Destroy(
	struct MEMORYPOOL* pool )
	{
		m_list.Destroy( pool );
		m_TotalRefCount = 0;
	}
	//-------------------------------------------------------------------------
	int Add(
	struct MEMORYPOOL* pool,
		const T& toAdd )
	{
		m_TotalRefCount++;

		REF_ENTRY<T> key;
		key.key = toAdd;
		REF_ENTRY<T>* element = m_list.FindExact( key );

		if( !element )
		{
			ASSERT_RETX( m_list.Insert( pool, key ), -1 );
			element = m_list.FindExact( key );
			element->count = 0;
		}

		DBG_ASSERT(element->count >= 0 );
		element->count++;

		return element->count;
	}
	//-------------------------------------------------------------------------
	int Remove(
	struct MEMORYPOOL* pool,
		const T& toRemove )
	{
		REF_ENTRY<T> key;
		key.key = toRemove;
		REF_ENTRY<T>* element = m_list.FindExact( key );

		if( !element )
			return -1;

		m_TotalRefCount--;
		element->count--;
		DBG_ASSERT(element->count >= 0 );

		if( element->count == 0 )
		{
			m_list.Delete(MEMORY_FUNC_FILELINE(pool, key));
			return 0;
		}

		return element->count;
	}
	//-------------------------------------------------------------------------
	unsigned int KeyCount(
		void ) const
	{
		return m_list.Count();
	}
	//-------------------------------------------------------------------------
	unsigned int KeyReferences(
		T & toFind )
	{
		REF_ENTRY<T> key;
		key.key = toFind;
		REF_ENTRY<T>* element = m_list.FindExact( key );

		if(element)
		{
			return element->count;
		}
		return 0;
	}
	//-------------------------------------------------------------------------
	unsigned int ReferenceCount(
		void ) const
	{
		return m_TotalRefCount;
	}
	//-------------------------------------------------------------------------
	const T& operator[](
		unsigned int index ) const
	{
		return m_list[ index ].key;
	}
};


#endif	//	_REFERENCELIST_H_
