#pragma once
//------------------------------------------------------------------------------
/**
    @class Dictionary
  
    A collection of key/value pairs with quick value retrieval
    by key at roughly O(log n). 

    Internally the dictionary is implemented as a sorted array.

    On insertion performance:
    Key/value pairs are inserted with the Add() method, which normally
    calls the Array::InsertSorted() method internally. If many
    insertions are performed at once, it may be beneficial to call
    BeginBulkAdd() before, and EndBulkAdd() after adding the 
    key/value pairs. Between BeginBulkAdd() and EndBulkAdd(), the
    Add() method will just append the new elements to the internal
    array, and only call Array::Sort() inside EndBulkAdd().

    Any methods which require the internal array to be sorted will
    throw an assertion between BeginBulkAdd() and EndBulkAdd().

    (C) 2006 Radon Labs GmbH
*/  

/**
	注意：

	Dictionay 测试报告，整体效率远远超过std::map ；

	键Key可以重复；

	修改了[]重载，如果key不存在，会自动创建一个；

	BeginBulkAdd()与EndBulkAdd()之间不排序，EndBulkAdd()时排一次，
	这样效率较高，大量数据插入推荐Bulk，Add则是每次排序插入；

	大量数据插入请用Reserve，获得类似于Array的极大效率提升;

	Added by Li
	(C) 2012 PhiloLabs
*/

#include "util/array.h"
#include "util/keyvaluepair.h"

//------------------------------------------------------------------------------
namespace Philo
{
template<class KEYTYPE, class VALUETYPE> class Dictionary
{
public:
    /// default constructor
    Dictionary();
    /// copy constructor
    Dictionary(const Dictionary<KEYTYPE, VALUETYPE>& rhs);
    /// assignment operator
    void operator=(const Dictionary<KEYTYPE, VALUETYPE>& rhs);
    /// read/write [] operator     	Modified by Dinghao Li
    VALUETYPE& operator[](const KEYTYPE& key);
    /// read-only [] operator
    const VALUETYPE& operator[](const KEYTYPE& key) const;
    /// return number of key/value pairs in the dictionary
    SizeT Size() const;
    /// clear the dictionary
    void Clear();
    /// return true if empty
    bool IsEmpty() const;
    /// reserve space (useful if number of elements is known beforehand)
    void Reserve(SizeT numElements);
    /// begin a bulk insert (array will be sorted at End)
    void BeginBulkAdd();
    /// add a key/value pair
    void Add(const KeyValuePair<KEYTYPE, VALUETYPE>& kvp);
    /// add a key and associated value
    void Add(const KEYTYPE& key, const VALUETYPE& value);
    /// end a bulk insert (this will sort the internal array)
    void EndBulkAdd();
    /// erase a key-value element
    void Erase(const KEYTYPE& key);
    /// erase a key-value element at index
    void EraseAtIndex(IndexT index);
    /// find index of key/value pair (InvalidIndex if doesn't exist)
    IndexT FindIndex(const KEYTYPE& key) const;
    /// return true if key exists in the array
    bool Contains(const KEYTYPE& key) const;
    /// get a key at given index
    const KEYTYPE& KeyAtIndex(IndexT index) const;
    /// access to value at given index
    VALUETYPE& ValueAtIndex(IndexT index);
    /// get a value at given index
    const VALUETYPE& ValueAtIndex(IndexT index) const;
    /// get key/value pair at index
    KeyValuePair<KEYTYPE, VALUETYPE>& KeyValuePairAtIndex(IndexT index) const;
    /// get all keys as an Array
    Array<KEYTYPE> KeysAsArray() const;
    /// get all keys as an Array
    Array<VALUETYPE> ValuesAsArray() const;
    /// get all keys as (typically) an array
    template<class RETURNTYPE> RETURNTYPE KeysAs() const;
    /// get all keys as (typically) an array
    template<class RETURNTYPE> RETURNTYPE ValuesAs() const;

protected:
    /// make sure the key value pair array is sorted
    void SortIfDirty() const;

    Array<KeyValuePair<KEYTYPE, VALUETYPE> > keyValuePairs;
    bool inBulkInsert;
};

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
Dictionary<KEYTYPE, VALUETYPE>::Dictionary() :
    inBulkInsert(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
Dictionary<KEYTYPE, VALUETYPE>::Dictionary(const Dictionary<KEYTYPE, VALUETYPE>& rhs) :
    keyValuePairs(rhs.keyValuePairs),
    inBulkInsert(false)
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!rhs.inBulkInsert);
    #endif
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> void
Dictionary<KEYTYPE, VALUETYPE>::operator=(const Dictionary<KEYTYPE, VALUETYPE>& rhs)
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->inBulkInsert);
    ph_assert(!rhs.inBulkInsert);
    #endif
    this->keyValuePairs = rhs.keyValuePairs;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> void
Dictionary<KEYTYPE, VALUETYPE>::Clear()
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->inBulkInsert);
    #endif
    this->keyValuePairs.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> SizeT
