//----------------------------------------------------------------------------
// memoryallocator_stl.h
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "memoryallocator_i.h"

namespace FSCommon
{

template<class T> 
class CMemoryAllocatorSTL;

template<> 
class CMemoryAllocatorSTL<void> 
{
	public:
	
		typedef void* pointer;
		typedef const void* const_pointer;	  
		typedef void value_type;

		template<class U> 
		struct rebind 
		{ 
			typedef CMemoryAllocatorSTL<U> other; 
		};
};

// class CMemoryAllocatorSTL
//
template<class T> 
class CMemoryAllocatorSTL 
{

	public:
	
		typedef size_t    size_type;
		typedef ptrdiff_t difference_type;
		typedef T*        pointer;
		typedef const T*  const_pointer;
		typedef T&        reference;
		typedef const T&  const_reference;
		typedef T         value_type;
		
		template<class U> 
		struct rebind 
		{ 
			typedef CMemoryAllocatorSTL<U> other; 
		};
		
	public:
		
		CMemoryAllocatorSTL() :
			m_Allocator(NULL)
		{
		}		

		CMemoryAllocatorSTL(IMemoryAllocator* allocator) :
			m_Allocator(allocator)
		{
		}
		
		CMemoryAllocatorSTL(const CMemoryAllocatorSTL& arg)
		{
			m_Allocator = arg.m_Allocator;
		}
		
		template<class U> 
		CMemoryAllocatorSTL(const CMemoryAllocatorSTL<U>& arg)			
		{
			m_Allocator = NULL;
		}
		
		~CMemoryAllocatorSTL()
		{			
		}
		
	public:

		pointer address(reference x) const
		{
			return &x;
		}
		
		const_pointer address(const_reference x) const
		{
			return &x;
		}

		pointer allocate(size_type count, CMemoryAllocatorSTL<void>::const_pointer hint = 0)
		{
			REF(hint);			
			return static_cast<T*>(MALLOC(m_Allocator, (MEMORY_SIZE)(count * sizeof(T))));
		}
		
		void deallocate(pointer p, size_type count)
		{			
			REF(count);
			FREE(m_Allocator, p);
		}

	    size_type max_size() const
	    {
			return size_t(-1) / sizeof(value_type);
		}
		
		void construct(pointer p, const T& val)
		{
			new(static_cast<void*>(p)) T(val);
		}
				
		void destroy(pointer p)
		{
			p->~T();
		}
		
	public:
	
		IMemoryAllocator* m_Allocator;
};

template<typename T, typename U>
inline bool operator==(const CMemoryAllocatorSTL<T>&, const CMemoryAllocatorSTL<U>)
{
	return true;
}

template<typename T, typename U>
inline bool operator!=(const CMemoryAllocatorSTL<T>&, const CMemoryAllocatorSTL<U>)
{	
	return false;
}

 
} // end namespace FSCommon