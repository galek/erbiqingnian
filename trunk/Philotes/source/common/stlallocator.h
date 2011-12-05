//----------------------------------------------------------------------------
// stlallacator.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _STLALLOCATOR_H_
#define _STLALLOCATOR_H_


//----------------------------------------------------------------------------
// STL ALLOCATOR WRAPPING THE MEMORYPOOL ALLOCATION
//----------------------------------------------------------------------------

template <class T> class mempool_allocator;

//	specialize for void:
template <> class mempool_allocator<void>
{
public:
	typedef void *			pointer;
	typedef const void *	const_pointer;
	typedef void			value_type;
	template <class U> struct rebind
	{
		typedef mempool_allocator<U> other;
	};
};

template <class T> class mempool_allocator
{
private:
	template <class T>
	friend class mempool_allocator;
	template <class T1, class T2>
	friend bool operator==(
		const mempool_allocator<T1>&,
		const mempool_allocator<T2>& ) throw();

	template <class T1, class T2>
	friend bool operator!=(
		const mempool_allocator<T1>&,
		const mempool_allocator<T2>& ) throw();

	struct MEMORYPOOL * m_pool;

	static inline void destruct(char* sz)		{}
	static inline void destruct(wchar_t* wsz)	{}
	template<typename T>
	static inline void destruct(
		T* t)
	{
		UNREFERENCED_PARAMETER(t); //huh ?
		t->~T();
	}

public:
	typedef size_t			size_type;
	typedef ptrdiff_t		difference_type;
	typedef T *				pointer;
	typedef const T *		const_pointer;
	typedef T &				reference;
	typedef const T &		const_reference;
	typedef T				value_type;
	template <class U> struct rebind
	{
		typedef mempool_allocator<U> other;
	};

	mempool_allocator() throw()
	{
		m_pool = NULL;
	}
	mempool_allocator(
		struct MEMORYPOOL * pool ) throw()
	{
		m_pool = pool;
	}
	mempool_allocator(
		const mempool_allocator & other ) throw()
	{
		m_pool = other.m_pool;
	}
	template <class U> mempool_allocator(
		const mempool_allocator<U> & other ) throw()
	{
		m_pool = other.m_pool;
	}
	~mempool_allocator() throw()
	{
		m_pool = NULL;
	}

	pointer address(
		reference x ) const
	{
		return &x;
	}
	const_pointer address(
		const_reference x) const
	{
		return &x;
	}

	pointer allocate(
		size_type n,
		mempool_allocator<void>::const_pointer hint = 0)
	{
		UNREFERENCED_PARAMETER(hint);
		return static_cast<T*>(MALLOC( m_pool, unsigned int(sizeof(T)*n) ));
	}
	void deallocate(
		pointer p,
		size_type n)
	{
		UNREFERENCED_PARAMETER(n);
		FREE( m_pool, p );
	}
	size_type max_size() const throw()
	{
		return (size_type)(0xFFFFFFFF/sizeof(T));
	}

	void construct(
		pointer p,
		const T & val)
	{
		new(static_cast<void*>(p)) T(val);
	}
	void destroy(
		pointer p)
	{
		mempool_allocator::destruct(p);
	}
};


//----------------------------------------------------------------------------
// HELPER OPERATORS
//----------------------------------------------------------------------------
template <class T1, class T2>
bool operator==(
	const mempool_allocator<T1>& alloc1,
	const mempool_allocator<T2>& alloc2 ) throw()
{
	return (alloc1.m_pool == alloc2.m_pool);
}

template <class T1, class T2>
bool operator!=(
	const mempool_allocator<T1>& alloc1,
	const mempool_allocator<T2>& alloc2 ) throw()
{
	return (alloc1.m_pool != alloc2.m_pool);
}


//----------------------------------------------------------------------------------------------------------------------
// string_mp, wstring_mp: std::string and std::wstring preconfigured to use the MEMORYPOOL allocator.

#include <string>
typedef std::basic_string<char, std::char_traits<char>, mempool_allocator<char>> string_mp;
typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, mempool_allocator<wchar_t>> wstring_mp;


//----------------------------------------------------------------------------------------------------------------------
// vector_mp: std::vector configured to use the MEMORYPOOL allocator.  Define: vector_mp<TYPE>::type.

#include <vector>
// can't directly typedef a template (yet!), so we have to use this workaround
template<typename VALUE_T>
class vector_mp
{
public:
	typedef std::vector<VALUE_T, mempool_allocator<VALUE_T>> type;
};


