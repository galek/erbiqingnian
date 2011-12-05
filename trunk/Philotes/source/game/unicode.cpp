//----------------------------------------------------------------------------
// FILE: unicode.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "unicode.h"

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR *UnicodeFileGetEncoding(
	const BYTE *pFileData,
	DWORD dwFileSize,
	int &nNumChars,
	BOOL bValidateUnicodeHeader /*= TRUE*/)
{	
	if (bValidateUnicodeHeader)
	{
		ASSERT_RETNULL(dwFileSize >= 2);
		if (pFileData[0] == 0xff && pFileData[1] == 0xfe)
		{
			nNumChars = (dwFileSize - 2) / 2;
			return (WCHAR *)(pFileData + 2);
		}
	}
		
	else
	{
		nNumChars = dwFileSize / 2;
		return (WCHAR *)pFileData;
	}
	
	ASSERT_RETNULL(0);
}
