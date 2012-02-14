#pragma once
//------------------------------------------------------------------------------
/**
    @class Set

	Philotes Set class (a set container like std::set).

	Internally the Set is like a sorted Array.

	Set can not contain duplicate keys.

	IMPORTANT: 
	If many insertions are performed at once, it may be damn beneficial to call
	BeginBulkInsert() before, and EndBulkInsert() after inserting these elements. 

	The default capacity of set is 16;

	Created by Li
	Last modified : 2011-11-24 12:04:11

	(C) 2012 PhiloLabs
*/

#include "core/types.h"

//------------------------------------------------------------------------------
namespace Philo
{
template<class TYPE> class Set
{
public:
    /// define iterator
    typedef TYPE* Iterator;

    /// constructor with default parameters
    Set();
    /// constuctor with initial size and grow size
    Set(SizeT initialCapacity);
    /// copy constructor
    Set(const Set<TYPE>& rhs);
    /// destructor
    ~Set();

    /// assignment operator
    void operator=(const Set<TYPE>& rhs);
    /// [] operator
    TYPE& operator[](IndexT index) const;
    /// equality operator
    bool operator==(const Set<TYPE>& rhs) const;
    /// inequality operator
    bool operator!=(const Set<TYPE>& rhs) const;

    /// get number of elements in Set
    SizeT Size() const;
    /// get overall allocated size of Set in number of elements
    SizeT Capacity() const;
    /// return true if Set is empty
    bool IsEmpty() const;
    /// erase element pointed to by iterator, return the next valid iterator
    Iterator Erase(Iterator iter);
	/// erase an element
	void Erase(const TYPE& elm);
	/// erase element at index
	void EraseAtIndex(IndexT index);
	/// begin a bulk insert (set will be sorted at end)
	void BeginBulkInsert();
    /* insert element into set, returns an iterator pointing to either the newly
	 inserted element or to the element that already had its same value in the set. */
    Iterator Insert(const TYPE& elm);
	/// end a bulk insert (this will sort the internal array)
	void EndBulkInsert();
    /// clear set (calls destructors)
    void Clear();
    /// return iterator to beginning of set
    Iterator Begin() const;
    /// return iterator to end of set
    Iterator End() const;
    /// find identical element in set, return iterator, or End if not found
    Iterator Find(const TYPE& elm) const;
	/// return true if element exists in the set
	bool Contains(const TYPE& elm) const;

private:
	/// append element to end of Set
	void Append(const TYPE& elm);
	/// insert element before element at index
	void InsertBefore(IndexT index, const TYPE& elm);
    /// destroy an element (call destructor without freeing memory)
    void Destroy(TYPE* elm);
    /// copy content
    void Copy(const Set<TYPE>& src);
    /// delete content
    void Delete();
    /// grow Set
    void Grow();
    /// grow Set to target size
    void GrowTo(SizeT newCapacity);
    /// move elements, grows Set if needed
    void Move(IndexT fromIndex, IndexT toIndex);

    static const SizeT MinGrowSize = 16;
    static const SizeT MaxGrowSize = 65536; // FIXME: big grow size needed for mesh tools
    SizeT capacity;                         // number of elements allocated
    SizeT size;                             // number of elements in Set
    TYPE* elements;                         // pointer to element Set
	bool inBulkInsert;
};


//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Set<TYPE>::Set() :
    capacity(0),
    size(0),
    elements(0),
	inBulkInsert(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Set<TYPE>::Set(SizeT initialCapacity) :
    capacity(initialCapacity),
    size(0),
	inBulkInsert(false)
{
    if (this->capacity > 0)
    {
        this->elements = ph_new_array(TYPE, this->capacity);
    }
    else
    {
        this->elements = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Set<TYPE>::Set(const Set<TYPE>& rhs) :
	capacity(0),
	size(0),
	elements(0)
{
	this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Set<TYPE>::Copy(const Set<TYPE>& src)
{
    #if PH_BOUNDSCHECKS
    ph_assert(0 == this->elements);
    #endif

    this->capacity = src.capacity;
    this->size = src.size;
    if (this->capacity > 0)
    {
        this->elements = ph_new_array(TYPE, this->capacity);
        IndexT i;
        for (i = 0; i < this->size; ++i)
        {
            this->elements[i] = src.elements[i];
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Set<TYPE>::Delete()
{
    this->capacity = 0;
    this->size = 0;
    if (this->elements)
    {
        ph_delete_array(this->elements);
        this->elements = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Set<TYPE>::Destroy(TYPE* elm)
{
    elm->~TYPE();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
Set<TYPE>::~Set()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void 
Set<TYPE>::operator=(const Set<TYPE>& rhs)
{
    if (this != &rhs)
    {
        if ((this->capacity > 0) && (rhs.size <= this->capacity))
        {
            // source Set fits into our capacity, copy in place
            ph_assert(0 != this->elements);
            IndexT i;
            for (i = 0; i < rhs.size; ++i)
            {
                this->elements[i] = rhs.elements[i];
            }

            // properly destroy remaining original elements
            for (; i < this->size; ++i)
            {
                this->Destroy(&(this->elements[i]));
            }

            this->size = rhs.size;
        }
        else
        {
            // source Set doesn't fit into our capacity, need to reallocate
            this->Delete();
            this->Copy(rhs);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Set<TYPE>::GrowTo(SizeT newCapacity)
{
    TYPE* newSet = ph_new_array(TYPE, newCapacity);
    if (this->elements)
    {
        // copy over contents
        IndexT i;
        for (i = 0; i < this->size; ++i)
        {
            newSet[i] = this->elements[i];
        }

        // discard old Set and update contents
        ph_delete_array(this->elements);
    }
    this->elements  = newSet;
    this->capacity = newCapacity;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Set<TYPE>::Grow()
{
    SizeT growToSize;
    if (0 == this->capacity)
    {
        growToSize = 16;//default capacity
    }
    else
    {
        // grow by half of the current capacity, but never more then MaxGrowSize
        SizeT growBy = this->capacity >> 1;
        if (growBy == 0)
        {
            growBy = MinGrowSize;
        }
        else if (growBy > MaxGrowSize)
        {
            growBy = MaxGrowSize;
        }
        growToSize = this->capacity + growBy;
    }
    this->GrowTo(growToSize);
}

//------------------------------------------------------------------------------
/**
    30-Jan-03   floh    serious bugfixes!
	07-Dec-04	jo		bugfix: neededSize >= this->capacity => neededSize > capacity	
*/
template<class TYPE> void
Set<TYPE>::Move(IndexT fromIndex, IndexT toIndex)
{
    #if PH_BOUNDSCHECKS
    ph_assert(this->elements);
    ph_assert(fromIndex < this->size);
    #endif

    // nothing to move?
    if (fromIndex == toIndex)
    {
        return;
    }

    // compute number of elements to move
    SizeT num = this->size - fromIndex;

    // check if Set needs to grow
    SizeT neededSize = toIndex + num;
    while (neededSize > this->capacity)
    {
        this->Grow();
    }

    if (fromIndex > toIndex)
    {
        // this is a backward move
        IndexT i;
        for (i = 0; i < num; ++i)
        {
            this->elements[toIndex + i] = this->elements[fromIndex + i];
        }

        // destroy remaining elements
        for (i = (fromIndex + i) - 1; i < this->size; ++i)
        {
            this->Destroy(&(this->elements[i]));
        }
    }
    else
    {
        // this is a forward move
        int i;  // NOTE: this must remain signed for the following loop to work!!!
        for (i = num - 1; i >= 0; --i)
        {
            this->elements[toIndex + i] = this->elements[fromIndex + i];
        }

        // destroy freed elements
        for (i = int(fromIndex); i < int(toIndex); ++i)
        {
            this->Destroy(&(this->elements[i]));
        }
    }

    // adjust Set size
    this->size = toIndex + num;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Set<TYPE>::Append(const TYPE& elm)
{
    // grow allocated space if exhausted
    if (this->size == this->capacity)
    {
        this->Grow();
    }
    #if PH_BOUNDSCHECKS
    ph_assert(this->elements);
    #endif
    this->elements[this->size++] = elm;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> SizeT
Set<TYPE>::Size() const
{
    return this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> SizeT
Set<TYPE>::Capacity() const
{
    return this->capacity;
}

//------------------------------------------------------------------------------
/**
    Access an element. This method will NOT grow the Set, and instead do
    a range check, which may throw an assertion.
*/
template<class TYPE> TYPE&
Set<TYPE>::operator[](IndexT index) const
{
    #if PH_BOUNDSCHECKS
    ph_assert(this->elements && (index < this->size));
    #endif
    return this->elements[index];
}

//------------------------------------------------------------------------------
/**
    The equality operator returns true if all elements are identical. The
    TYPE class must support the equality operator.
*/
template<class TYPE> bool
Set<TYPE>::operator==(const Set<TYPE>& rhs) const
{
    if (rhs.Size() == this->size)
    {
        IndexT i;
        SizeT num = this->size;
        for (i = 0; i < num; ++i)
        {
            if (!(this->elements[i] == rhs.elements[i]))
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    The inequality operator returns true if at least one element in the 
    Set is different, or the Set sizes are different.
*/
template<class TYPE> bool
Set<TYPE>::operator!=(const Set<TYPE>& rhs) const
{
    return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> bool 
Set<TYPE>::IsEmpty() const
{
    return (this->size == 0);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Set<TYPE>::EraseAtIndex(IndexT index)
{
    #if PH_BOUNDSCHECKS
    ph_assert(this->elements && (index < this->size));
    #endif
    if (index == (this->size - 1))
    {
        // special case: last element
        this->Destroy(&(this->elements[index]));
        --(this->size);
    }
    else
    {
        this->Move(index + 1, index);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename Set<TYPE>::Iterator
Set<TYPE>::Erase(typename Set<TYPE>::Iterator iter)
{
    #if PH_BOUNDSCHECKS
		ph_assert(!this->inBulkInsert && this->elements && (iter >= this->elements) && (iter < (this->elements + this->size)));
    #endif

    this->EraseAtIndex(IndexT(iter - this->elements));
    return iter;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Set<TYPE>::Erase(const TYPE& elm)
{
	#if PH_BOUNDSCHECKS
		ph_assert(!this->inBulkInsert);
	#endif

	TYPE* result = this->Find(elm);
	if ( result != this->elements + this->size)
	{
		this->EraseAtIndex(IndexT(result - this->elements));
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
Set<TYPE>::InsertBefore(IndexT index, const TYPE& elm)
{
    #if PH_BOUNDSCHECKS
    ph_assert(index <= this->size);
    #endif
    if (index == this->size)
    {
        // special case: append element to back
        this->Append(elm);
    }
    else
    {
        this->Move(index, index + 1);
        this->elements[index] = elm;
    }
}

//------------------------------------------------------------------------------
/**
    The current implementation of this method does not shrink the 
    preallocated space. It simply sets the Set size to 0.
*/
template<class TYPE> void
Set<TYPE>::Clear()
{
    IndexT i;
    for (i = 0; i < this->size; ++i)
    {
        this->Destroy(&(this->elements[i]));
    }
    this->size = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> typename Set<TYPE>::Iterator
Set<TYPE>::Begin() const
{
	#if PH_BOUNDSCHECKS
		ph_assert(!this->inBulkInsert);
	#endif

    return this->elements;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> typename Set<TYPE>::Iterator
Set<TYPE>::End() const
{
    return this->elements + this->size;
}

//------------------------------------------------------------------------------
/**
    Does a binary search on the set, returns the Iterator of the identical
    element, or End if not found
*/
template<class TYPE>
typename Set<TYPE>::Iterator
Set<TYPE>::Find(const TYPE& elm) const
{
	#if PH_BOUNDSCHECKS
		ph_assert(!this->inBulkInsert);
	#endif

	SizeT num = this->size;
	if (num > 0)
	{
		IndexT half;
		IndexT lo = 0;
		IndexT hi = num - 1;
		IndexT mid;
		while (lo <= hi) 
		{
			if (0 != (half = num/2)) 
			{
				mid = lo + ((num & 1) ? half : (half - 1));
				if (elm < this->elements[mid])
				{
					hi = mid - 1;
					num = num & 1 ? half : half - 1;
				} 
				else if (elm > this->elements[mid]) 
				{
					lo = mid + 1;
					num = half;
				} 
				else
				{
					return this->elements + mid;
				}
			} 
			else if (0 != num) 
			{
				if (elm != this->elements[lo])
				{
					return this->elements + this->size;
				}
				else      
				{
					return this->elements + lo;
				}
			} 
			else 
			{
				break;
			}
		}
	}
	return this->elements + this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
bool
Set<TYPE>::Contains(const TYPE& elm) const
{
	return ( this->elements + this->size != this->Find(elm) );
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Set<TYPE>::BeginBulkInsert()
{
#if PH_BOUNDSCHECKS
	ph_assert(!this->inBulkInsert);
#endif
	this->inBulkInsert = true;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
typename Set<TYPE>::Iterator
Set<TYPE>::Insert(const TYPE& elm)
{
    SizeT num = this->size;
    if (num == 0 || this->inBulkInsert)
    {
        // Set is currently empty
        this->Append(elm);
        return this->elements + this->size - 1;
    }
    else
    {
        IndexT half;
        IndexT lo = 0;
	    IndexT hi = num - 1;
	    IndexT mid;
        while (lo <= hi) 
        {
            if (0 != (half = num/2)) 
            {
                mid = lo + ((num & 1) ? half : (half - 1));
                if (elm < this->elements[mid])
                {
                    hi = mid - 1;
                    num = num & 1 ? half : half - 1;
                } 
                else if (elm > this->elements[mid]) 
                {
                    lo = mid + 1;
                    num = half;
                } 
                else
                {
                    // element already exists at [mid]
					return this->elements + mid;
                }
            } 
            else if (0 != num) 
            {
                if (elm < this->elements[lo])
                {
                    this->InsertBefore(lo, elm);
                    return this->elements + lo;
                }
                else if (elm > this->elements[lo])
                {
                    this->InsertBefore(lo + 1, elm);
                    return this->elements + lo + 1;
                }
                else      
                {
                    // element already exists at [low]
					return this->elements + mid;
                }
            } 
            else 
            {
                #if PH_BOUNDSCHECKS
                ph_assert(0 == lo);
                #endif
                this->InsertBefore(lo, elm);
                return this->elements + lo;
            }
        }
        if (elm < this->elements[lo])
        {
            this->InsertBefore(lo, elm);
            return this->elements + lo;
        }
        else if (elm > this->elements[lo])
        {
            this->InsertBefore(lo + 1, elm);
            return this->elements + lo + 1;
        }
        else
        {
            // can't happen(?)
        }
    }
    // can't happen
    ph_error("Set::Insert : Can't happen!");
    return this->elements + this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
Set<TYPE>::EndBulkInsert()
{
#if PH_BOUNDSCHECKS
	ph_assert(this->inBulkInsert);
#endif

	std::sort(this->elements, this->elements + this->size);
	this->inBulkInsert = false;
}

} // namespace Philo
//------------------------------------------------------------------------------
