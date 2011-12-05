
#include "sha1.h"

#define SHA1RealHashSize 20

namespace COMMON_SHA1
{

static void sSHA1ContextDestroy(SHA1Context* pCtx)
{
	ASSERT_RETURN(pCtx != NULL);
	if (pCtx->hHash != NULL) {
		CryptDestroyHash(pCtx->hHash);
		pCtx->hHash = NULL;
	}
	if (pCtx->hCryptProv != NULL) {
		CryptReleaseContext(pCtx->hCryptProv,0);
		pCtx->hCryptProv = NULL;
	}
}

SHA1Context::SHA1Context() : hCryptProv(NULL), hHash(NULL)
{

}

SHA1Context::~SHA1Context()
{
	sSHA1ContextDestroy(this);
}

BOOL SHA1Reset(SHA1Context* pCtx)
{
	sSHA1ContextDestroy(pCtx);
/*	if(CryptAcquireContext(&pCtx->hCryptProv, NULL, NULL, PROV_RSA_FULL, 0) == FALSE) {
		ASSERT_RETFALSE(GetLastError() == NTE_BAD_KEYSET);
		ASSERT_RETFALSE(CryptAcquireContext(&pCtx->hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET));
	}*/
	ASSERT_RETFALSE(CryptAcquireContext(&pCtx->hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT));
	ASSERT_RETFALSE(CryptCreateHash(pCtx->hCryptProv, CALG_SHA1, 0, 0, &pCtx->hHash));
	return TRUE;
}

BOOL SHA1Input(SHA1Context *pCtx, const UINT8 *message_array, UINT32 length)
{
	ASSERT_RETFALSE(pCtx->hCryptProv != NULL);
	ASSERT_RETFALSE(pCtx->hHash != NULL);
	ASSERT_RETFALSE(CryptHashData(pCtx->hHash, message_array, length, 0));
	return TRUE;
}

BOOL SHA1Result(SHA1Context *pCtx, UINT8 Message_Digest[SHA1RealHashSize])
{
	DWORD len = SHA1RealHashSize;

	ASSERT_GOTO(pCtx->hCryptProv != NULL, _error);
	ASSERT_GOTO(pCtx->hHash != NULL, _error);
	ASSERT_GOTO(CryptGetHashParam(pCtx->hHash, HP_HASHVAL, Message_Digest, &len, 0), _error);
	ASSERT_GOTO(len == SHA1RealHashSize, _error);
	return TRUE;

_error:
	trace("SHA1Result error: 0x%x\n", GetLastError());
	return FALSE;
}

BOOL SHA1Calculate(UINT8* input, UINT32 len, UINT8* output)
{
	SHA1Context ctx;
	UINT8 tmpOutput[SHA1RealHashSize];

	ASSERT_RETFALSE(SHA1Reset(&ctx));
	ASSERT_RETFALSE(SHA1Input(&ctx, input, len));
	ASSERT_RETFALSE(SHA1Result(&ctx, tmpOutput));
	ASSERT_RETFALSE(MemoryCopy(output, SHA1HashSize, tmpOutput, SHA1HashSize));
	return TRUE;
}

#define FILE_READ_SIZE (0x1000)

BOOL SHA1GetFileSHA(HANDLE hFile, UINT8* output)
{
	SHA1Context ctx;
	DWORD bytes_read;
	UINT8 buffer[FILE_READ_SIZE];
	UINT8 tmpOutput[SHA1RealHashSize];

	ASSERT_RETFALSE(hFile != NULL);
	ASSERT_RETFALSE(SHA1Reset(&ctx));

	while (TRUE) {
		ASSERT_RETFALSE(ReadFile(hFile, buffer, FILE_READ_SIZE, &bytes_read, NULL));
		if (bytes_read == 0) {
			break;
		}
		ASSERT_RETFALSE(SHA1Input(&ctx, buffer, bytes_read));
	}
	ASSERT_RETFALSE(SHA1Result(&ctx, tmpOutput));
	ASSERT_RETFALSE(MemoryCopy(output, SHA1HashSize, tmpOutput, SHA1HashSize));
	return TRUE;
}

};
