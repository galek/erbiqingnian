// ---------------------------------------------------------------------------
// FILE:	cryptoapi.cpp
// DESC:	CryptoAPI wrappers
//
// Modified : $DateTime: 2007/12/11 11:49:11 $
// by       : $Author: pmason $
//
// Copyright 2006, Flagship Studios
// ---------------------------------------------------------------------------


#include <ServerSuite/include/ehm.h>
#include <wincrypt.h>
#include "cryptoapi.h"
#if ISVERSION(SERVER_VERSION)
#include "cryptoapi.cpp.tmh"
#endif
#pragma intrinsic(memcpy,memset)

HCRYPTPROV  hVerifyContextProvider;
BOOL        sbInitialized = FALSE;

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL CryptoInit(void)
{
   if(sbInitialized)
   {
       return FALSE;
   }
   return (sbInitialized = CryptAcquireContext(&hVerifyContextProvider,
                              NULL,
                              NULL,//MS_ENH_RSA_AES_PROV,
                              PROV_RSA_AES,
                              CRYPT_VERIFYCONTEXT));

}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
HCRYPTPROV CryptoGetProvider(void)
{
    ASSERT_RETNULL(sbInitialized);
    return hVerifyContextProvider;
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL CryptoHashBuffer(ALG_ID hashAlg,
                      const BYTE *pBuffer,
                      DWORD dwBufLen,
                      BYTE **ppOutput,
                      DWORD *pdwOutLen)
{
    BOOL bRet = TRUE;
    HCRYPTHASH hHash = NULL;
    DWORD dwOutputSize = 0;
    DWORD dataSize = 0;
    BYTE *pOutput= NULL;

    ASSERT_RETFALSE(sbInitialized);
    
    CBRA(CryptCreateHash(CryptoGetProvider(),
                         hashAlg,
                         NULL,
                         0,
                         &hHash));

    CBRA(CryptHashData(hHash,pBuffer,dwBufLen,0));

    // get the size required.
    dataSize = sizeof(dwOutputSize);

    CBRA(CryptGetHashParam(hHash,HP_HASHSIZE,(BYTE*)&dwOutputSize,&dataSize,0));

    CBRA(dwOutputSize);


    pOutput = (BYTE*)MALLOCZ(NULL,dwOutputSize);

    CBRA(pOutput);

    CBRA(CryptGetHashParam(hHash,HP_HASHVAL,pOutput,&dwOutputSize,0));

    *ppOutput = pOutput;
    *pdwOutLen = dwOutputSize;

Error:
    if(!bRet)
    {
        if(pOutput)
        {
            SecureZeroMemory(pOutput,dwOutputSize);
            FREE(NULL,pOutput);
        }
    }
    if(hHash)
    {
        CryptDestroyHash(hHash);
    }
    return bRet;
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void CryptoFreeHashBuffer(BYTE *pBuffer)
{
    ASSERT_RETURN(sbInitialized);
    FREE(NULL,pBuffer);
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL CryptoDeriveKeyFromPassword(ALG_ID     hashAlg,
                                 ALG_ID     encryptAlg,
                                 DWORD      dwKeyLen,
                                 const char      *password,
                                 HCRYPTKEY *phKey)
{
    HCRYPTHASH hHash = NULL;
    BOOL bRet =FALSE;
    ASSERT_RETFALSE(sbInitialized);

    if(!CryptCreateHash(CryptoGetProvider(),
                        hashAlg,
                        0,
                        0,
                        &hHash))
    {
       TraceError("CryptCreate Hash failed: %d\n",GetLastError()); 
       goto done;
    }
    if(!CryptHashData(hHash,(BYTE*)password,(DWORD)strlen(password),0))
    {
        TraceError("CryptHashData failed: %d\n",GetLastError());
        goto done;
    }
    if(!CryptDeriveKey(CryptoGetProvider(),
                       encryptAlg,
                       hHash,
                       dwKeyLen,
                       phKey))
    {
        TraceError("CryptDeriveKey failed: %d\n",GetLastError());
        goto done;
    }

    bRet = TRUE;
done:
    if(hHash)
    {
        CryptDestroyHash(hHash);
    }
    return bRet;
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void CryptoDestroyKey(HCRYPTKEY hKey)
{
    ASSERT_RETURN(sbInitialized);
    CryptDestroyKey(hKey);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL CryptoDecryptBufferWithKey(ALG_ID alg,
                                BYTE *pDataToDecrypt,
                                DWORD *pdwDataLen,
                                const BYTE *pTextKey,
                                DWORD dwKeyLen,
                                const BYTE *pIV) // must be size indicated by algorithm
{
    BOOL bRet = TRUE;
    HCRYPTKEY hKey = NULL;
    PUBLICKEYSTRUC *pKeyBlob = NULL;

    ASSERT_RETFALSE(sbInitialized);

    pKeyBlob = (PUBLICKEYSTRUC*)MALLOCZ(NULL,sizeof(PUBLICKEYSTRUC) + dwKeyLen + sizeof(DWORD));

    CBRA(pKeyBlob);

    pKeyBlob->bType = PLAINTEXTKEYBLOB;
    pKeyBlob->bVersion = CUR_BLOB_VERSION;
    pKeyBlob->aiKeyAlg = alg;

    memcpy(pKeyBlob + 1, &dwKeyLen,sizeof(DWORD));
    memcpy((BYTE*)(pKeyBlob + 1) + sizeof(DWORD),pTextKey,dwKeyLen);

    CBRA(CryptImportKey(CryptoGetProvider(),
                        (BYTE*)pKeyBlob,
                        sizeof(PUBLICKEYSTRUC) + sizeof(DWORD)+dwKeyLen,
                        NULL,
                        0,
                        &hKey));

    CBRA(CryptSetKeyParam(hKey,KP_IV,pIV,0));

    CBRA(CryptDecrypt(hKey,NULL,TRUE,0,pDataToDecrypt,pdwDataLen));
Error:
    if(pKeyBlob)
    {
       FREE(NULL,pKeyBlob); 
    }
    if(hKey)
    {
        CryptDestroyKey(hKey);
    }
    
    return bRet;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL CryptoDecryptBufferWithKeyNoAssert(ALG_ID alg,
								BYTE *pDataToDecrypt,
								DWORD *pdwDataLen,
								const BYTE *pTextKey,
								DWORD dwKeyLen,
								const BYTE *pIV) // must be size indicated by algorithm
{
	BOOL bRet = TRUE;
	HCRYPTKEY hKey = NULL;
	PUBLICKEYSTRUC *pKeyBlob = NULL;

	ASSERT_RETFALSE(sbInitialized);

	pKeyBlob = (PUBLICKEYSTRUC*)MALLOCZ(NULL,sizeof(PUBLICKEYSTRUC) + dwKeyLen + sizeof(DWORD));

	CBRA(pKeyBlob);

	pKeyBlob->bType = PLAINTEXTKEYBLOB;
	pKeyBlob->bVersion = CUR_BLOB_VERSION;
	pKeyBlob->aiKeyAlg = alg;

	memcpy(pKeyBlob + 1, &dwKeyLen,sizeof(DWORD));
	memcpy((BYTE*)(pKeyBlob + 1) + sizeof(DWORD),pTextKey,dwKeyLen);

	CBR(CryptImportKey(CryptoGetProvider(),
		(BYTE*)pKeyBlob,
		sizeof(PUBLICKEYSTRUC) + sizeof(DWORD)+dwKeyLen,
		NULL,
		0,
		&hKey));

	CBR(CryptSetKeyParam(hKey,KP_IV,pIV,0));

	CBR(CryptDecrypt(hKey,NULL,TRUE,0,pDataToDecrypt,pdwDataLen));
Error:
	if(pKeyBlob)
	{
		FREE(NULL,pKeyBlob); 
	}
	if(hKey)
	{
		CryptDestroyKey(hKey);
	}

	return bRet;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL CryptoEncryptBufferWithKey(ALG_ID alg,
								BYTE *pbDataToEncrypt,
								DWORD *pdwDataLen,
								DWORD dwBufLen,
								const BYTE *pTextKey,
								DWORD dwKeyLen,
								const BYTE *pIV) // must be size indicated by algorithm
{
	BOOL bRet = TRUE;
	HCRYPTKEY hKey = NULL;
	PUBLICKEYSTRUC *pKeyBlob = NULL;

	ASSERT_RETFALSE(sbInitialized);

	pKeyBlob = (PUBLICKEYSTRUC*)MALLOCZ(NULL,sizeof(PUBLICKEYSTRUC) + dwKeyLen + sizeof(DWORD));

	CBRA(pKeyBlob);

	pKeyBlob->bType = PLAINTEXTKEYBLOB;
	pKeyBlob->bVersion = CUR_BLOB_VERSION;
	pKeyBlob->aiKeyAlg = alg;

	memcpy(pKeyBlob + 1, &dwKeyLen,sizeof(DWORD));
	memcpy((BYTE*)(pKeyBlob + 1) + sizeof(DWORD),pTextKey,dwKeyLen);

	CBRA(CryptImportKey(CryptoGetProvider(),
		(BYTE*)pKeyBlob,
		sizeof(PUBLICKEYSTRUC) + sizeof(DWORD)+dwKeyLen,
		NULL,
		0,
		&hKey));

	CBRA(CryptSetKeyParam(hKey,KP_IV,pIV,0));

	CBRA(CryptEncrypt(hKey, NULL, TRUE, 0, pbDataToEncrypt, pdwDataLen, dwBufLen ))
Error:
	if(pKeyBlob)
	{
		FREE(NULL,pKeyBlob); 
	}
	if(hKey)
	{
		CryptDestroyKey(hKey);
	}

	return bRet;
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void CryptoCleanup(void)
{
    if(!sbInitialized)
		return;
    CryptReleaseContext(hVerifyContextProvider,0);
}