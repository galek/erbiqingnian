
/********************************************************************
	日期:		2012年2月20日
	文件名: 	StlUtil.h
	创建者:		王延兴
	描述:		对STL一些操作的封装	
*********************************************************************/

#pragma once

namespace stl
{
	template <typename Map>
	inline typename Map::mapped_type FindInMap(const Map& mapKeyToValue, const typename Map::key_type& key, typename Map::mapped_type valueDefault)
	{
		typename Map::const_iterator it = mapKeyToValue.find (key);
		if (it == mapKeyToValue.end())
			return valueDefault;
		else
			return it->second;
	}

	template <typename Map>
	inline typename Map::mapped_type& MapInsertOrGet(Map& mapKeyToValue, const typename Map::key_type& key, const typename Map::mapped_type& defValue = typename Map::mapped_type())
	{
		std::pair<typename Map::iterator, bool> iresult = mapKeyToValue.insert(typename Map::value_type(key, defValue));
		return iresult.first->second;
	}

	template <typename Key, typename mapped_type, typename Traits, typename Allocator>
	inline mapped_type& FindInMapRef(std::map<Key, mapped_type, Traits, Allocator>& mapKeyToValue, const Key& key, mapped_type& valueDefault)
	{
		typedef std::map<Key, mapped_type, Traits, Allocator> Map;
		typename Map::iterator it = mapKeyToValue.find (key);
		if (it == mapKeyToValue.end())
			return valueDefault;
		else
			return it->second;
	}

	template <typename Key, typename mapped_type, typename Traits, typename Allocator>
	inline const mapped_type& FindInMapRef(const std::map<Key, mapped_type, Traits, Allocator>& mapKeyToValue, const Key& key, const mapped_type& valueDefault)
	{
		typedef std::map<Key, mapped_type, Traits, Allocator> Map;
		typename Map::const_iterator it = mapKeyToValue.find (key);
		if (it == mapKeyToValue.end())
			return valueDefault;
		else
			return it->second;
	}

	template <class Map,class Vector>
	inline void MapToVector( const Map& theMap,Vector &array )
	{
		array.resize(0);
		array.reserve( theMap.size() );
		for (typename Map::const_iterator it = theMap.begin(); it != theMap.end(); ++it)
		{
			array.push_back( it->second );
		}
	}

	template <class Set,class Vector>
	inline void SetToVector( const Set& theSet,Vector &array )
	{
		array.resize(0);
		array.reserve( theSet.size() );
		for (typename Set::const_iterator it = theSet.begin(); it != theSet.end(); ++it)
		{
			array.push_back( *it );
		}
	}

	template <class Container,class Value>
	inline bool FindAndErase( Container& container,const Value &value )
	{
		typename Container::iterator it = std::find( container.begin(),container.end(),value );
		if (it != container.end())
		{
			container.erase( it );
			return true;
		}
		return false;
	}

	template <class Container>
	inline void FindAndEraseAll(Container& container, const typename Container::value_type&value)
	{
		typename Container::iterator endIter(container.end());
		typename Container::iterator newEndIter(std::remove(container.begin(), endIter, value));

		container.erase(newEndIter, endIter);
	}

	template <class Container,class Key>
	inline bool MemberFindAndErase( Container& container,const Key &key )
	{
		typename Container::iterator it = container.find (key);
		if (it != container.end())
		{
			container.erase( it );
			return true;
		}
		return false;
	}

	template <class Container,class Value>
		inline bool PushBackUnique( Container& container,const Value &value )
	{
		if (std::find(container.begin(),container.end(),value) == container.end())
		{
			container.push_back( value );
			return true;
		}
		return false;
	}

	template <class Container,class Iter>
	inline void PushBackRange( Container& container,Iter begin,Iter end )
	{
		for (Iter it = begin; it != end; ++it)
		{
			container.push_back(*it);
		}
	}

	template <class Container,class Value>
		inline bool Find( Container& container,const Value &value )
	{
		return std::find(container.begin(),container.end(),value) != container.end();
	}

	template <class Iterator,class T>
		inline Iterator BinaryFind(Iterator first,Iterator last,const T& value)
	{
		Iterator it = std::lower_bound(first,last,value);
		return (it == last || value != *it) ? last : it;
	}

