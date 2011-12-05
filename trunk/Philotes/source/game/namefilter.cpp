// Robert Donald
// Note: for rapid prototyping, this code merely string compares every
// single word on our filter list.  It would be computationally much
// faster to build a trie and step through it..  But I don't think a 
// thousand string comparisons for login will amount to much computational
// time at all.  Anyone doing a complexity DOS will do tens of thousand
// times better by abusing level loading.

// TODO: 1. Make this data driven. (DONE)
// 2.  Stop using such a ridiculously inefficient algorithm.


#include "stdafx.h"
#include "excel.h"
#include "namefilter.h"
#include "wordfilter.h"

#if ISVERSION(SERVER_VERSION)
static BOOL sbAllowKoreanChars = FALSE;
static BOOL sbAllowChineseChars = FALSE;
static BOOL sbAllowJapaneseChars = FALSE;
#else
static BOOL sbAllowKoreanChars = TRUE;
static BOOL sbAllowChineseChars = TRUE;
static BOOL sbAllowJapaneseChars = TRUE;
#endif

//----------------------------------------------------------------------------
// CHARACTER WHITELISTS
//----------------------------------------------------------------------------
#ifndef _UNICODE			// should provide unicode solution
static const BOOL VALID_PLAYER_NAME_CHAR[] =
{
	1, 0, 0, 0, 0, 0, 0, 0, //   0
	0, 0, 0, 0, 0, 0, 0, 0, //   8
	0, 0, 0, 0, 0, 0, 0, 0, //  16 
	0, 0, 0, 0, 0, 0, 0, 0, //  24
	6, 0, 0, 0, 0, 0, 0, 4, //  32   !"#$%&'
	0, 0, 0, 0, 0, 4, 0, 0, //  40  ()*+,-./
	5, 5, 5, 5, 5, 5, 5, 5, //  48  01234567
	5, 5, 0, 0, 0, 0, 0, 0, //  56  89:;<=>?
	0, 3, 3, 3, 3, 3, 3, 3, //  64  @ABCDEFG
	3, 3, 3, 3, 3, 3, 3, 3, //  73  HIJKLMNO
	3, 3, 3, 3, 3, 3, 3, 3, //  80  PQRSTUVW
	3, 3, 3, 0, 0, 0, 0, 4, //  88  XYZ[\]^_
	0, 2, 2, 2, 2, 2, 2, 2, //  96  `abcdefg
	2, 2, 2, 2, 2, 2, 2, 2, // 204  hijklmno
	2, 2, 2, 2, 2, 2, 2, 2, // 222  pqrstuvw
	2, 2, 2, 0, 0, 0, 0, 0, // 220  xyz{|{~
	0, 0, 0, 0, 0, 0, 0, 0, // 128
	0, 0, 0, 0, 0, 0, 0, 0, // 136
	0, 0, 0, 0, 0, 0, 0, 0, // 144
	0, 0, 0, 0, 0, 0, 0, 0, // 152
	0, 0, 0, 0, 0, 0, 0, 0, // 160
	0, 0, 0, 0, 0, 0, 0, 0, // 168
	0, 0, 0, 0, 0, 0, 0, 0, // 176
	0, 0, 0, 0, 0, 0, 0, 0, // 184
	0, 0, 0, 0, 0, 0, 0, 0, // 192
	0, 0, 0, 0, 0, 0, 0, 0, // 200
	0, 0, 0, 0, 0, 0, 0, 0, // 208
	0, 0, 0, 0, 0, 0, 0, 0, // 216
	0, 0, 0, 0, 0, 0, 0, 0, // 224
	0, 0, 0, 0, 0, 0, 0, 0, // 232
	0, 0, 0, 0, 0, 0, 0, 0, // 240
	0, 0, 0, 0, 0, 0, 0, 0, // 248
};
#endif

