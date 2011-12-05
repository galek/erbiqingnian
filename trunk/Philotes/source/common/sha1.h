#ifndef _SHA1_H_
#define _SHA1_H_

#if 0

namespace COMMON_SHA1
{

#ifndef _SHA_enum_
#define _SHA_enum_

enum
{
	shaSuccess = 0,
	shaNull,            /* Null pointer parameter */
	shaInputTooLong,    /* input data too long */
	shaStateError       /* called Input after Result */
};

#endif

#define SHA1HashSize 20

typedef struct SHA1Context
{
	UINT32 Intermediate_Hash[SHA1HashSize/4]; /* Message Digest  */

	UINT32 Length_Low;            /* Message length in bits      */
	UINT32 Length_High;           /* Message length in bits      */

	/* Index into message block array   */
	UINT32 Message_Block_Index;
	UINT8 Message_Block[64];      /* 512-bit message blocks      */

	INT32 Computed;               /* Is the digest computed?         */
	INT32 Corrupted;             /* Is the message digest corrupted? */
} SHA1Context;

BOOL SHA1Reset(  SHA1Context *);
BOOL SHA1Input(  SHA1Context *,
				const UINT8 *,
				UINT32);
BOOL SHA1Result( SHA1Context *,
				 UINT8 Message_Digest[SHA1HashSize]);

BOOL SHA1Calculate(UINT8* input, UINT32 len, UINT8* output);

};

#else // 1

#include <WinCrypt.h>

namespace COMMON_SHA1
{

#define SHA1HashSize 8
#define SHA1HashSizeBuffer 12

struct SHA1Context
{
	HCRYPTPROV hCryptProv;
	HCRYPTHASH hHash;

	SHA1Context();
	~SHA1Context();
};

BOOL SHA1Calculate(UINT8* input, UINT32 len, UINT8* output);
BOOL SHA1GetFileSHA(HANDLE hFile, UINT8* output);

};

#endif

#endif