//----------------------------------------------------------------------------------------------------------------------
// deque_mp: std::deque configured to use the MEMORYPOOL allocator.  Define: deque_mp<TYPE>::type.

#include <deque>
// can't directly typedef a template (yet!), so we have to use this workaround
template<typename VALUE_T>
class deque_mp
{
public:
	typedef std::deque<VALUE_T, mempool_allocator<VALUE_T>> type;
};


//----------------------------------------------------------------------------------------------------------------------
// list_mp: std::list configured to use the MEMORYPOOL allocator.  Define: list_mp<TYPE>::type.

#include <list>
// can't directly typedef a template (yet!), so we have to use this workaround
template<typename VALUE_T>
class list_mp
{
public:
	typedef std::list<VALUE_T, mempool_allocator<VALUE_T>> type;
};


//----------------------------------------------------------------------------------------------------------------------
// map_mp: std::map configured to use the MEMORYPOOL allocator.  
// Define: map_mp<KEY_T, VALUE_T>::type Map
// Construct: Map(map_mp<KEY_T, VALUE_T>::type::key_compare(), pMemoryPool)

#include <map>
// can't directly typedef a template (yet!), so we have to use this workaround
template<typename KEY_T, typename VALUE_T, typename LESS_T = std::less<KEY_T>>
class map_mp
{
public:
	typedef std::map<KEY_T, VALUE_T, LESS_T, mempool_allocator<std::pair<const KEY_T, VALUE_T>>> type;
};


//----------------------------------------------------------------------------------------------------------------------
// multi_map_mp: std::multimap configured to use the MEMORYPOOL allocator.  
// Define: multimap_mp<KEY_T, VALUE_T>::type MultiMap
// Construct: MultiMap(map_mp<KEY_T, VALUE_T>::type::key_compare(), pMemoryPool)

#include <map>
// can't directly typedef a template (yet!), so we have to use this workaround
template<typename KEY_T, typename VALUE_T, typename LESS_T = std::less<KEY_T>>
class multimap_mp
{
public:
	typedef std::multimap<KEY_T, VALUE_T, LESS_T, mempool_allocator<std::pair<const KEY_T, VALUE_T>>> type;
};


//----------------------------------------------------------------------------------------------------------------------
// set_mp: std::set configured to use the MEMORYPOOL allocator.  
// Define: set_mp<TYPE>::type Set
// Construct: Set(set_mp<TYPE>::type::key_compare(), pMemoryPool)

#include <set>
// can't directly typedef a template (yet!), so we have to use this workaround
template<typename KEY_T, typename LESS_T = std::less<KEY_T>>
class set_mp
{
public:
	typedef std::set<KEY_T, LESS_T, mempool_allocator<KEY_T>> type;
};


//----------------------------------------------------------------------------------------------------------------------
// hash_map_mp: stdext::hash_map configured to use the MEMORYPOOL allocator.  
// Define: hash_map_mp<KEY_T, VALUE_T>::type HashMap
// Construct: HashMap(hash_map_mp<KEY_T, VALUE_T>::type::key_compare(), pMemoryPool)

#include <hash_map>
// can't directly typedef a template (yet!), so we have to use this workaround
template<typename KEY_T, typename VALUE_T, typename HASH_COMPARE_T = stdext::hash_compare<KEY_T, std::less<KEY_T>>>
class hash_map_mp
{
public:
	typedef stdext::hash_map<KEY_T, VALUE_T, HASH_COMPARE_T, mempool_allocator<std::pair<const KEY_T, VALUE_T>>> type;
};


//----------------------------------------------------------------------------------------------------------------------
// hash_set_mp: stdext::hash_set configured to use the MEMORYPOOL allocator.  
// Define: hash_set_mp<TYPE>::type HashSet
// Construct: HashSet(hash_set_mp<TYPE>::type::key_compare(), pMemoryPool)

#include <hash_set>
// can't directly typedef a template (yet!), so we have to use this workaround
template<typename KEY_T, typename HASH_COMPARE_T = stdext::hash_compare<KEY_T, std::less<KEY_T>>>
class hash_set_mp
{
public:
	typedef stdext::hash_set<KEY_T, HASH_COMPARE_T, mempool_allocator<KEY_T>> type;
};


#endif	//	_STLALLOCATOR_H_
