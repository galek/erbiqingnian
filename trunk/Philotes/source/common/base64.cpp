

#define INV_ARRAY_SIZE sizeof(BYTE)

static LPCSTR listBase64Char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline CHAR sGetCharAt(LPCTSTR pSrc, UINT32 dwSrcLen, UINT32 i)
{
	return (i < dwSrcLen ? pSrc[i] : '\0');
}

BOOL Base64Encode(LPCSTR pSrc, UINT32 dwSrcLen, LPSTR pDst, UINT32 dwDstLen)
{
	UINT32 dwDstIdx = 0;
	ASSERT_RETFALSE(dwSrcLen > 0);

	for (UINT32 i = 0; i < dwSrcLen; i += 3) {
		if ((i > 0) && ((i / 3 * 4) % 76 == 0)) {
			ASSERT_RETFALSE(dwDstIdx + 2 <= dwDstLen);
			pDst[dwDstIdx++] = '\r';
			pDst[dwDstIdx++] = '\n';
		}
		ASSERT_RETFALSE(dwDstIdx + 4 <= dwDstLen);

		UINT32 n = (sGetCharAt(pSrc, dwSrcLen, i) << 16) + (sGetCharAt(pSrc, dwSrcLen, i+1) << 8) + sGetCharAt(pSrc, dwSrcLen, i+2);
		pDst[dwDstIdx++] = listBase64Char[(n >> 18) & 63];
		pDst[dwDstIdx++] = listBase64Char[(n >> 12) & 63];
		pDst[dwDstIdx++] = listBase64Char[(n >> 6) & 63];
		pDst[dwDstIdx++] = listBase64Char[n & 63];
	}

	ASSERT_RETFALSE(dwDstIdx < dwDstLen);
	pDst[dwDstIdx] = '\0';

	UINT32 iPadCount = (dwSrcLen * 2) % 3;
	for (UINT32 i = dwDstIdx - iPadCount; i < dwDstIdx; i++) {
		pDst[i] = '=';
	}

	return TRUE;
}

static BOOL sIsBase64Char(CHAR c)
{
	UINT32 dwCountBase64Char = PStrLen(listBase64Char);
	for (UINT32 i = 0; i < dwCountBase64Char; i++) {
		if (listBase64Char[i] == c) {
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Base64Decode(LPCSTR pSrc, UINT32 dwSrcLen, LPSTR pDst, UINT32& dwDstLen)
{
	ASSERT_RETFALSE(dwSrcLen > 0);

	UINT32 iPadCount = 0;
	if (pSrc[dwSrcLen-1] == '=') {
		iPadCount++;
		if (dwSrcLen > 1 &&  pSrc[dwSrcLen-2] == '=') {
			iPadCount++;
		}
	}

	UINT32 dwCountBase64Char = PStrLen(listBase64Char);
	BYTE listBase64Inv[INV_ARRAY_SIZE];

	for (UINT32 i = 0; i < INV_ARRAY_SIZE; i++) {
		listBase64Inv[i] = 0;
	}
	for (BYTE i = 0; i < dwCountBase64Char; i++) {
		listBase64Inv[listBase64Char[i]] = i;
	}

	CHAR tmpChar[4];
	UINT32 dwDstIdx = 0, dwSrcIdx = 0;
	while (dwSrcIdx < dwSrcLen) {
		ASSERT_RETFALSE(dwDstIdx + 3 <= dwDstLen);
		for (UINT32 i = 0; i < 4; i++) {
			while (TRUE) {
				ASSERT_RETFALSE(dwSrcIdx < dwSrcLen);
				CHAR c = pSrc[dwSrcIdx++];
				if (c == '=' && (dwSrcLen - dwSrcIdx < iPadCount)) {
					c = 'A';
				}
				if (sIsBase64Char(c)) {
					tmpChar[i] = c;
					break;
				}
			}
		}
		UINT32 n = (listBase64Inv[tmpChar[0]] << 18) + (listBase64Inv[tmpChar[1]] << 12) +
			(listBase64Inv[tmpChar[2]] << 6) + (listBase64Inv[tmpChar[3]]);
		pDst[dwDstIdx++] = (CHAR)((n >> 16) & 255);
		pDst[dwDstIdx++] = (CHAR)((n >> 8) & 255);
		pDst[dwDstIdx++] = (CHAR)(n & 255);
	}
	ASSERT_RETFALSE(dwDstIdx - iPadCount + 1 <= dwDstLen);
	pDst[dwDstIdx - iPadCount] = '\0';
	dwDstLen = dwDstIdx - iPadCount;

	return TRUE;
}