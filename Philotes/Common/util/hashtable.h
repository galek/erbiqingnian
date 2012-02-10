#pragma once
//------------------------------------------------------------------------------
/**
    @class HashTable
    
    Organizes key/value pairs by a hash code. Looks very similar
    to a Dictionary, but may provide better search times (up to O(1))
    by computing a (ideally unique) hash code on the key and using that as an
    index into an array. The flipside is that the key class must provide
    a hash code and the memory footprint may be larger then Dictionary.
    
    The default capacity is 128. Matching the capacity against the number
    of expected elements in the hash table is one key to get optimal 
    insertion and search times, the other is to provide a good (and fast) 
    hash code computation which produces as few collissions as possible 
    for the key type.

    The key class must implement the following method in order to
    work with the HashTable:    
    IndexT HashCode() const;

    The String class implements this method as an example.
    Internally the hash table is implemented as a fixed array
    of sorted arrays. The fixed array is indexed by the hash code
    of the key, the sorted arrays contain all values with identical
    keys.

    (C) 2006 Radon Labs GmbH
*/

/**
	注意：

	编写添加了迭代器以及相关函数，功能正常，效率不错；

	Capacity默认128，大数据量(比如2,000,000个)记得HashTable(SizeT capacity)构造，要不等着悲剧；

	Added by Li
	(C) 2012 PhiloLabs
*/

#include "util/fixedarray.h"
#include "util/array.h"
#include "util/keyvaluepair.h"

