//----------------------------------------------------------------------------
//	fasttraversehash.h
//  Copyright 2006, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _FASTTRAVERSEHASH_H_
#define _FASTTRAVERSEHASH_H_


//----------------------------------------------------------------------------
//	CFastTraverseHash
//	Just a hash that also inserts into a doubly linked list. Added T* put at end of list.
//	Exactly like HASH found in hash.h, but provides very fast traversal over
//		large tables that may be sparsley populated. GetFirst and GetNext
//		are always constant time.
//	Requires the same members as HASH plus [ T* traverse[2] ] 
//	WARNING: make sure you know what you're doing if you insert/remove nodes
//		while you're iterating
//-----------------------------------------------------------------------------
template <class T, class ID, unsigned int SIZE>
class CFastTraverseHash
{
private:
	static const int	PREV	=	0;
	static const int	NEXT	=	1;

	HASH<T, ID, SIZE>	m_table;
	T*					m_head;

	void UnlinkNode(
		T* toRemove )
	{
		toRemove->traverse[NEXT]->traverse[PREV] = toRemove->traverse[PREV];
		toRemove->traverse[PREV]->traverse[NEXT] = toRemove->traverse[NEXT];

		if( m_head == toRemove ){
			m_head = ( toRemove->traverse[NEXT] == toRemove ) ? NULL : toRemove->traverse[NEXT];
		}
	}

public:
	
	void Init(
		void)
	{
		m_head = NULL;
		m_table.Init();
	}

	void Destroy(
		MEMORYPOOL * pool)
	{
		m_table.Destroy( pool );
	}

	void Destroy(
		MEMORYPOOL * pool,
		void fnDelete(MEMORYPOOL * pool, T * a))
	{
		m_table.Destroy( pool, fnDelete );
	}

	T * Add(
		MEMORYPOOL * pool,
		const ID & id)
	{
		T* toRet = m_table.Add( pool, id );
		if( toRet ){
			if( m_head ) {
				toRet->traverse[NEXT] = m_head;
				toRet->traverse[PREV] = m_head->traverse[PREV];
				m_head->traverse[PREV]->traverse[NEXT] = toRet;
				m_head->traverse[PREV] = toRet;
			} else {
				toRet->traverse[NEXT] = toRet->traverse[PREV] = toRet;
				m_head = toRet;
			}
		}
		return toRet;
	}

	T * Get(
		const ID & id)
	{
		return m_table.Get( id );
	}

	BOOL Remove(
		const ID & id)
	{
		T* toRemove = m_table.Get( id );
		if( toRemove )
		{
			UnlinkNode( toRemove );
			return m_table.Remove( id );
		}
		return FALSE;
	}

	BOOL Remove(
		MEMORYPOOL * pool,
		const ID & id,
		void fnDelete(MEMORYPOOL * pool, T & a))
	{
		T* toRemove = m_table.Get( id );
		if( toRemove )
		{
			UnlinkNode( toRemove );
			return m_table.Remove( pool, id, fnDelete );
		}
		return FALSE;
	}

	T* GetFirst(
		void )
	{
		return m_head;
	}

	//----------------------------------------------------------------------
	//	NOTE: not safe for NULL itr parameter or empty list with previously aquired node
	//----------------------------------------------------------------------
	T* GetNext(
		T* itr )
	{
		DBG_ASSERT_RETNULL( itr );
		DBG_ASSERT_RETNULL( m_head );

		//	because we don't deal w/ null itrs, the only way we can get hear is with a non-empty list, => safe to de-ref head.
		return ( itr == m_head->traverse[PREV] ) ? NULL : itr->traverse[NEXT];
	}
};


#endif	//	_FASTTRAVERSEHASH_H_
