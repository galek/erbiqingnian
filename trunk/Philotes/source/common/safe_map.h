//----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------

#ifndef _SAFE_MAP_H_
#define _SAFE_MAP_H_

#if 1

#include "hash.h"
#include <utility>	// for std::pair

// safe_map_traits describes map features,
// in particular the mapping from key type
// to CHash id.
template<class K>
struct safe_map_traits;

// Traits for DWORD key.
template<>
struct safe_map_traits<DWORD>
{
	static int Key2Id(DWORD n)
	{
		return n;
	}
};

// Simulates the Standard C++ Library std::map<> using CHash.
template<class K, class T>
class safe_map
{
	public:
		typedef safe_map<K, T> this_type;
		typedef K key_type;
		typedef T value_type;
		typedef std::pair<K, T> pair_type;
		typedef unsigned size_type;

	protected:
		struct node_type
		{
			node_type * pNext;
			int nId;
			pair_type m_Data;
		};
		typedef CHash<node_type> hash_type;

		static int _Key2Id(const key_type & Key)
		{
			return safe_map_traits<key_type>::Key2Id(Key);
		}

		void _Init(MEMORYPOOL* pool)
		{
			m_Pool = pool;
			HashInit(m_Hash, pool, 512);
		}

		node_type * _Insert(const pair_type & p)
		{
			node_type * pNew = HashAdd(m_Hash, _Key2Id(p.first));
			if (pNew != 0)
			{
				new(&pNew->m_Data) pair_type(p);
			}
			return pNew;
		}

	public:
		class const_iterator
		{
			public:
				const pair_type * operator->(void) const
				{
					return &m_pNode->m_Data;
				}
				bool operator==(const const_iterator & r) const
				{
					return m_pNode == r.m_pNode;
				}
				bool operator!=(const const_iterator & r) const
				{
					return !(*this == r);
				}
				const_iterator & operator++(void)
				{
					m_pNode = HashGetNext(*m_pHash, m_pNode);
					return *this;
				}
				const_iterator(void)
				{
				}
			protected:
				const_iterator(hash_type * pHash, node_type * pNode) : m_pHash(pHash), m_pNode(pNode)
				{
				}

				hash_type * m_pHash;
				node_type * m_pNode;

			friend class this_type;
		};

		class iterator : public const_iterator
		{
			protected:
				typedef const_iterator base_type;

				iterator(hash_type * pHash, node_type * pNode) : base_type(pHash, pNode)
				{
				}

			public:
				pair_type * operator->(void) const
				{
					return const_cast<pair_type *>(base_type::operator->());
				}
				iterator(void)
				{
				}

			friend class this_type;
		};

	protected:
		void _Copy(const this_type & rhs)
		{
			for (const_iterator i = rhs.begin(); i != rhs.end(); ++i)
			{
				insert(i.m_pNode->m_Data);
			}
		}

	public:
		size_type size(void) const
		{
			return m_Hash.GetCount();
		}

		const_iterator begin(void) const
		{
			return const_iterator(const_cast<hash_type *>(&m_Hash), HashGetFirst(const_cast<hash_type &>(m_Hash)));
		}

		iterator begin(void)
		{
			return iterator(&m_Hash, HashGetFirst(m_Hash));
		}

		const_iterator end(void) const
		{
			return const_iterator(0, 0);
		}

		iterator end(void)
		{
			return iterator(0, 0);
		}

		iterator find(const key_type & Key)
		{
			node_type * p = HashGet(m_Hash, _Key2Id(Key));
			return iterator(&m_Hash, p);
		}

		value_type & operator[](const key_type & Key)
		{
			node_type * p = HashGet(m_Hash, _Key2Id(Key));
			if (p == 0)
			{
				p = _Insert(std::make_pair(Key, value_type()));
			}
			return p->m_Data.second;
		}

		void insert(const pair_type & p)
		{
			_Insert(p);
		}

		void erase(iterator i)
		{
			int nId = i.m_pNode->nId;
			i.m_pNode->m_Data.~pair_type();
			HashRemove(m_Hash, nId);
		}

		void clear(void)
		{
			while (begin() != end())
			{
				erase(begin());
			}
			m_Hash.Clear();
		}

		this_type(MEMORYPOOL* pool = NULL)
		{
			_Init(pool);
		}

		this_type(const this_type & rhs)
		{
			_Init(rhs.m_Pool);
			_Copy(rhs);
		}

		this_type & operator=(const this_type & rhs)
		{
			clear();
			_Copy(rhs);
			return *this;
		}

		~safe_map(void)
		{
			clear();
			m_Hash.Free();
		}

		MEMORYPOOL* m_Pool;
		
	private:
		hash_type m_Hash;
};

#else

#include <map>
#define safe_map std::map

#endif

#endif