//------------------------------------------------------------------------------
namespace Philo
{
template<class KEYTYPE, class VALUETYPE> class HashTable
{
public:
	class Iterator;
	typedef KeyValuePair<KEYTYPE, VALUETYPE> KeyValueType;

    /// default constructor
    HashTable();
    /// constructor with capacity
    HashTable(SizeT capacity);
    /// copy constructor
    HashTable(const HashTable<KEYTYPE, VALUETYPE>& rhs);
    /// assignment operator
    void operator=(const HashTable<KEYTYPE, VALUETYPE>& rhs);
    /// read/write [] operator, assertion if key not found
    VALUETYPE& operator[](const KEYTYPE& key) const;
    /// return current number of values in the hashtable
	SizeT Size() const{return this->size;}
    /// return fixed-size capacity of the hash table
	SizeT Capacity() const{return this->hashArray.Size();}
    /// clear the hashtable
    void Clear();
    /// return true if empty
    bool IsEmpty() const;
    /// add a key/value pair object to the hash table
    void Add(const KeyValuePair<KEYTYPE, VALUETYPE>& kvp);
    /// add a key and associated value
    void Add(const KEYTYPE& key, const VALUETYPE& value);
    /// erase an entry
    void Erase(const KEYTYPE& key);
    /// return true if key exists in the array
    bool Contains(const KEYTYPE& key) const;
    /// return array of all key/value pairs in the table (slow)
    Array<KeyValuePair<KEYTYPE, VALUETYPE> > Content() const;

	/// find element in table with key, return pointer to KeyValuePair, NULL if not found   Added by Li
	KeyValuePair<KEYTYPE, VALUETYPE>* FindKV(const KEYTYPE& key);
	/// find element in table with key, return iterator, End if not found   Added by Li
	Iterator Find(const KEYTYPE& key);
	/// get iterator to first element   Added by Li
	Iterator Begin();
	/// get iterator !past! the last element   Added by Li
	Iterator End();
	/// erase an element with an iterator, the iterator will automatically be the next valid iterator   Added by Li
	void Erase(Iterator& iter);

	/// the embeded HashTable Iterator   Added by Li
	class Iterator
	{
	public:
		/// default constructor
		Iterator();
		/// constructor
		Iterator(FixedArray<Array<KeyValuePair<KEYTYPE, VALUETYPE> > >* hashArr, int idx1, int idx2);
		/// copy constructor
		Iterator(const Iterator& rhs);
		/// assignment operator
		const Iterator& operator=(const Iterator& rhs);
		/// equality operator
		bool operator==(const Iterator& rhs) const;
		/// inequality operator
		bool operator!=(const Iterator& rhs) const;
		/// pre-increment operator, if none, return End, if End, return Begin
		const Iterator& operator++();
		/// post-increment operator, if none, return End, if End, return Begin
		Iterator operator++(int);
		/// pre-decrement operator, if none, return End
		const Iterator& operator--();
		/// safe -> operator
		KeyValuePair<KEYTYPE, VALUETYPE>* operator->() const;
		/// safe dereference operator
		KeyValuePair<KEYTYPE, VALUETYPE>& operator*() const;

	private:
		friend class HashTable<KEYTYPE, VALUETYPE>;

		void set_begin();
		void set_end();
		bool is_end();

		FixedArray<Array<KeyValuePair<KEYTYPE, VALUETYPE> > >* hash_array_ptr;
		int idx_1;
		int idx_2;
	};

private:
    FixedArray<Array<KeyValuePair<KEYTYPE, VALUETYPE> > > hashArray;
    int size;
};

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
HashTable<KEYTYPE, VALUETYPE>::HashTable() :
    hashArray(128),
    size(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
HashTable<KEYTYPE, VALUETYPE>::HashTable(SizeT capacity) :
    hashArray(capacity),
    size(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
HashTable<KEYTYPE, VALUETYPE>::HashTable(const HashTable<KEYTYPE, VALUETYPE>& rhs) :
    hashArray(rhs.hashArray),
    size(rhs.size)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::operator=(const HashTable<KEYTYPE, VALUETYPE>& rhs)
{
    if (this != &rhs)
    {
        this->hashArray = rhs.hashArray;
        this->size = rhs.size;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
VALUETYPE&
HashTable<KEYTYPE, VALUETYPE>::operator[](const KEYTYPE& key) const
{
    // get hash code from key, trim to capacity
    IndexT hashIndex = key.HashCode() % this->hashArray.Size();
    const Array<KeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
    int numHashElements = hashElements.Size();
    #if NEBULA3_BOUNDSCHECKS    
    ph_assert(0 != numHashElements); // element with key doesn't exist
    #endif
    if (1 == numHashElements)
    {
        // no hash collissions, just return the only existing element
        return hashElements[0].Value();
    }
    else
    {
        // here's a hash collision, find the right key
        // with a binary search
        IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
        #if NEBULA3_BOUNDSCHECKS
        ph_assert(InvalidIndex != hashElementIndex);
        #endif
        return hashElements[hashElementIndex].Value();
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Clear()
{
    IndexT i;
    SizeT num = this->hashArray.Size();
    for (i = 0; i < num; ++i)
    {
        this->hashArray[i].Clear();
    }
    this->size = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
HashTable<KEYTYPE, VALUETYPE>::IsEmpty() const
{
    return (0 == this->size);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Add(const KeyValuePair<KEYTYPE, VALUETYPE>& kvp)
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->Contains(kvp.Key()));
    #endif
    IndexT hashIndex = kvp.Key().HashCode() % this->hashArray.Size();
    this->hashArray[hashIndex].InsertSorted(kvp);
    this->size++;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Add(const KEYTYPE& key, const VALUETYPE& value)
{
    KeyValuePair<KEYTYPE, VALUETYPE> kvp(key, value);
    this->Add(kvp);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Erase(const KEYTYPE& key)
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(this->size > 0);
    #endif
    IndexT hashIndex = key.HashCode() % this->hashArray.Size();
    Array<KeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
    IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(InvalidIndex != hashElementIndex); // key doesn't exist
    #endif
    hashElements.EraseIndex(hashElementIndex);
    this->size--;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
bool
HashTable<KEYTYPE, VALUETYPE>::Contains(const KEYTYPE& key) const
{
    if (this->size > 0)
    {
        IndexT hashIndex = key.HashCode() % this->hashArray.Size();
        Array<KeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
        IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
        return (InvalidIndex != hashElementIndex);
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
Array<KeyValuePair<KEYTYPE, VALUETYPE> >
HashTable<KEYTYPE, VALUETYPE>::Content() const
{
    Array<KeyValuePair<KEYTYPE, VALUETYPE> > result(this->size, 8);//预留size个，by Li
    int num = this->hashArray.Size();
    for (int i = 0; i < num; i++)
    {
        if (this->hashArray[i].Size() > 0)
        {
            result.AppendArray(this->hashArray[i]);
        }
    }
    return result;
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
KeyValuePair<KEYTYPE, VALUETYPE>*
HashTable<KEYTYPE, VALUETYPE>::FindKV(const KEYTYPE& key)
{
	if (this->size < 0)
		return NULL;

	IndexT hashIndex = key.HashCode() % this->hashArray.Size();
	Array<KeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
	IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
	if (InvalidIndex != hashElementIndex)
		return &(hashElements[hashElementIndex]);

	return NULL;
}
//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
typename HashTable<KEYTYPE, VALUETYPE>::Iterator
HashTable<KEYTYPE, VALUETYPE>::Find(const KEYTYPE& key)
{
	if (this->size < 0)
		return this->End();

	IndexT hashIndex = key.HashCode() % this->hashArray.Size();
	Array<KeyValuePair<KEYTYPE, VALUETYPE> >& hashElements = this->hashArray[hashIndex];
	IndexT hashElementIndex = hashElements.BinarySearchIndex(key);
	if (InvalidIndex != hashElementIndex)
		return Iterator(&(this->hashArray), hashIndex, hashElementIndex); 

	return this->End();
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
typename HashTable<KEYTYPE, VALUETYPE>::Iterator
HashTable<KEYTYPE, VALUETYPE>::Begin()
{
	for (int i = 0; i < this->hashArray.Size(); ++i)
	{
		for (int j = 0; j < this->hashArray[i].Size(); ++j)
		{
			return Iterator(&(this->hashArray), i, j); 
		}
	}

	return Iterator(&(this->hashArray), this->hashArray.Size(), 0); 
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
typename HashTable<KEYTYPE, VALUETYPE>::Iterator
HashTable<KEYTYPE, VALUETYPE>::End()
{
	return Iterator(&(this->hashArray), this->hashArray.Size(), 0); 
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
void
HashTable<KEYTYPE, VALUETYPE>::Erase(Iterator& iter)
{
	int idx1 = iter.idx_1;
	int idx2 = iter.idx_2;
	--iter;
	this->hashArray[idx1].EraseIndex(idx2);
	++iter;
	--(this->size);
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
HashTable<KEYTYPE, VALUETYPE>::Iterator::Iterator() :
hash_array_ptr(0),
idx_1(0),
idx_2(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
HashTable<KEYTYPE, VALUETYPE>::Iterator::Iterator(FixedArray<Array<KeyValuePair<KEYTYPE, VALUETYPE> > >* hashArr, int idx1, int idx2) :
hash_array_ptr(hashArr),
idx_1(idx1),
idx_2(idx2)
{
	// empty
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
HashTable<KEYTYPE, VALUETYPE>::Iterator::Iterator(const Iterator& rhs) :
hash_array_ptr(rhs.hash_array_ptr),
idx_1(rhs.idx_1),
idx_2(rhs.idx_2)
{
	// empty
}

//------------------------------------------------------------------------------
/**
Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
inline void
HashTable<KEYTYPE, VALUETYPE>::Iterator::set_begin()
{
	for (int i = 0; i < (*this->hash_array_ptr).Size(); ++i)
	{
		for (int j = 0; j < (*this->hash_array_ptr)[i].Size(); ++j)
		{
			this->idx_1 = i;
			this->idx_2 = j;
			return;
		}
	}

	set_end();
}

//------------------------------------------------------------------------------
/**
Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
inline void
HashTable<KEYTYPE, VALUETYPE>::Iterator::set_end()
{
	this->idx_1 = (*this->hash_array_ptr).Size();
	this->idx_2 = 0;
}

//------------------------------------------------------------------------------
/**
Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
inline bool
HashTable<KEYTYPE, VALUETYPE>::Iterator::is_end()
{
	return (this->idx_1 == (*this->hash_array_ptr).Size() && this->idx_2 == 0);
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
const typename HashTable<KEYTYPE, VALUETYPE>::Iterator&
HashTable<KEYTYPE, VALUETYPE>::Iterator::operator=(const Iterator& rhs)
{
	if (&rhs != this)
	{
		this->hash_array_ptr = rhs.hash_array_ptr;
		this->idx_1 = rhs.idx_1;
		this->idx_2 = rhs.idx_2;
	}
	return *this;
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
bool
HashTable<KEYTYPE, VALUETYPE>::Iterator::operator==(const Iterator& rhs) const
{
	return (this->hash_array_ptr == rhs.hash_array_ptr &&
				this->idx_1 == rhs.idx_1 && this->idx_2 == rhs.idx_2 );
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
bool
HashTable<KEYTYPE, VALUETYPE>::Iterator::operator!=(const Iterator& rhs) const
{
	return (this->hash_array_ptr != rhs.hash_array_ptr ||
		this->idx_1 != rhs.idx_1 || this->idx_2 != rhs.idx_2 );
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
const typename HashTable<KEYTYPE, VALUETYPE>::Iterator&
HashTable<KEYTYPE, VALUETYPE>::Iterator::operator++()
{
	#if NEBULA3_BOUNDSCHECKS    
		ph_assert(0 != this->hash_array_ptr);
	#endif

	if ( is_end() )
		set_begin();

	if ( (*(this->hash_array_ptr))[this->idx_1].Size() - 1 > this->idx_2 )
	{
		++(this->idx_2);
		return *this; 
	}
	else
	{
		int i, j;
		for ( i = this->idx_1 + 1; i < this->hash_array_ptr->Size(); ++i )
		{
			for ( j = 0; j < (*(this->hash_array_ptr))[i].Size(); ++j )
			{
				this->idx_1 = i;
				this->idx_2 = j;
				return *this; 
			}
		}
	}

	set_end();
	return *this;
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
typename HashTable<KEYTYPE, VALUETYPE>::Iterator
HashTable<KEYTYPE, VALUETYPE>::Iterator::operator++(int)
{
	#if NEBULA3_BOUNDSCHECKS
		ph_assert(0 != this->hash_array_ptr);
	#endif

	Iterator temp( (*this) );

	++this;

// 	if ( (*(this->hash_array_ptr))[this->idx_1].Size() - 1 > this->idx_2 )
// 	{
// 		++(this->idx_2);
// 		return temp;
// 	}
// 	else
// 	{
// 		int i, j;
// 		for ( i = this->idx_1 + 1; i < this->hash_array_ptr->Size(); ++i )
// 		{
// 			for ( j = 0; j < (*(this->hash_array_ptr))[i].Size(); ++j )
// 			{
// 				this->idx_1 = i;
// 				this->idx_2 = j;
// 				return temp; 
// 			}
// 		}
// 	}
// 
// 	set_end();

	return temp;
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
const typename HashTable<KEYTYPE, VALUETYPE>::Iterator&
HashTable<KEYTYPE, VALUETYPE>::Iterator::operator--()
{
#if NEBULA3_BOUNDSCHECKS    
	ph_assert(0 != this->hash_array_ptr);
#endif

	if (this->idx_2 > 0)
	{
		--(this->idx_2);
		return *this; 
	}
	else
	{
		int i, j;
		for ( i = this->idx_1 - 1; i >=0 ; --i )
		{
			for ( j = (*(this->hash_array_ptr))[i].Size() - 1; j >=0; --j )
			{
				this->idx_1 = i;
				this->idx_2 = j;
				return *this; 
			}
		}
	}

	set_end();
	return *this;
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
KeyValuePair<KEYTYPE, VALUETYPE>*
HashTable<KEYTYPE, VALUETYPE>::Iterator::operator->() const
{
	#if NEBULA3_BOUNDSCHECKS
		ph_assert(this->hash_array_ptr);
	#endif
	return &((*(this->hash_array_ptr))[this->idx_1][this->idx_2]);
}

//------------------------------------------------------------------------------
/**
	Added by Li
*/
template<class KEYTYPE, class VALUETYPE>
KeyValuePair<KEYTYPE, VALUETYPE>&
HashTable<KEYTYPE, VALUETYPE>::Iterator::operator*() const
{
	#if NEBULA3_BOUNDSCHECKS
		ph_assert(this->hash_array_ptr);
	#endif
	return (*(this->hash_array_ptr))[this->idx_1][this->idx_2];
}

} // namespace Philo
//------------------------------------------------------------------------------