	template <class Container,class Value>
		inline bool BinaryInsertUnique( Container& container,const Value &value )
	{
		typename Container::iterator it = std::lower_bound(container.begin(),container.end(),value);
		if (it != container.end())
		{
			if (*it == value)
				return false;
			container.insert( it,value );
		}
		else
			container.insert( container.end(),value );
		return true;
	}
	
	template <class Container,class Value>
		inline bool BinaryErase( Container& container,const Value &value )
	{
		typename Container::iterator it = std::lower_bound(container.begin(),container.end(),value);
		if (it != container.end() && *it == value)
		{
			container.erase(it);
			return true;
		}
		return false;
	}

	struct ContainerObjectDeleter
	{
		template<typename T>
			void operator()(const T* ptr) const
		{
			delete ptr;
		}
	};

	template <class Type>
		inline const char* ConstcharCast( const Type &type )
	{
		return type;
	}

	template <>
	inline const char* ConstcharCast( const std::string &type )
	{
		return type.c_str();
	}

	template <class Type>
	struct LessStrcmp : public std::binary_function<Type,Type,bool> 
	{
		bool operator()( const Type &left,const Type &right ) const
		{
			return strcmp(ConstcharCast(left),ConstcharCast(right)) < 0;
		}
	};

	template <class Type>
	struct LessStricmp : public std::binary_function<Type,Type,bool> 
	{
		bool operator()( const Type &left,const Type &right ) const
		{
			return stricmp(ConstcharCast(left),ConstcharCast(right)) < 0;
		}
	};

	template <class Key>
	class HashSimple
	{
	public:
		enum 
		{
			bucket_size = 4,	// 0 < bucket_size
			min_buckets = 8
		};// min_buckets = 2 ^^ N, 0 < N

		size_t operator()( const Key& key ) const
		{
			return size_t(key);
		};
		bool operator()( const Key& key1,const Key& key2 ) const
		{
			return key1 < key2;
		}
	};

	class HashUint32
	{
	public:
		enum 
		{
			bucket_size = 4,	// 0 < bucket_size
			min_buckets = 8  // min_buckets = 2 ^^ N, 0 < N
		};

		inline size_t operator()( UINT a ) const
		{
			a = (a+0x7ed55d16) + (a<<12);
			a = (a^0xc761c23c) ^ (a>>19);
			a = (a+0x165667b1) + (a<<5);
			a = (a+0xd3a2646c) ^ (a<<9);
			a = (a+0xfd7046c5) + (a<<3);
			a = (a^0xb55a4f09) ^ (a>>16);
			return a;
		};
		bool operator()( UINT key1,UINT key2 ) const
		{
			return key1 < key2;
		}
	};

	template <class Key>
	class HashStrcmp
	{
	public:
		enum
		{
			bucket_size = 4,	// 0 < bucket_size
			min_buckets = 8
		};// min_buckets = 2 ^^ N, 0 < N

		size_t operator()( const Key& key ) const
		{
			unsigned int h = 0; 
			const char *s = ConstcharCast(key);
			for (; *s; ++s) h = 5*h + *(unsigned char*)s;
			return size_t(h);

		};
		bool operator()( const Key& key1,const Key& key2 ) const
		{
			return strcmp(ConstcharCast(key1),ConstcharCast(key2)) < 0;
		}
	};

	template <class Key>
	class HashStricmp
	{
	public:
		enum {	// parameters for hash table
			bucket_size = 4,	// 0 < bucket_size
			min_buckets = 8
		};// min_buckets = 2 ^^ N, 0 < N

		size_t operator()( const Key& key ) const
		{
			unsigned int h = 0; 
			const char *s = ConstcharCast(key);
			for (; *s; ++s) h = 5*h + tolower(*(unsigned char*)s);
			return size_t(h);

		};
		bool operator()( const Key& key1,const Key& key2 ) const
		{
			return stricmp(ConstcharCast(key1),ConstcharCast(key2)) < 0;
		}
	};

#ifdef USE_HASH_MAP
	//C:\Work\Main\Code\SDKs\STLPORT\stlport

