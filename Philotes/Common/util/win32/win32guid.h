#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32Guid
    
    Win32 implementation of the Guid class. GUIDs can be
    compared and provide a hash code, so they can be used as keys in
    most collections.
    
    (C) 2006 Radon Labs GmbH
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32Guid
{
public:
   
	/// ‘› ±…æµÙnew÷ÿ‘ÿ

    /// constructor
    Win32Guid();
    /// copy constructor
    Win32Guid(const Win32Guid& rhs);
    /// construct from raw binary data as returned by AsBinary()
    Win32Guid(const unsigned char* ptr, SizeT size);
    /// assignement operator
    void operator=(const Win32Guid& rhs);
    /// assignment operator from string
    void operator=(const String& rhs);
    /// equality operator
    bool operator==(const Win32Guid& rhs) const;
    /// inequlality operator
    bool operator!=(const Win32Guid& rhs) const;
    /// less-then operator
    bool operator<(const Win32Guid& rhs) const;
    /// less-or-equal operator
    bool operator<=(const Win32Guid& rhs) const;
    /// greater-then operator
    bool operator>(const Win32Guid& rhs) const;
    /// greater-or-equal operator
    bool operator>=(const Win32Guid& rhs) const;
    /// return true if the contained guid is valid (not NIL)
    bool IsValid() const;
    /// generate a new guid
    void Generate();
    /// construct from string representation
    static Win32Guid FromString(const String& str);
    /// construct from binary representation
    static Win32Guid FromBinary(const unsigned char* ptr, SizeT numBytes);
    /// get as string
    String AsString() const;
    /// get pointer to binary data
    SizeT AsBinary(const unsigned char*& outPtr) const;
    /// return the size of the binary representation in bytes
    static SizeT BinarySize();
    /// get a hash code (compatible with HashTable)
    IndexT HashCode() const;

private:
    UUID uuid;
};


//------------------------------------------------------------------------------
/**
*/
inline
Win32Guid::Win32Guid()
{
    ZeroMemory(&(this->uuid), sizeof(this->uuid));
}

//------------------------------------------------------------------------------
/**
*/
inline
Win32Guid::Win32Guid(const Win32Guid& rhs)
{
    this->uuid = rhs.uuid;
}

//------------------------------------------------------------------------------
/**
*/
inline
Win32Guid::Win32Guid(const unsigned char* ptr, SizeT size)
{
    ph_assert((0 != ptr) && (size == sizeof(UUID)));
    memcpy(&this->uuid, ptr, sizeof(UUID));
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Win32Guid::BinarySize()
{
    return sizeof(UUID);
}

} // namespace Win32
//------------------------------------------------------------------------------
