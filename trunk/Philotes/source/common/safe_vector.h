//----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
// CAUTION! The following code contains an important limitation.
// Because the underlying container type employs a realloc()-like
// allocation when the array expands, which performs a bit-wise copy
// of memory, types that have a non-trivial copy constructor or
// where a bit-wise copy is not equivalent to a member-wise copy
// will NOT be C++-safe.
//
// With some effort it would be possible to overcome this limitation.
//
//----------------------------------------------------------------------------

#ifndef _SAFE_VECTOR_H_
#define _SAFE_VECTOR_H_

#if 1

#include <iterator>	// for random_access_iterator_tag
#include "safe_array.h"

template<class T>
class safe_vector
{
	public:
		typedef T value_type;
		typedef safe_vector<T> this_type;
		typedef size_t size_type;

		class const_iterator
		{
			public:
				typedef std::random_access_iterator_tag iterator_category;
				typedef value_type value_type;
				typedef ptrdiff_t difference_type;
				typedef value_type * pointer;
				typedef value_type & reference;
				const_iterator(void)
				{
				}
				bool operator<(const const_iterator & rhs) const
				{
					return p < rhs.p;
				}
				bool operator==(const const_iterator & rhs) const
				{
					return p == rhs.p;
				}
				bool operator!=(const const_iterator & rhs) const
				{
					return !(*this == rhs);
				}
				const_iterator & operator++(void)	// prefix
				{
					++p;
					return *this;
				}
				const_iterator operator++(int)	// postfix
				{
					return const_iterator(p++);
				}
				const_iterator & operator--(void)	// prefix
				{
					--p;
					return *this;
				}
				const value_type & operator*(void) const
				{
					return *p;
				}
				const value_type * operator->(void) const
				{
					return p;
				}
				const_iterator operator+(difference_type n) const
				{
					return const_iterator(p + n);
				}
				const_iterator & operator+=(difference_type n)
				{
					p += n;
					return *this;
				}
				const_iterator operator-(difference_type n) const
				{
					return const_iterator(p - n);
				}
				difference_type operator-(const const_iterator & rhs) const
				{
					return p - rhs.p;
				}
				void swap(const_iterator & rhs)
				{
					value_type * t = rhs.p;
					rhs.p = p;
					p = t;
				}
			protected:
				const_iterator(value_type * r) : p(r)
				{
				}
			// CHB 2007.03.30 - Hopefully this fixes the compile error.
			// Why doesn't it generate an error for me?
			//private:

			// CML 2007.03.31 - That didn't fix it.  As a temporary workaround, I'm making this member public.
			public:

				value_type * p;
			friend this_type;
		};

		class iterator : public const_iterator
		{
			public:
				//typedef difference_type difference_type;
				value_type & operator*(void) const
				{
					return *p;
				}
				iterator & operator++(void)	// prefix
				{
					++p;
					return *this;
				}
				iterator operator++(int)	// postfix
				{
					return iterator(p++);
				}
				iterator & operator--(void)	// prefix
				{
					--p;
					return *this;
				}
				iterator operator+(size_type n) const
				{
					return iterator(p + n);
				}
				iterator operator-(size_type n) const
				{
					return iterator(p - n);
				}
				difference_type operator-(const const_iterator & rhs) const
				{
					return p - rhs.p;
				}
			protected:
				typedef const_iterator base_type;
				iterator(value_type * r) : base_type(r)
				{
				}
			friend this_type;
		};

	private:
		value_type * _GetBase(void) const
		{
			return const_cast<this_type *>(this)->m_Array._GetBase();
		}

	public:
		const_iterator begin(void) const
		{
			return const_iterator(_GetBase());
		}

		iterator begin(void)
		{
			return iterator(_GetBase());
		}

		const_iterator end(void) const
		{
			return const_iterator(_GetBase() + size());
		}

		iterator end(void)
		{
			return iterator(_GetBase() + size());
		}

		size_type size(void) const
		{
			return m_Array.Count();
		}

		value_type & operator[](size_type nIndex)
		{
			return m_Array[static_cast<unsigned>(nIndex)];
		}

		const value_type & operator[](size_type nIndex) const
		{
			return m_Array[static_cast<unsigned>(nIndex)];
		}

		bool empty(void) const
		{
			return size() < 1;
		}

		void push_back(const value_type & v)
		{
			m_Array.Add(v);
		}

		value_type & front(void)
		{
			#if ISVERSION(DEVELOPMENT)
			ASSERT(!empty());
			#endif
			return m_Array[0];
		}

		const value_type & front(void) const
		{
			return const_cast<this_type *>(this)->front();
		}

		value_type & back(void)
		{
			#if ISVERSION(DEVELOPMENT)
			ASSERT(!empty());
			#endif
			return m_Array[m_Array.Count() - 1];
		}

		const value_type & back(void) const
		{
			return const_cast<this_type *>(this)->back();
		}

		iterator insert(iterator iBefore, const value_type & v)
		{
			size_type n = iBefore.p - _GetBase();
			size_type nOriginalSize = size();
			#if ISVERSION(DEVELOPMENT)
			ASSERT(n <= nOriginalSize);	// <= intentional, since it's valid to insert at the end after the last element
			#endif
			push_back(v);	// we need to add a constructed element
			if (n < nOriginalSize)
			{
				for (size_type i = nOriginalSize; i > n; --i)
				{
					m_Array[static_cast<unsigned>(i)] = m_Array[static_cast<unsigned>(i - 1)];
				}
				m_Array[static_cast<unsigned>(n)] = v;
			}
			return begin() + n;
		}

		void insert(iterator iBefore, const_iterator iStart, const_iterator iEnd)
		{
			// Right now only insertion at the end is supported.
			#if ISVERSION(DEVELOPMENT)
			ASSERT(iBefore == end());
			#endif
			for (const_iterator i = iStart; i != iEnd; ++i)
			{
				push_back(*i);
			}
		}

		void erase(iterator i)
		{
			size_type n = i.p - _GetBase();
			#if ISVERSION(DEVELOPMENT)
			ASSERT(n < size());
			#endif
			m_Array.RemoveByIndex(static_cast<unsigned>(n));
		}

		void clear(void)
		{
			m_Array.Clear();
		}

		void reserve(size_type /*n*/)
		{
			// reserve not supported
		}

		void swap(this_type & rhs)
		{
			m_Array.Swap(rhs.m_Array);
		}

		this_type(void)
		{
			_Init();
		}

		this_type(const this_type & rhs) : m_Array(rhs.m_Array)
		{
		}

		this_type(const_iterator iBegin, const_iterator iEnd)
		{
			_Init();
			for (const_iterator i = iBegin; i != iEnd; ++i)
			{
				push_back(*i);
			}
		}

		this_type & operator=(const this_type & rhs)
		{
			if (this != &rhs)
			{
				m_Array = rhs.m_Array;
			}
			return *this;
		}

		~safe_vector(void)
		{
			clear();
		}

	protected:
		void _Init(void)
		{
			ArrayInit(m_Array, NULL, 1);
		}

		typedef SAFE_SIMPLE_DYNAMIC_ARRAY<value_type> array_type;
		array_type m_Array;
};

#else

#include <vector>
#define safe_vector std::vector

#endif

#endif
