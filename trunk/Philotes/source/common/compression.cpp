
#include "compression.h"
#include "zlib.h"
#include "performance.h"
//-----------------------------------
// Available compression levels
// Z_NO_COMPRESSION
// Z_BEST_SPEED
// Z_BEST_COMPRESSION
// Z_DEFAULT_COMPRESSION

const INT32 COMPRESSION_NONE = Z_NO_COMPRESSION;
const INT32 COMPRESSION_SPEED = Z_BEST_SPEED;
const INT32 COMPRESSION_SIZE = Z_BEST_COMPRESSION;
const INT32 COMPRESSION_DEFAULT = Z_DEFAULT_COMPRESSION;

BOOL ZLIB_DEFLATE(
	UINT8* in,
	UINT32 in_size,
	UINT8* out,
	UINT32* out_size,
	UINT32 level)
{
	ASSERT_RETFALSE(in != NULL);
	ASSERT_RETFALSE(out != NULL);
	ASSERT_RETFALSE(out_size != NULL);

	UINT32 cur, cur_out, size, size_out, available, left;
	UINT32 flush;
	z_stream strm;

	cur = 0;
	cur_out = 0;
	available = *out_size;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ASSERT_RETFALSE(deflateInit(&strm, level) == Z_OK);

	while ((left = in_size - cur) > 0) {
		if (left <= DEFAULT_CHUNK_SIZE) {
			size = left;
			flush = Z_FINISH;
		} else {
			size = DEFAULT_CHUNK_SIZE;
			flush = Z_NO_FLUSH;
		}
		strm.avail_in = size;
		strm.next_in = in + cur;

		while (TRUE) {
			size_out = (available > DEFAULT_CHUNK_SIZE ? DEFAULT_CHUNK_SIZE : available);
			ASSERT_GOTO(size_out > 0, _error);

			strm.avail_out = size_out;
			strm.next_out = out + cur_out;
			ASSERT_GOTO(deflate(&strm, flush) != Z_STREAM_ERROR, _error);
			cur_out += (size_out - strm.avail_out);
			available -= (size_out - strm.avail_out);

			if (strm.avail_out > 0) {
				break;
			}
			if (strm.avail_in == 0 && available == 0) {
				break;
			}
		}
		cur += size;
	}
	*out_size = cur_out;

	deflateEnd(&strm);
	return TRUE;

_error:
	deflateEnd(&strm);
	return FALSE;
}

BOOL ZLIB_INFLATE(
	UINT8* in,
	UINT32 in_size,
	UINT8* out,
	UINT32* out_size)
{
	ASSERT_RETFALSE(in != NULL);
	ASSERT_RETFALSE(out != NULL);
	ASSERT_RETFALSE(out_size != NULL);

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ASSERT_RETFALSE(inflateInit(&strm) == Z_OK);

	UINT32 cur_in, cur_out, left, size;
	UINT32 size_out, available;
	INT32 result;

	cur_in = 0;
	cur_out = 0;
	available = *out_size;

	while ((left = in_size - cur_in) > 0) {
		size = (left <= DEFAULT_CHUNK_SIZE ? left : DEFAULT_CHUNK_SIZE);
		if (left - size < 0x100) {
			size = left;
		}
		strm.avail_in = size;
		strm.next_in = in + cur_in;
//		trace("  decompressing 0x%x bytes 0x%x left\n", size, left);

		while(TRUE) {
			size_out = (available > DEFAULT_CHUNK_SIZE ? DEFAULT_CHUNK_SIZE : available);
			ASSERT_GOTO(size_out > 0, _error);
			
			strm.avail_out = size_out;
			strm.next_out = out + cur_out;
			result = inflate(&strm, Z_NO_FLUSH);

			ASSERT_GOTO(result != Z_STREAM_ERROR, _error);
			ASSERT_GOTO(result != Z_NEED_DICT, _error);
			ASSERT_GOTO(result != Z_DATA_ERROR, _error);
			ASSERT_GOTO(result != Z_MEM_ERROR, _error);
			cur_out += (size_out - strm.avail_out);
			available -= (size_out - strm.avail_out);

//			trace("    writing 0x%x bytes   total 0x%x\n", (size_out - strm.avail_out), size_out);

			if (strm.avail_out > 0) {
				break;
			}
			if (strm.avail_in == 0 && available == 0) {
				break;
			}
		}
		cur_in += size;
	}
	*out_size = cur_out;
	inflateEnd(&strm);
	return TRUE;

_error:
	inflateEnd(&strm);
	return FALSE;
}
