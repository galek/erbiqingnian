// ---------------------------------------------------------------------------
// FILE:	cryptoapi.h
// DESC:	CryptoAPI wrappers
//
// Modified : $DateTime: 2007/12/11 11:49:11 $
// by       : $Author: pmason $
//
// Copyright 2006, Flagship Studios
// ---------------------------------------------------------------------------

#pragma once
#include <wincrypt.h>

BOOL CryptoInit(void);
void CryptoCleanup(void);

HCRYPTPROV CryptoGetProvider(void);

BOOL CryptoHashBuffer(ALG_ID hashAlg,
                      __in __bcount(dwBufLen) const BYTE *pBuffer,
                      DWORD dwBufLen,
                      __in __notnull BYTE **ppOutput,
                      __in __notnull DWORD *pdwOutLen);

void CryptoFreeHashBuffer(BYTE *pBuffer);

BOOL CryptoDeriveKeyFromPassword(ALG_ID     hashAlg,
                                 ALG_ID     encryptAlg,
                                 DWORD      dwKeyLen,
                                 __in __notnull const char      *password,
                                 __out HCRYPTKEY *phKey);


void CryptoDestroyKey(HCRYPTKEY hKey);
BOOL CryptoDecryptBufferWithKey(ALG_ID alg,
                                __in __notnull BYTE *pDataToDecrypt,
                                __in __notnull DWORD *pdwDataLen,
                                __in __notnull __bcount(dwKeyBlobLen) const BYTE *pKeyBlob,
                                DWORD dwKeyBlobLen,
                                const BYTE *pIV);

BOOL CryptoDecryptBufferWithKeyNoAssert(ALG_ID alg,
								__in __notnull BYTE *pDataToDecrypt,
								__in __notnull DWORD *pdwDataLen,
								__in __notnull __bcount(dwKeyBlobLen) const BYTE *pKeyBlob,
								DWORD dwKeyBlobLen,
								const BYTE *pIV);

BOOL CryptoEncryptBufferWithKey(ALG_ID alg,
								__in __notnull BYTE *pDataToDecrypt,
								__in __notnull DWORD *pdwDataLen,
								DWORD dwBufLen,
								__in __notnull __bcount(dwKeyBlobLen) const BYTE *pKeyBlob,
								DWORD dwKeyBlobLen,
								const BYTE *pIV);
