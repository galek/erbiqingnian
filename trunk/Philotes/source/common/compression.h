//------------------------------------------------------------
// FILE: compression.h
//
//------------------------------------------------------------

#ifndef _COMPRESSION_H_
#define _COMPRESSION_H_

#include <winnt.h>

#define DEFAULT_CHUNK_SIZE (0x4000)

const extern INT32 COMPRESSION_NONE;
const extern INT32 COMPRESSION_SPEED;
const extern INT32 COMPRESSION_SIZE;
const extern INT32 COMPRESSION_DEFAULT;

BOOL ZLIB_DEFLATE(
	UINT8* in,
	UINT32 in_size,
	UINT8* out,
	UINT32* out_size,
	UINT32 level = COMPRESSION_DEFAULT);


BOOL ZLIB_INFLATE(
	UINT8* in,
	UINT32 in_size,
	UINT8* out,
	UINT32* out_size);




#endif
