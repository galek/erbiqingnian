///PHILOTES Source Code.  (C)2012 PhiloLabs
#pragma once
//------------------------------------------------------------------------------
/**
@class FastHash

Compute FastHash checksums over a range of memory.

General hash function, derived from here
http://www.azillionmonkeys.com/qed/hash.html
Original by Paul Hsieh 

Added by Li
(C) 2012 PhiloLabs
*/

#include "core/types.h"

//------------------------------------------------------------------------------
namespace Philo
{
class FastHash
{

public:

	/// Fast general hashing algorithm
	static uint32 ComputeHash (const char * data, int len, uint32 hashSoFar = 0);
	/// Combine hashes with same style as boost::hash_combine
	template<typename T> static uint32 HashCombine (uint32 hashSoFar, const T& data);

};

//------------------------------------------------------------------------------
/**
*/
template<typename T> inline uint32
FastHash::HashCombine( uint32 hashSoFar, const T& data )
{
	return ComputeHash((const char*)&data, sizeof(T), hashSoFar);
}

} // namespace Philo
//------------------------------------------------------------------------------