#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::BitField
    
	Implements large bit field with multiple of 32 bits (is like std::bitset).
    
	(C) 2012 PhiloLabs
*/

/**
	注意：

	WARNING:移位时候rhs用uint效率可以高很多，不要用int ；

	效率修正了，现在综合效率比stl高一点；on 2011-11-25 12:59:00

	Added by Li
	(C) 2012 PhiloLabs
*/

#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{
template<uint NUMBITS> class BitField
{
public:
    /// constructor
    BitField();
    /// copy constructor
    BitField(const BitField<NUMBITS>& rhs);
    
    /// assignment operator
    void operator=(const BitField<NUMBITS>& rhs);
    /// equality operator
    bool operator==(const BitField<NUMBITS>& rhs) const;
    /// inequality operator
    bool operator!=(const BitField<NUMBITS>& rhs) const;

	/// set a bit by index
	void SetBit(uint i)
	{
		ph_assert(i < NUMBITS);
		this->bits[i / 32] |= 1 << i % 32;
	}
	/// clear a bit by index
	void ClearBit(uint i)
	{
		ph_assert(i < NUMBITS);
		this->bits[i / 32] &= ~(1 << i % 32);
	}
	/// test if bit at index is 1
	bool TestBit(uint i) const
	{
		ph_assert(i < NUMBITS);
		return (this->bits[i / 32] & (1 << i % 32)) != 0 ;
	}
	/// clear content
	void Clear()
	{
		for (uint i = 0; i < size; ++i)
			this->bits[i] = 0;
	}
	/// return true if all bits are 0
	bool IsNull() const
	{
		for (uint i = 0; i < size; ++i)
			if (this->bits[i] != 0)
				return false;

		return true;
	}

    /// set bitfield to OR combination
    static BitField<NUMBITS> Or(const BitField<NUMBITS>& b0, const BitField<NUMBITS>& b1);
    /// set bitfield to AND combination
    static BitField<NUMBITS> And(const BitField<NUMBITS>& b0, const BitField<NUMBITS>& b1);

private:
    static const uint size = (NUMBITS + 31) / 32;
    uint bits[size];
};

//------------------------------------------------------------------------------
/**
*/
template<unsigned int NUMBITS>
BitField<NUMBITS>::BitField()
{
    for (uint i = 0; i < size; ++i)
    {
        this->bits[i] = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<unsigned int NUMBITS>
BitField<NUMBITS>::BitField(const BitField<NUMBITS>& rhs)
{
    for (uint i = 0; i < size; ++i)
    {
        this->bits[i] = rhs.bits[i];
    }
}

//------------------------------------------------------------------------------
/**
*/
template<unsigned int NUMBITS> void
BitField<NUMBITS>::operator=(const BitField<NUMBITS>& rhs)
{
    for (uint i = 0; i < size; ++i)
    {
        this->bits[i] = rhs.bits[i];
    }
}

//------------------------------------------------------------------------------
/**
*/
template<unsigned int NUMBITS> bool
BitField<NUMBITS>::operator==(const BitField<NUMBITS>& rhs) const
{
    for (uint i = 0; i < size; ++i)
    {
        if (this->bits[i] != rhs.bits[i])
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
template<unsigned int NUMBITS> bool
BitField<NUMBITS>::operator!=(const BitField<NUMBITS>& rhs) const
{
    return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
template<unsigned int NUMBITS> BitField<NUMBITS>
BitField<NUMBITS>::Or(const BitField<NUMBITS>& b0, const BitField<NUMBITS>& b1)
{
    BitField<NUMBITS> res;
    for (uint i = 0; i < size; ++i)
    {
        res.bits[i] = b0.bits[i] | b1.bits[i];
    }
    return res;
}

//------------------------------------------------------------------------------
/**
*/
template<unsigned int NUMBITS> BitField<NUMBITS>
BitField<NUMBITS>::And(const BitField<NUMBITS>& b0, const BitField<NUMBITS>& b1)
{
    BitField<NUMBITS> res;
    for (uint i = 0; i < size; ++i)
    {
        res.bits[i] = b0.bits[i] & b1.bits[i];
    }
    return res;
}

} // namespace Util
//------------------------------------------------------------------------------

    