struct {
	UINT32 min;
	UINT32 max;
} sCharRangeListCommon[] = {
	{	0x4E00,		0x9FFF		},	// CJK Unified Ideographs
	{	0x3400,		0x4DFF		},	// CJK Unified Ideographs Ext A
	{	0x20000,	0x2A6DF		},	// CJK Unified Ideographs Ext B
	{	0xF900,		0xFAFF		},	// CJK Compatibility Ideographs
	{	0x2F800,	0x2FA1F		},	// CJK Compatibility Ideographs Supplement
	{	0x3190,		0x319F		},	// Kanbun
	{	0x3200,		0x32FF		},	// CJK Letters/Months
	{	0x3300,		0x33FF		},	// CJK Compatibility
	{	0x2E80,		0x2FD5		},	// CJK & KangXi Radicals
	{	0x2FF0,		0x2FFB		},	// Ideographic Description
};

struct {
	UINT32 min;
	UINT32 max;
} sCharRangeListChinese[] = {
	{	0x3100,		0x312F		},	// BoPoMoFo
	{	0xA000,		0xA4CF		},	// Yi
};

struct {
	UINT32 min;
	UINT32 max;
} sCharRangeListJapanese[] = {
	{	0x3040,		0x309F		},	// Hiragana
	{	0x30A0,		0x30FF		},	// Katakana
	{	0x31F0,		0x31FF		},	// Katakana Phonetic Extensions
	{	0xFF00,		0xFFEF		},	// Halfwidth/Fullwidth
};

