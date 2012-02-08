
#include "util/guid.h"

namespace Philo
{
	
//------------------------------------------------------------------------------
/**
*/
void
Guid::operator=(const Guid& rhs)
{
    if (this != &rhs)
    {
        this->uuid = rhs.uuid;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Guid::operator=(const String& rhs)
{
    ph_assert(rhs.IsValid());
    RPC_STATUS result = UuidFromString((RPC_CSTR) rhs.AsCharPtr(), &(this->uuid));
    ph_assert(RPC_S_INVALID_STRING_UUID != result);
}

//------------------------------------------------------------------------------
/**
*/
bool
Guid::operator==(const Guid& rhs) const
{
    RPC_STATUS status;
    int result = UuidCompare(const_cast<UUID*>(&this->uuid), const_cast<UUID*>(&rhs.uuid), &status);
    return (0 == result);
}

//------------------------------------------------------------------------------
/**
*/
bool
Guid::operator!=(const Guid& rhs) const
{
    RPC_STATUS status;
    int result = UuidCompare(const_cast<UUID*>(&this->uuid), const_cast<UUID*>(&rhs.uuid), &status);
    return (0 != result);
}

//------------------------------------------------------------------------------
/**
*/
bool
Guid::operator<(const Guid& rhs) const
{
    RPC_STATUS status;
    int result = UuidCompare(const_cast<UUID*>(&this->uuid), const_cast<UUID*>(&rhs.uuid), &status);
    return (-1 == result);
}

//------------------------------------------------------------------------------
/**
*/
bool
Guid::operator<=(const Guid& rhs) const
{
    RPC_STATUS status;
    int result = UuidCompare(const_cast<UUID*>(&this->uuid), const_cast<UUID*>(&rhs.uuid), &status);
    return ((-1 == result) || (0 == result));
}

//------------------------------------------------------------------------------
/**
*/
bool
Guid::operator>(const Guid& rhs) const
{
    RPC_STATUS status;
    int result = UuidCompare(const_cast<UUID*>(&this->uuid), const_cast<UUID*>(&rhs.uuid), &status);
    return (1 == result);
}

//------------------------------------------------------------------------------
/**
*/
bool
Guid::operator>=(const Guid& rhs) const
{
    RPC_STATUS status;
    int result = UuidCompare(const_cast<UUID*>(&this->uuid), const_cast<UUID*>(&rhs.uuid), &status);
    return ((1 == result) || (0 == result));
}

//------------------------------------------------------------------------------
/**
*/
bool
Guid::IsValid() const
{
    RPC_STATUS status;
    int result = UuidIsNil(const_cast<UUID*>(&this->uuid), &status);
    return (TRUE != result);
}

//------------------------------------------------------------------------------
/**
*/
void
Guid::Generate()
{
    UuidCreate(&this->uuid);
}

//------------------------------------------------------------------------------
/**
*/
String
Guid::AsString() const
{
    const char* uuidStr;
    UuidToString((UUID*) &this->uuid, (RPC_CSTR*) &uuidStr);
    String result = uuidStr;
    RpcStringFree((RPC_CSTR*)&uuidStr);
    return result;
}

//------------------------------------------------------------------------------
/**
    This method allows read access to the raw binary data of the uuid.
    It returns the number of bytes in the buffer, and a pointer to the
    data.
*/
SizeT
Guid::AsBinary(const unsigned char*& outPtr) const
{
    outPtr = (const unsigned char*) &this->uuid;
    return sizeof(UUID);
}

//------------------------------------------------------------------------------
/**
*/
Guid
Guid::FromString(const String& str)
{
    Guid newGuid;
    RPC_STATUS success = UuidFromString((RPC_CSTR)str.AsCharPtr(), &(newGuid.uuid));
    ph_assert(RPC_S_OK == success);
    return newGuid;
}

//------------------------------------------------------------------------------
/**
    Constructs the guid from binary data, as returned by the AsBinary().
*/
Guid
Guid::FromBinary(const unsigned char* ptr, SizeT numBytes)
{
    ph_assert((0 != ptr) && (numBytes == sizeof(UUID)));
    Guid newGuid;
    memcpy(&(newGuid.uuid),ptr, sizeof(UUID));
    return newGuid;
}

//------------------------------------------------------------------------------
/**
    This method returns a hash code for the uuid, compatible with 
    HashTable.
*/
IndexT
Guid::HashCode() const
{
    RPC_STATUS status;
    unsigned short hashCode = UuidHash((UUID*)&this->uuid, &status);
    return (IndexT) hashCode;
}

}