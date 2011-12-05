//----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
#ifndef _SAFE_ARRAY_H_
#define _SAFE_ARRAY_H_

#include "array.h"

// Standard SIMPLE_DYNAMIC_ARRAY is not C++ safe.
template <class T>
class SAFE_SIMPLE_DYNAMIC_ARRAY : public SIMPLE_DYNAMIC_ARRAY<T>
{
	protected:
		typedef SIMPLE_DYNAMIC_ARRAY<T> base_type;
		typedef SAFE_SIMPLE_DYNAMIC_ARRAY<T> this_type;
		typedef T value_type;

		void _Init(
			void)
		{
			pool = 0;
			size = 0;
			count = 0;
			list = NULL;
		}

		void _Copy(
			const this_type & rhs)
		{
			// Base class requires at least one allocated space.
#if DEBUG_MEMORY_ALLOCATIONS
			base_type::Init(rhs.pool, max(rhs.Count(), 1), __FILE__, __LINE__);
#else
			base_type::Init(rhs.pool, max(rhs.Count(), 1));
#endif
			for (unsigned ii = 0; ii < rhs.Count(); ++ii)
			{
				Add(rhs[ii]);
			}
		}

		base_type & _AsBase(void)
		{
			return static_cast<base_type &>(*this);
		}

	public:
		~SAFE_SIMPLE_DYNAMIC_ARRAY(
			void)
		{
			Destroy();
		}

		this_type(
			void)
		{
			_Init();
		}

		this_type(
			const this_type & rhs)
		{
			_Init();
			_Copy(rhs);
		}

		this_type & operator=(
			const this_type & rhs)
		{
			Destroy();
			_Copy(rhs);
			return *this;
		}

		int Add(
			const value_type & tValue)
		{
			unsigned int nIdNew = INVALID_ID;
#if DEBUG_MEMORY_ALLOCATIONS
			value_type * pNew = base_type::Add(&nIdNew, __FILE__, __LINE__);
#else
			value_type * pNew = base_type::Add(&nIdNew);
#endif
			if (pNew != 0)
			{
				new(pNew) value_type(tValue);
			}
			return nIdNew;
		}

		BOOL RemoveByIndex(unsigned index)
		{
			ASSERT_RETFALSE(index < count);

			--count;
			for (unsigned i = index; i < count; ++i)
			{
				list[i] = list[i + 1];
			}
			list[count].~value_type();

			return true;
		}

		void Clear(unsigned nToCount = 0)
		{
			while (Count() > nToCount)
			{
				RemoveByIndex(Count() - 1);
			}
		}

		void Destroy(void)
		{
			Clear();
			base_type::Destroy();
		}

		void Swap(this_type & rhs)
		{
			this_type t;
			t._AsBase() = rhs._AsBase();
			rhs._AsBase() = _AsBase();
			_AsBase() = t;
			t.list = NULL;
			t.count = 0;
		}

		value_type * _GetBase(void)
		{
			return list;
		}
};

#endif
