//------------------------------------------------------------------------------
//  fast_hash.cpp 
//  (C) 2012 PhiloLabs
//------------------------------------------------------------------------------

#include "util/fast_hash.h"

namespace Util
{
/*
#if PHILO_ENDIAN == PHILO_ENDIAN_LITTLE
#  define PHILO_GET16BITS(d) (*((const uint16 *) (d)))
#else
// Cast to uint16 in little endian means first byte is least significant
// replicate that here
#  define PHILO_GET16BITS(d) (*((const uint8 *) (d)) + (*((const uint8 *) (d+1))<<8))
#endif
*/
#define PHILO_GET16BITS(d) (*((const uint16 *) (d)))

//------------------------------------------------------------------------------
/**
*/
uint32
FastHash::ComputeHash( const char * data, int len, uint32 hashSoFar /*= 0*/ )
{
	uint32 hash;
	uint32 tmp;
	int rem;

	if (hashSoFar)
		hash = hashSoFar;
	else
		hash = len;

	if (len <= 0 || data == NULL) return 0;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; len--) {
		hash  += PHILO_GET16BITS (data);
		tmp    = (PHILO_GET16BITS (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (uint16);
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
	case 3: hash += PHILO_GET16BITS (data);
		hash ^= hash << 16;
		hash ^= data[sizeof (uint16)] << 18;
		hash += hash >> 11;
		break;
	case 2: hash += PHILO_GET16BITS (data);
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
	case 1: hash += *data;
		hash ^= hash << 10;
		hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}

}