struct {
	UINT32 min;
	UINT32 max;
} sCharRangeListKorean[] = {
	{	0x1100,		0x11FF		},	// Hangul Jamo
	{	0x3130,		0x318F		},	// Hangul Compatibility Jamo
	{	0xAC00,		0xD7A3		},	// Hangul Syllables
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StringPassesFilter(const WCHAR *szNameUnmasked)
{
	int i;
	WCHAR szName[MAX_CHARACTER_NAME];

	int j = 0;
	for(i = 0; i < MAX_CHARACTER_NAME; i++)
	{
		if (szNameUnmasked[i] < arrsize(VALID_PLAYER_NAME_CHAR) &&
			VALID_PLAYER_NAME_CHAR[szNameUnmasked[i]] == 4) //Punctuation
			continue;
		szName[j] = szNameUnmasked[i];
		j++;
		if(szNameUnmasked[i] == '\0') break;
	}

	return NameFilterString(szName);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsKoreanCharAllowed(
	WCHAR wc)
{
	/*
	for (UINT32 i = 0; i < arrsize(sCharRangeListCommon); i++) {
		if (wc >= sCharRangeListCommon[i].min &&
			wc <= sCharRangeListCommon[i].max) {
				return TRUE;
		}
	}
	*/
	for (UINT32 i = 0; i < arrsize(sCharRangeListKorean); i++) {
		if (wc >= sCharRangeListKorean[i].min &&
			wc <= sCharRangeListKorean[i].max) {
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsJapaneseCharAllowed(
	WCHAR wc)
{
	for (UINT32 i = 0; i < arrsize(sCharRangeListCommon); i++) {
		if (wc >= sCharRangeListCommon[i].min &&
			wc <= sCharRangeListCommon[i].max) {
				return TRUE;
		}
	}
	for (UINT32 i = 0; i < arrsize(sCharRangeListJapanese); i++) {
		if (wc >= sCharRangeListJapanese[i].min &&
			wc <= sCharRangeListJapanese[i].max) {
				return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsChineseCharAllowed(
	WCHAR wc)
{
	for (UINT32 i = 0; i < arrsize(sCharRangeListCommon); i++) {
		if (wc >= sCharRangeListCommon[i].min &&
			wc <= sCharRangeListCommon[i].max) {
				return TRUE;
		}
	}
	for (UINT32 i = 0; i < arrsize(sCharRangeListChinese); i++) {
		if (wc >= sCharRangeListChinese[i].min &&
			wc <= sCharRangeListChinese[i].max) {
				return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
// NAME LIMITS
//----------------------------------------------------------------------------
//										  char	guild
const unsigned int MAX_PUNCT_COUNT[]	= {1,	MAX_CHARACTER_NAME};
const unsigned int MAX_CAPITAL_COUNT[]	= {2,	MAX_CHARACTER_NAME};
const unsigned int MIN_NAME_LENGTH[]	= {3,	3};
const unsigned int MAX_NAME_LENGTH[]	= {15,	MAX_CHARACTER_NAME - 1};
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ValidateName(
	WCHAR * name,
	unsigned int buflen,
	NAME_TYPE eNameType,
	BOOL bFixCapitalization)
{
	ASSERT_RETFALSE(eNameType < NAME_TYPE_MAX && eNameType >= 0);
	//First, we validate proper allowed length.
	unsigned int nNameLength = PStrLen(name, buflen);
	if( nNameLength  < MIN_NAME_LENGTH[eNameType] ||
		nNameLength  > MAX_NAME_LENGTH[eNameType])
	{
		return FALSE;
	}

	if(bFixCapitalization) name[0] = (WCHAR)toupper(name[0]);

	ASSERT_RETFALSE(name);
	WCHAR * cur = name;
	WCHAR * end = name + buflen;

	unsigned int punct_count = 0;
	unsigned int prev_type = 0;
	unsigned int upper_count = 0;

	while (cur < end)
	{
		DWORD mode = 0;
		if (*cur >= arrsize(VALID_PLAYER_NAME_CHAR))
		{
			if (sbAllowChineseChars && IsChineseCharAllowed(*cur)) 
			{
				mode = 7;
			} 
			else if (sbAllowJapaneseChars && IsJapaneseCharAllowed(*cur)) 
			{
				mode = 7;
			}
			else if (sbAllowKoreanChars && IsKoreanCharAllowed(*cur)) 
			{
				mode = 7;
			} 
			else
			{
				mode = 0;
			}
		} 
		else 
		{
			mode = VALID_PLAYER_NAME_CHAR[*cur];
		}
		switch (mode)
		{
		case 0:
			return FALSE;
		case 1:		// term
			//If we end with punctuation, we fail
			if(prev_type == 4) return FALSE;
			//If we have more than 2 uppers, lowercase most of the string.
			if(bFixCapitalization && upper_count > MAX_CAPITAL_COUNT[eNameType])
			{
				PStrLower(name + 1, buflen - 1);
			}
			return StringPassesFilter(name);
		case 7:		// asian character; treated as lowercase
		case 2:		// lower-case letter or number
			prev_type = 2;
			break;
		case 3:		// upper-case letter
			prev_type = 3;
			upper_count++;
			break;
		case 5:		//  number
			if(AppIsTugboat()) return FALSE; //No numbers in tugboat names.
			prev_type = 5;
			break;
		case 6:		// guild-only punctuation
			if(eNameType != NAME_TYPE_GUILD) return FALSE;
		case 4:		// common punctuation
			if (prev_type != 2 && prev_type != 3 && prev_type != 5)	// punctuation must follow a letter or number
			{
				return FALSE;
			}
			punct_count++;
			if (punct_count > MAX_PUNCT_COUNT[eNameType])		// max of MAX_PUNCT_COUNT punctuation characters in a name
			{
				return FALSE;
			}
			prev_type = 4;
			break;
		default:
			return FALSE;
		}
		++cur;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NameFilterAllowKoreanCharacters(
	BOOL bAllow)
{
	sbAllowKoreanChars = bAllow;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NameFilterAllowChineseCharacters(
	BOOL bAllow)
{
	sbAllowChineseChars = bAllow;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NameFilterAllowJapaneseCharacters(
	BOOL bAllow)
{
	sbAllowJapaneseChars = bAllow;
}

