#ifndef _BASE_64_
#define _BASE_64_

BOOL Base64Encode(LPCSTR pSrc, UINT32 dwSrcLen, LPSTR pDst, UINT32 dwDstLen);
BOOL Base64Decode(LPCSTR pSrc, UINT32 dwSrcLen, LPSTR pDst, UINT32& dwDstLen);

#endif


