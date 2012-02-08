#pragma once

#include "core/types.h"
#include "util/string.h"

namespace Philo
{
class Guid
{
public:

	/// ‘› ±…æµÙnew÷ÿ‘ÿ

	/// constructor
	Guid();
	/// copy constructor
	Guid(const Guid& rhs);
	/// construct from raw binary data as returned by AsBinary()
	Guid(const unsigned char* ptr, SizeT size);
	/// assignement operator
	void operator=(const Guid& rhs);
	/// assignment operator from string
	void operator=(const String& rhs);
	/// equality operator
	bool operator==(const Guid& rhs) const;
	/// inequlality operator
	bool operator!=(const Guid& rhs) const;
	/// less-then operator
	bool operator<(const Guid& rhs) const;
	/// less-or-equal operator
	bool operator<=(const Guid& rhs) const;
	/// greater-then operator
	bool operator>(const Guid& rhs) const;
	/// greater-or-equal operator
	bool operator>=(const Guid& rhs) const;
	/// return true if the contained guid is valid (not NIL)
	bool IsValid() const;
	/// generate a new guid
	void Generate();
	/// construct from string representation
	static Guid FromString(const String& str);
	/// construct from binary representation
	static Guid FromBinary(const unsigned char* ptr, SizeT numBytes);
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
Guid::Guid()
{
	ZeroMemory(&(this->uuid), sizeof(this->uuid));
}

//------------------------------------------------------------------------------
/**
*/
inline
Guid::Guid(const Guid& rhs)
{
	this->uuid = rhs.uuid;
}

//------------------------------------------------------------------------------
/**
*/
inline
Guid::Guid(const unsigned char* ptr, SizeT size)
{
	ph_assert((0 != ptr) && (size == sizeof(UUID)));
	memcpy(&this->uuid, ptr, sizeof(UUID));
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Guid::BinarySize()
{
	return sizeof(UUID);
}

}