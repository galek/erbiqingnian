//----------------------------------------------------------------------------
// FILE: wordfilter.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __WORDFILTER_H_
#define __WORDFILTER_H_

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

BOOL ChatFilterInitForApp(
	void);
	
void ChatFilterFreeForApp(
	void);

void ChatFilterStringInPlace(
	WCHAR *puszString);

BOOL NameFilterInitForApp(
	void);

void NameFilterFreeForApp(
	void);

BOOL NameFilterString(
	WCHAR *szName);
		
struct FILTER_WORD_DEFINITION
{
	char	szWord[256];
	BOOL	bAllowPrefixed;
	BOOL	bAllowSuffixed;
};

#endif