Dictionary<KEYTYPE, VALUETYPE>::Size() const
{
    return this->keyValuePairs.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> bool
Dictionary<KEYTYPE, VALUETYPE>::IsEmpty() const
{
    return (0 == this->keyValuePairs.Size());
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> void
Dictionary<KEYTYPE, VALUETYPE>::Add(const KeyValuePair<KEYTYPE, VALUETYPE>& kvp)
{
    if (this->inBulkInsert)
    {
        this->keyValuePairs.Append(kvp);
    }
    else
    {
        this->keyValuePairs.InsertSorted(kvp);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> void
Dictionary<KEYTYPE, VALUETYPE>::Reserve(SizeT numElements)
{
    this->keyValuePairs.Reserve(numElements);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> void
Dictionary<KEYTYPE, VALUETYPE>::BeginBulkAdd()
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->inBulkInsert);
    #endif
    this->inBulkInsert = true;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> void
Dictionary<KEYTYPE, VALUETYPE>::EndBulkAdd()
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(this->inBulkInsert);
    #endif
    this->keyValuePairs.Sort();
    this->inBulkInsert = false;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> void
Dictionary<KEYTYPE, VALUETYPE>::Add(const KEYTYPE& key, const VALUETYPE& value)
{
    //ph_assert(!this->Contains(key));
    KeyValuePair<KEYTYPE, VALUETYPE> kvp(key, value);
    if (this->inBulkInsert)
    {
        this->keyValuePairs.Append(kvp);
    }
    else
    {
        this->keyValuePairs.InsertSorted(kvp);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> void
Dictionary<KEYTYPE, VALUETYPE>::Erase(const KEYTYPE& key)
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->inBulkInsert);
    #endif
    IndexT eraseIndex = this->keyValuePairs.BinarySearchIndex(key);
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(InvalidIndex != eraseIndex);
    #endif
    this->keyValuePairs.EraseIndex(eraseIndex);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> void
Dictionary<KEYTYPE, VALUETYPE>::EraseAtIndex(IndexT index)
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->inBulkInsert);
    #endif
    this->keyValuePairs.EraseIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> IndexT
Dictionary<KEYTYPE, VALUETYPE>::FindIndex(const KEYTYPE& key) const
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->inBulkInsert);
    #endif
    return this->keyValuePairs.BinarySearchIndex(key);
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> bool
Dictionary<KEYTYPE, VALUETYPE>::Contains(const KEYTYPE& key) const
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->inBulkInsert);
    #endif
    return (InvalidIndex != this->keyValuePairs.BinarySearchIndex(key));
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> const KEYTYPE&
Dictionary<KEYTYPE, VALUETYPE>::KeyAtIndex(IndexT index) const
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->inBulkInsert);
    #endif
    return this->keyValuePairs[index].Key();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> VALUETYPE&
Dictionary<KEYTYPE, VALUETYPE>::ValueAtIndex(IndexT index)
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->inBulkInsert);
    #endif
    return this->keyValuePairs[index].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> const VALUETYPE&
Dictionary<KEYTYPE, VALUETYPE>::ValueAtIndex(IndexT index) const
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->inBulkInsert);
    #endif
    return this->keyValuePairs[index].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> KeyValuePair<KEYTYPE, VALUETYPE>&
Dictionary<KEYTYPE, VALUETYPE>::KeyValuePairAtIndex(IndexT index) const
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->inBulkInsert);
    #endif
    return this->keyValuePairs[index];
}

//------------------------------------------------------------------------------
/**
	Modified by Dinghao Li
*/
template<class KEYTYPE, class VALUETYPE>
VALUETYPE&
Dictionary<KEYTYPE, VALUETYPE>::operator[](const KEYTYPE& key)
{
    int keyValuePairIndex = this->keyValuePairs.BinarySearchIndex(key);
//     #if NEBULA3_BOUNDSCHECKS
//     ph_assert(InvalidIndex != keyValuePairIndex);
//     #endif   
	if (InvalidIndex == keyValuePairIndex)
	{
		KeyValuePair<KEYTYPE, VALUETYPE> kvp(key);
		if (this->inBulkInsert)
		{
			this->keyValuePairs.Append(kvp);
			return keyValuePairs.Back().Value();
		}
		else
		{
			return keyValuePairs[this->keyValuePairs.InsertSorted(kvp)].Value();
		}
	}
    return this->keyValuePairs[keyValuePairIndex].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> const VALUETYPE&
Dictionary<KEYTYPE, VALUETYPE>::operator[](const KEYTYPE& key) const
{
    int keyValuePairIndex = this->FindIndex(key);
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(InvalidIndex != keyValuePairIndex);
    #endif
    return this->keyValuePairs[keyValuePairIndex].Value();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
template<class RETURNTYPE>
RETURNTYPE
Dictionary<KEYTYPE, VALUETYPE>::ValuesAs() const
{
    #if NEBULA3_BOUNDSCHECKS
    ph_assert(!this->inBulkInsert);
    #endif
    RETURNTYPE result(this->Size(),this->Size());
    IndexT i;
    for (i = 0; i < this->keyValuePairs.Size(); i++)
    {
        result.Append(this->keyValuePairs[i].Value());
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
Array<VALUETYPE>
Dictionary<KEYTYPE, VALUETYPE>::ValuesAsArray() const
{
    return this->ValuesAs<Array<VALUETYPE> >();
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE> 
template<class RETURNTYPE>
RETURNTYPE
Dictionary<KEYTYPE, VALUETYPE>::KeysAs() const
{
    #if NEBULA3_BOUNDSCHECKS    
    ph_assert(!this->inBulkInsert);
    #endif
    RETURNTYPE result(this->Size(),this->Size());
    IndexT i;
    for (i = 0; i < this->keyValuePairs.Size(); i++)
    {
        result.Append(this->keyValuePairs[i].Key());
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
template<class KEYTYPE, class VALUETYPE>
Array<KEYTYPE>
Dictionary<KEYTYPE, VALUETYPE>::KeysAsArray() const
{
    return this->KeysAs<Array<KEYTYPE> >();
}

} // namespace Philo
//------------------------------------------------------------------------------