	// Support for both Microsoft and SGI kind of hash_map.

#ifdef _STLP_HASH_MAP 
	// STL Port
	template <class _Key,class _Predicate=std::less<_Key> >
	struct HashCompare
	{
		enum
		{	// parameters for hash table
			bucket_size = 4,	// 0 < bucket_size
			min_buckets = 8   // min_buckets = 2 ^^ N, 0 < N
		};

		size_t operator()(const _Key& _Keyval) const
		{
			// return hash value.
			UINT a = _Keyval;
			a = (a+0x7ed55d16) + (a<<12);
			a = (a^0xc761c23c) ^ (a>>19);
			a = (a+0x165667b1) + (a<<5);
			a = (a+0xd3a2646c) ^ (a<<9);
			a = (a+0xfd7046c5) + (a<<3);
			a = (a^0xb55a4f09) ^ (a>>16);
			return a;
		}

		// Less then function.
		bool operator()(const _Key& _Keyval1, const _Key& _Keyval2) const
		{	// test if _Keyval1 ordered before _Keyval2
			_Predicate comp;
			return (comp(_Keyval1, _Keyval2));
		}
	};

	template <class Key,class HashFunc>
	struct StlportHashEqual
	{
		// Equal function.
		bool operator()(const Key& k1, const Key& k2) const
		{
			HashFunc less;
			// !(k1 < k2) && !(k2 < k1)
			return !less(k1,k2) && !less(k2,k1);
		}
	};

	template <class Key,class Value,class HashFunc = HashCompare<Key>, class Alloc = std::allocator< std::pair<Key,Value> > >
	struct hash_map : public std__hash_map<Key,Value,HashFunc,StlportHashEqual<Key,HashFunc>,Alloc>
	{
		hash_map() : std__hash_map<Key,Value,HashFunc,StlportHashEqual<Key,HashFunc>,Alloc>( HashFunc::min_buckets ) {}
	};

	/*
	//template <class Key,class Value,class HashFunc=hash_compare<Key>,class Alloc = std::allocator< std::pair<Key,Value> > >
	template <class Key>
	public hash_map1
	{
	public:
		hash_map1() {};
		//hash_map1() : std__hash_map<Key,Value,HashFunc<Key>,HashFunc<Key>,Alloc>( HashFunc::min_buckets ) {}
	};
	*/
#else
	// MS STL
	using stdext::hash_compare;
	using stdext::hash_map;
#endif

#else // USE_HASH_MAP
		
#endif //USE_HASH_MAP


	template<class T>
	class IntrusiveLinkedListNode
	{
	public:
		IntrusiveLinkedListNode()  { link_to_intrusive_list(static_cast<T*>(this)); }
		// Not virtual by design
		~IntrusiveLinkedListNode() { unlink_from_intrusive_list(static_cast<T*>(this)); }

		static T* get_intrusive_list_root() { return m_root_intrusive; };

		static void link_to_intrusive_list( T *pNode )
		{
			if (m_root_intrusive)
			{
				// Add to the beginning of the list.
				T *head = m_root_intrusive;
				pNode->m_prev_intrusive = 0;
				pNode->m_next_intrusive = head;
				head->m_prev_intrusive = pNode;
				m_root_intrusive = pNode;
			}
			else
			{
				m_root_intrusive = pNode;
				pNode->m_prev_intrusive = 0;
				pNode->m_next_intrusive = 0;
			}
		}
		static void unlink_from_intrusive_list( T *pNode )
		{
			if (pNode == m_root_intrusive) // if head of list.
			{
				m_root_intrusive = pNode->m_next_intrusive;
				if (m_root_intrusive)
				{
					m_root_intrusive->m_prev_intrusive = 0;
				}
			}
			else
			{
				if (pNode->m_prev_intrusive)
				{
					pNode->m_prev_intrusive->m_next_intrusive = pNode->m_next_intrusive;
				}
				if (pNode->m_next_intrusive)
				{
					pNode->m_next_intrusive->m_prev_intrusive = pNode->m_prev_intrusive;
				}
			}
			pNode->m_next_intrusive = 0;
			pNode->m_prev_intrusive = 0;
		}

	public:
		static T* m_root_intrusive;
		T *m_next_intrusive;
		T *m_prev_intrusive;
	};
}

#define DEFINE_INTRUSIVE_LINKED_LIST( Class ) \
	template<> Class* stl::IntrusiveLinkedListNode<Class>::m_root_intrusive = 0;