//----------------------------------------------------------------------------
// excel_common.cpp
//----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// INCLUDE
//-----------------------------------------------------------------------------

#include "excel_private.h"
#include "filepaths.h"
#include "language.h"
#include "pakfile.h"
#include "prime.h"
#include "stringtable.h"
#include "stringtable_common.h"
#include "dictionary.h"
#include "crc.h"
#ifndef _DATATABLES_H_
#include "datatables.h"
#endif
#if ISVERSION(SERVER_VERSION)
#include "excel_common.cpp.tmh"
#endif

//----------------------------------------------------------------------------
// DEBUG CONSTANTS
//----------------------------------------------------------------------------
#define EXCEL_CRC_TRACE							0							// use to debug crc differences


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define EXCEL_TABLE_VERSION						(15 + GLOBAL_FILE_VERSION)	// table file version
#define EXCEL_TABLE_MAGIC						'hexc'						// magic number for cooked file
#define EXCEL_MAX_STATS_BUFSIZE					4096						// maximum size for single default statslist cooked
#define UNITTYPES_TABLE							"unittypes"					// used to determine table for FIELDTYPE_UNITTYPES lookup
#define IGNORE_COLUMN_CHARACTER					(L'*')

//#define EXCEL_PACKAGE_VERSION_MAJOR_PATCH_1		10
#define EXCEL_PACKAGE_VERSION_MAJOR_PATCH_2		20

// tokens for VERSION_APP
STR_DICT g_VersionAppTokens[] =
{
	{ "hellgate",			EXCEL_APP_VERSION_HELLGATE			},
	{ "tugboat",			EXCEL_APP_VERSION_TUGBOAT			},
	{ "mythos",				EXCEL_APP_VERSION_TUGBOAT			},
	{ "all",				EXCEL_APP_VERSION_ALL				},
	{ "!hellgate",			EXCEL_APP_VERSION_HELLGATE			},
	{ "!tugboat",			EXCEL_APP_VERSION_TUGBOAT			},
	{ "!mythos",			EXCEL_APP_VERSION_TUGBOAT			},
	{ NULL,					0									},
};

// tokens for VERSION_PACKAGE
STR_DICT g_VersionPackageFlagTokens[] =
{
	{ "development",		EXCEL_PACKAGE_VERSION_DEVELOPMENT },
	{ "demo_only",			EXCEL_PACKAGE_VERSION_DEMO },
	{ "demo",				EXCEL_PACKAGE_VERSION_DEMO | EXCEL_PACKAGE_VERSION_BETA | EXCEL_PACKAGE_VERSION_RELEASE | EXCEL_PACKAGE_VERSION_ONGOING | EXCEL_PACKAGE_VERSION_EXPANSION },
	{ "beta",				EXCEL_PACKAGE_VERSION_BETA | EXCEL_PACKAGE_VERSION_RELEASE | EXCEL_PACKAGE_VERSION_ONGOING | EXCEL_PACKAGE_VERSION_EXPANSION },
	{ "release",			EXCEL_PACKAGE_VERSION_BETA | EXCEL_PACKAGE_VERSION_RELEASE | EXCEL_PACKAGE_VERSION_ONGOING | EXCEL_PACKAGE_VERSION_EXPANSION },
	{ "ongoing",			EXCEL_PACKAGE_VERSION_ONGOING | EXCEL_PACKAGE_VERSION_EXPANSION },
	{ "expansion",			EXCEL_PACKAGE_VERSION_EXPANSION },
	{ "all",				EXCEL_PACKAGE_VERSION_ALL },
	{ "!development",		EXCEL_PACKAGE_VERSION_DEVELOPMENT },
	{ "!demo",				EXCEL_PACKAGE_VERSION_DEMO },
	{ "!beta",				EXCEL_PACKAGE_VERSION_BETA },
	{ "!release",			EXCEL_PACKAGE_VERSION_RELEASE },
	{ "!ongoing",			EXCEL_PACKAGE_VERSION_ONGOING },
	{ "!expansion",			EXCEL_PACKAGE_VERSION_EXPANSION },
	{ NULL,					0 },
};
STR_DICT g_VersionPackageTokens[] =
{
	{ "development",		EXCEL_PACKAGE_VERSION_DEVELOPMENT },
	{ "demo",				EXCEL_PACKAGE_VERSION_DEMO },
	{ "beta",				EXCEL_PACKAGE_VERSION_BETA },
	{ "release",			EXCEL_PACKAGE_VERSION_RELEASE },
	{ "ongoing",			EXCEL_PACKAGE_VERSION_ONGOING },
	{ "expansion",			EXCEL_PACKAGE_VERSION_EXPANSION },
	{ NULL,					0 },
};

EXCEL g_Excel;

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#if EXCEL_MT
 #define	EXCEL_LOCK(c, d)					c UIDEN(autolock)(d)
 #define	EXCEL_LOCK_INIT(d)					d.Init()
 #define	EXCEL_LOCK_FREE(d)					d.Free()
#else
 #define	EXCEL_LOCK(c, d)					
 #define	EXCEL_LOCK_INIT(d)
 #define	EXCEL_LOCK_FREE(d)
#endif

#define TABLE_DATA_READ_LOCK(table)				EXCEL_LOCK(CWRAutoLockReader, (CReaderWriterLock_NS_WP *)&table->m_CS_Data)
#define TABLE_DATA_WRITE_LOCK(table)			EXCEL_LOCK(CWRAutoLockWriter, &table->m_CS_Data)
#define TABLE_INDEX_READ_LOCK(table)			EXCEL_LOCK(CWRAutoLockReader, (CReaderWriterLock_NS_WP *)&table->m_CS_Index)
#define TABLE_INDEX_WRITE_LOCK(table)			EXCEL_LOCK(CWRAutoLockWriter, &table->m_CS_Index)


#if EXCEL_CRC_TRACE
#define ExcelCRCTrace(...)						trace(__VA_ARGS__)
#else
#define ExcelCRCTrace(...)
#endif


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
static char EXCEL_BLANK[1] = { 0 };

#if ISVERSION(DEVELOPMENT)
#define ExcelWarn(fmt, ...) { GLOBAL_DEFINITION* global = DefinitionGetGlobal(); if ( global && global->nDebugOutputLevel <= OP_HIGH ) ASSERTV_MSG(fmt, __VA_ARGS__) }
#else
#if defined(HELLGATE)
#define ExcelWarn(fmt, ...) ASSERTV_MSG(fmt, __VA_ARGS__)
#elif defined(TUGBOAT)
#define ExcelWarn(fmt, ...) ExcelLog(fmt, __VA_ARGS__)
#else
#define ExcelWarn(fmt, ...) ExcelLog(fmt, __VA_ARGS__)
#endif
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD sGetVersionAppFlag(
	void)
{
	APP_GAME eAppGame = AppGameGet();
	DWORD dwFlag = 0;
	switch (eAppGame)
	{
		case APP_GAME_HELLGATE: dwFlag = EXCEL_APP_VERSION_HELLGATE;	break;
		case APP_GAME_TUGBOAT:	dwFlag = EXCEL_APP_VERSION_TUGBOAT;		break;
		case APP_GAME_ALL:		dwFlag = EXCEL_APP_VERSION_HELLGATE | EXCEL_APP_VERSION_TUGBOAT;	break;
	}
	return dwFlag;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LOAD_MANIFEST::EXCEL_LOAD_MANIFEST(
	void)
{
	m_VersionApp = sGetVersionAppFlag();

#if ISVERSION(RETAIL_VERSION)

	// we'll load the manifest from the excel data
	m_VersionPackage = EXCEL_PACKAGE_VERSION_ALL;

	m_VersionMajorMin = 0;
	m_VersionMajorMax = EXCEL_DATA_VERSION::EXCEL_MAX_VERSION;
	m_VersionMinorMin = 0;
	m_VersionMinorMax = EXCEL_DATA_VERSION::EXCEL_MAX_VERSION;
#else

	m_VersionMajorMin = gtAppCommon.m_wExcelVersionMajor;
	m_VersionMajorMax = gtAppCommon.m_wExcelVersionMajor;
	m_VersionMinorMin = gtAppCommon.m_wExcelVersionMinor;
	m_VersionMinorMax = gtAppCommon.m_wExcelVersionMinor;

	#if ISVERSION(DEMO_VERSION)
		m_VersionPackage = EXCEL_PACKAGE_VERSION_DEMO;
		return;
	#else //!DEMO
		if (gtAppCommon.m_dwExcelManifestPackage)
		{
			m_VersionPackage = gtAppCommon.m_dwExcelManifestPackage;
		}
		else if ( AppIsHellgate() )
		{
			m_VersionPackage = EXCEL_PACKAGE_VERSION_ONGOING;
		} else
		{
			m_VersionPackage = EXCEL_PACKAGE_VERSION_RELEASE;
		}
		m_VersionPackage |= EXCEL_PACKAGE_VERSION_DEVELOPMENT;
	#endif

		if ( gtAppCommon.m_wExcelVersionMajor )
		{
			m_VersionMajorMin = gtAppCommon.m_wExcelVersionMajor;
			m_VersionMajorMax = gtAppCommon.m_wExcelVersionMajor;
		}
		else if (AppIsHellgate())
		{
			m_VersionMajorMin = EXCEL_PACKAGE_VERSION_MAJOR_PATCH_2;
			m_VersionMajorMax = EXCEL_PACKAGE_VERSION_MAJOR_PATCH_2;
		}
		else
		{
			m_VersionMajorMin = 0;
			m_VersionMajorMax = 0;
		}
#endif
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_READ_MANIFEST::EXCEL_READ_MANIFEST(
	void)
{
	static const EXCEL_READ_MANIFEST & read_manifest = Excel().m_ReadManifest;
	
	m_VersionApp = read_manifest.m_VersionApp;
	m_VersionPackage = read_manifest.m_VersionPackage;
	m_VersionMajor = read_manifest.m_VersionMajor;
	m_VersionMinor = read_manifest.m_VersionMinor;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelInitReadManifest(
	void)
{
	EXCEL_READ_MANIFEST & read_manifest = Excel().m_ReadManifest;

	read_manifest.m_VersionApp = sGetVersionAppFlag();

#if ISVERSION(RETAIL_VERSION)

	// we'll load the manifest from the excel data
	read_manifest.m_VersionPackage = EXCEL_PACKAGE_VERSION_ALL;

#else

	#if ISVERSION(DEMO_VERSION)
		read_manifest.m_VersionPackage = EXCEL_PACKAGE_VERSION_DEMO;
	#else
		if (gtAppCommon.m_dwExcelManifestPackage)
		{
			read_manifest.m_VersionPackage = gtAppCommon.m_dwExcelManifestPackage;
		}
		else if ( AppIsHellgate() )
		{
			read_manifest.m_VersionPackage = EXCEL_PACKAGE_VERSION_ONGOING;
		} else
		{
			read_manifest.m_VersionPackage = EXCEL_PACKAGE_VERSION_RELEASE;
		}
		read_manifest.m_VersionPackage |= EXCEL_PACKAGE_VERSION_DEVELOPMENT;
	#endif

#endif

	if ( gtAppCommon.m_wExcelVersionMajor )
		read_manifest.m_VersionMajor = gtAppCommon.m_wExcelVersionMajor;
	else if (AppIsHellgate())
		read_manifest.m_VersionMajor = EXCEL_PACKAGE_VERSION_MAJOR_PATCH_2;
	else
		read_manifest.m_VersionMajor = 0;

	read_manifest.m_VersionMinor = gtAppCommon.m_wExcelVersionMinor;
}


//----------------------------------------------------------------------------
// DICTIONARY of STRINT
//----------------------------------------------------------------------------
struct STRINT_DICT
{
	static const unsigned int BUFSIZE =			16;

	const char **								items;						// in val order
	unsigned int *								index;						// index in str order
	unsigned int								count;
	
	void Init(
		void)
	{
		items = NULL;
		index = NULL;
		count = 0;
	}
	void Free(
		void)
	{
		if (items)
		{
			for (unsigned int ii = 0; ii <  count; ++ii)
			{
				ASSERT_CONTINUE(items[ii]);
				FREE(NULL, items[ii]);
			}
			FREE(NULL, items);
			items = NULL;
		}
		if (index)
		{
			FREE(NULL, index);
			index = NULL;
		}
		count = 0;
	}
	unsigned int GetValByStrClosest(
		const char * str,
		BOOL & bExact) const
	{
		unsigned int min = 0;
		unsigned int max = count;
		unsigned int ii = min + (max - min) / 2;

		while (max > min)
		{
			int cmp = PStrICmp(items[index[ii]], str);
			if (cmp > 0)
			{
				max = ii;
			}
			else if (cmp < 0)
			{
				min = ii + 1;
			}
			else
			{
				bExact = TRUE;
				return index[ii];
			}
			ii = min + (max - min) / 2;
		}
		bExact = FALSE;
		return max;
	}
	unsigned int Add(
		const char * str)
	{
		ASSERT_RETVAL(str, EXCEL_LINK_INVALID);
		BOOL bExact;
		unsigned int link = GetValByStrClosest(str, bExact);
		if (bExact)
		{
			return link;
		}

		if (count % BUFSIZE == 0)
		{
			unsigned int newsize = count + BUFSIZE;
			items = (const char **)REALLOCZ(NULL, items, sizeof(const char *) * newsize);
			index = (unsigned int *)REALLOCZ(NULL, index, sizeof(unsigned int) * newsize);
		}
		unsigned int len = PStrLen(str) + 1;
		char * buf = (char *)MALLOCZ(NULL, len * sizeof(char));
		PStrCopy(buf, str, len);
		items[count] = (const char *)buf;
		if (link < count)
		{
			ASSERT_RETVAL(MemoryMove(index + link + 1, (count - link) * sizeof(unsigned int), index + link, (count - link) * sizeof(unsigned int)),EXCEL_LINK_INVALID);
		}
		index[link] = count;
		count++;
		return count - 1;
	}
	unsigned int GetValByStr(
		const char * str) const
	{
		unsigned int min = 0;
		unsigned int max = count;
		unsigned int ii = min + (max - min) / 2;

		while (max > min)
		{
			int cmp = PStrICmp(items[index[ii]], str);
			if (cmp > 0)
			{
				max = ii;
			}
			else if (cmp < 0)
			{
				min = ii + 1;
			}
			else
			{
				return index[ii];
			}
			ii = min + (max - min) / 2;
		}
		return EXCEL_LINK_INVALID;
	}
	const char * GetStrByVal(
		unsigned int val) const
	{
		ASSERT_RETNULL(val < count);

		return items[val];
	}
	BOOL SaveToBuffer(
		WRITE_BUF_DYNAMIC & fbuf) const
	{
		ASSERT_RETFALSE(count <= 0 || items);
		ASSERT_RETFALSE(count <= 0 || index);
		ASSERT_RETFALSE(fbuf.PushInt(count));
		for (unsigned int ii = 0; ii < count; ++ii)
		{
			const char * str = items[ii];
			if (!str || !str[0])
			{
				ASSERT_RETFALSE(fbuf.PushInt(0));
			}
			else
			{
				int len = PStrLen(str) + 1;
				ASSERT(len > 0);
				ASSERT_RETFALSE(fbuf.PushInt(len));
				ASSERT_RETFALSE(fbuf.PushBuf(str, len));
			}
		}
		return TRUE;
	}
	BOOL ReadFromBuffer(
		BYTE_BUF & fbuf)
	{
		ASSERT(items == NULL);
		ASSERT(index == NULL);
		unsigned int num;
		ASSERT_RETFALSE(fbuf.PopInt(&num));
		for (unsigned int ii = 0; ii < num; ++ii)
		{
			unsigned int len;
			ASSERT_RETFALSE(fbuf.PopInt(&len));
			Add((const char *)fbuf.GetPtr());
			fbuf.SetCursor(fbuf.GetCursor() + len);
		}
		return TRUE;
	}
};


//----------------------------------------------------------------------------
// STATIC FUNCTIONS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// excel parse helper
//----------------------------------------------------------------------------
template <typename T>
struct EXCEL_TOKENIZE
{
	T old;
	T * end;

	EXCEL_TOKENIZE(T * token, unsigned int toklen)
	{
		ASSERT_RETURN(token);
		end = &token[toklen];
		old = *end;
		if (*end != 0)
		{
			*end = 0;
		}
	}
	~EXCEL_TOKENIZE(void)
	{
		ASSERT_RETURN(end);
		if (old != 0)
		{
			*end = old;
		}
	}
};


//----------------------------------------------------------------------------
// read in a token up to tab, newline, or EOF, doesn't null terminate.
// if the delimiter is tab, it eats it.  if the delimiter is newline,
// it only eats it if the newline is the first character in the buffer.
// it always leaves EOF on the buffer.  
//----------------------------------------------------------------------------
template <typename T>
EXCEL_TOK ExcelGetToken(
	T * & cur,
	T * & token,
	unsigned int & toklen)
{
	ASSERT_RETVAL(cur, EXCEL_TOK_EOL);

	if (*cur == (T)('\r'))
	{
		++cur;
	}
	if (*cur == (T)('\n'))
	{
		++cur;
		return EXCEL_TOK_EOL;
	}

	// eat white space
	while (*cur == (T)(' '))
	{
		++cur;
	}
	if (*cur == (T)('`'))			// special character allows us to have leading spaces in data
	{
		++cur;
	}
	
	BOOL isQuoted = FALSE;
	if (*cur == (T)('\"'))
	{
		++cur;
		if (*cur == (T)('`'))		// excel handles ` , with " ` ,"
		{
			++cur;
		}
		isQuoted = TRUE;
	}

	toklen = 0;
	token = cur;

	while (TRUE)
	{
		switch (*cur)
		{
		case 0:
			{
				ASSERT_RETVAL(cur >= token && cur - token <= EXCEL_MAX_FIELD_LEN, EXCEL_TOK_EOF);
				if (isQuoted)
				{
					ASSERT_RETVAL(cur > token, EXCEL_TOK_EOF);
					if (cur[-1] == (T)('\"'))
					{
						toklen = (unsigned int)(cur - token - 1);
						return EXCEL_TOK_TOKEN;
					}
				}
				toklen = (unsigned int)(cur - token);
				return (toklen > 0 ? EXCEL_TOK_TOKEN : EXCEL_TOK_EOF);
			}
		case (T)('\r'):
		case (T)('\t'):
			{
				ASSERT_RETVAL(cur >= token && cur - token <= EXCEL_MAX_FIELD_LEN, EXCEL_TOK_EOF);
				if (isQuoted)
				{
					ASSERT_RETVAL(cur > token, EXCEL_TOK_EOF);
					if (cur[-1] == (T)('\"'))
					{
						toklen = (unsigned int)(cur - token - 1);
						++cur;
						return EXCEL_TOK_TOKEN;
					}
				}
				toklen = (unsigned int)(cur - token);
				++cur;
				return EXCEL_TOK_TOKEN;
			}
		}
		++cur;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
static BOOL sExcelGetCommaDelimitedToken(
	T * & cur,
	T * & token,
	unsigned int & toklen)
{
	ASSERT_RETFALSE(cur);
	
	while (*cur == (T)(' '))
	{
		++cur;
	}

	BOOL retval = FALSE;

	token = cur;
	while (1)
	{
		switch (*cur)
		{
		case 0:
		case (T)('\n'):
		case (T)('\t'):
			{
				toklen = (unsigned int)(cur - token);
				retval = (cur > token);
				goto _done;
			}
		case (T)(','):
			{
				toklen = (unsigned int)(cur - token);
				retval = TRUE;
				cur++;
				goto _done;
			}
		default:
			++cur;
		}
	}

_done:
	ASSERT_DO(toklen <= EXCEL_MAX_FIELD_LEN)
	{
		toklen = 0;
		return FALSE;
	}
	return retval;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
BOOL sExcelGetSpaceDelimitedToken(
	T * & cur,
	T * & token)
{
	ASSERT_RETFALSE(cur);
	
	while (*cur == (T)(' '))
	{
		++cur;
	}

	token = cur;
	while (1)
	{
		switch (*cur)
		{
		case 0:
			{
				return (cur > token);
			}
		case (T)('\n'):
		case (T)('\t'):
		case (T)(' '):
		case (T)(','):
			{
				BOOL retval = (cur > token);
				*cur = 0;
				cur++;
				return retval;
			}
		default:
			++cur;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_TOK ExcelGetStrTableGetToken(
	WCHAR * & cur,
	WCHAR * & token)
{
	unsigned int toklen;
	EXCEL_TOK retval = ExcelGetToken(cur, token, toklen);
	if (retval == EXCEL_TOK_TOKEN)
	{
		ASSERT_RETVAL(token,retval);
		token[toklen] = 0;
	}
	else
	{
		token = NULL;
	}
	return retval;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelStringTableLoadDirectAddHeader( 
	LANGUAGE_COLUMN_HEADER * header, 
	WCHAR * str,
	int index)
{
	if (!str)
	{
		return FALSE;
	}
	WCHAR * cur = str;
	
	if (PStrLen(cur) > 0 && cur[0] == IGNORE_COLUMN_CHARACTER)
	{
		return FALSE;
	}
	
	// separate each of the words to serve as separate attributes
	WCHAR * token;
	while (sExcelGetSpaceDelimitedToken(cur, token))
	{
		// do not consider language names as attributes themselves
		char temp[255];		
		PStrCvt(temp, token, arrsize(temp));
		PStrCopy(header->uszAttribute[header->nNumAttributes++], token, MAX_ATTRIBUTE_NAME_LENGTH);
	}

	header->nColumnIndex = index;
	header->dwAttributeFlags = 0;
		
	return TRUE;		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
static EXCEL_LINK sExcelExpandLink(
	T link)
{
	if (link == (T)(-1))
	{
		return EXCEL_LINK_INVALID;
	}
	return (EXCEL_LINK)link;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
static void PStrFixFilename(
	T * filename,
	unsigned int len)
{
	PStrFixPathBackslash(filename);
	PStrLower(filename, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * sExcelStringTableLookupVerbose(
	int strindex = INVALID_LINK)
{
	static const WCHAR * strUnknown = L"???";
	int nLanguage = LanguageGetCurrent();
	const WCHAR * str = StringTableCommonGetStringByIndexVerbose(nLanguage, strindex, 0, 0, NULL, NULL);
	ASSERT_RETVAL(str, strUnknown);
	return str;
}


//----------------------------------------------------------------------------
// EXCEL_DSTRING_POOL:: FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T, unsigned int SIZE>
void EXCEL_DSTRING_POOL<T, SIZE>::Free(
	void)
{
	EXCEL_DSTRING_BUCKET<T, SIZE> * next = m_StringBucket;
	EXCEL_DSTRING_BUCKET<T, SIZE> * cur = NULL;
	while ((cur = next) != NULL)
	{
		next = cur->m_Next;

		if (cur->m_Strings)
		{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
			FREE(g_StaticAllocator, cur->m_Strings);
#else		
			FREE(NULL, cur->m_Strings);
#endif
		}
		FREE(NULL, cur);
	}
	m_StringBucket = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T, unsigned int SIZE>
T * EXCEL_DSTRING_POOL<T, SIZE>::GetSpace(
	unsigned int len)
{
	if (m_StringBucket)
	{
		if (m_StringBucket->m_Size - m_StringBucket->m_Used >= len)
		{
			ASSERT_RETNULL(m_StringBucket->m_Strings);
			unsigned int cur = m_StringBucket->m_Used;
			m_StringBucket->m_Used += len;
			return m_StringBucket->m_Strings + cur;
		}
	}
	unsigned int allocsize = MAX(len, SIZE);
	EXCEL_DSTRING_BUCKET<T, SIZE> * bucket = (EXCEL_DSTRING_BUCKET<T, SIZE> *)MALLOCZ(NULL, sizeof(EXCEL_DSTRING_BUCKET<T, SIZE>));
	bucket->m_Strings = (T *)MALLOCZ(NULL, sizeof(T) * allocsize);
	bucket->m_Size = allocsize;
	bucket->m_Used = len;
	bucket->m_Next = m_StringBucket;
	m_StringBucket = bucket;
	
	return bucket->m_Strings;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T, unsigned int SIZE>
const T * EXCEL_DSTRING_POOL<T, SIZE>::AddString(
	const T * str,
	unsigned int len)
{
	ASSERT_RETNULL(str);
	ASSERT_RETNULL(str[len] == 0);

	T * buf = GetSpace(len + 1);
	ASSERT_RETNULL(buf);
	PStrCopy(buf, str, len + 1);
	return buf;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T, unsigned int SIZE>
const T * EXCEL_DSTRING_POOL<T, SIZE>::AddString(
	const T * str)
{
	unsigned int len = PStrLen(str);
	return AddString(str, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T, unsigned int SIZE>
unsigned int EXCEL_DSTRING_POOL<T, SIZE>::GetStringOffset(
	const T * str)
{
	if (!str || str[0] == 0)
	{
		return EXCEL_LINK_INVALID;
	}

	unsigned int offset = 0;
	EXCEL_DSTRING_BUCKET<T, SIZE> * cur = m_StringBucket;
	while (cur)
	{
		if (str >= cur->m_Strings && str < cur->m_Strings + cur->m_Used)
		{
			offset += (unsigned int)(str - cur->m_Strings);
			return offset;
		}
		offset += cur->m_Used;
		cur = cur->m_Next;
	}
	ASSERT_MSG("EXCEL ERROR:  string not found in dynamic string pool.");
	return EXCEL_LINK_INVALID;
}


//----------------------------------------------------------------------------
// return the second string bucket (or the first if no second bucket)
//----------------------------------------------------------------------------
template <typename T, unsigned int SIZE>
T * EXCEL_DSTRING_POOL<T, SIZE>::GetReadBuffer(
	unsigned int & size) const
{
	if (!m_StringBucket)
	{
		size = 0;
		return NULL;
	}
	if (!m_StringBucket->m_Next)
	{
		size = m_StringBucket->m_Used;
		return m_StringBucket->m_Strings;
	}
	size = m_StringBucket->m_Next->m_Used;
	return m_StringBucket->m_Next->m_Strings;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T, unsigned int SIZE>
BOOL EXCEL_DSTRING_POOL<T, SIZE>::WriteToBuffer(
	WRITE_BUF_DYNAMIC & fbuf) const
{
	unsigned int totalsize = 0;
	EXCEL_DSTRING_BUCKET<T, SIZE> * cur = m_StringBucket;
	while (cur)
	{
		totalsize += cur->m_Used;
		cur = cur->m_Next;
	}

	ASSERT_RETFALSE(fbuf.PushInt(totalsize));
	if (totalsize <= 0)
	{
		return TRUE;
	}
	cur = m_StringBucket;
	while (cur)
	{
		ASSERT_RETFALSE(fbuf.PushBuf(cur->m_Strings, cur->m_Used * sizeof(T)));
		cur = cur->m_Next;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T, unsigned int SIZE>
BOOL EXCEL_DSTRING_POOL<T, SIZE>::ReadFromBuffer(
	BYTE_BUF & fbuf)
{
	unsigned int totalsize = 0;
	ASSERT_RETFALSE(fbuf.PopInt(&totalsize));
	if (totalsize <= 0)
	{
		return TRUE;
	}

	EXCEL_DSTRING_BUCKET<T, SIZE> * bucket = (EXCEL_DSTRING_BUCKET<T, SIZE> *)MALLOCZ(NULL, sizeof(EXCEL_DSTRING_BUCKET<T, SIZE>));
	
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	bucket->m_Strings = (T *)MALLOCZ(g_StaticAllocator, sizeof(T) * totalsize);
#else	
	bucket->m_Strings = (T *)MALLOCZ(NULL, sizeof(T) * totalsize);
#endif
	
	bucket->m_Size = totalsize;
	bucket->m_Used = totalsize;
	ASSERT_RETFALSE(fbuf.PopBuf(bucket->m_Strings, sizeof(T) * totalsize));

	// insert after top bucket (if one exists)
	if (!m_StringBucket)
	{
		m_StringBucket = bucket;
		return TRUE;
	}

	bucket->m_Next = m_StringBucket->m_Next;
	m_StringBucket->m_Next = bucket;
	return TRUE;
}


//----------------------------------------------------------------------------
// EXCEL_FIELD:: FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sFieldReportErrorsForApp(
	struct EXCEL_TABLE * table, 
	unsigned int row,
	const EXCEL_FIELD &tExcelField)
{	
	EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(table->m_Data[row]);			

#if ISVERSION(DEVELOPMENT)
	if(AppIsHellgate())
	{
		if ((tExcelField.m_dwFlags & EXCEL_FIELD_MYTHOS_ONLY))
		{
			return FALSE;
		}
		if (!(version->m_VersionApp & EXCEL_APP_VERSION_HELLGATE))
		{
			return FALSE;
		}
	}
	else if(AppIsTugboat())
	{
		if ((tExcelField.m_dwFlags & EXCEL_FIELD_HELLGATE_ONLY))
		{
			return FALSE;
		}
		if (!(version->m_VersionApp & EXCEL_APP_VERSION_TUGBOAT))
		{
			return FALSE;
		}	
	}
#else
#if defined(HELLGATE)
	if ((tExcelField.m_dwFlags & EXCEL_FIELD_MYTHOS_ONLY) )
	{
		return FALSE;
	}
	if (!(version->m_VersionApp & EXCEL_APP_VERSION_HELLGATE))
	{
		return FALSE;
	}
#elif defined(TUGBOAT)
	if ((tExcelField.m_dwFlags & EXCEL_FIELD_HELLGATE_ONLY))
	{
		return FALSE;
	}
	if (!(version->m_VersionApp & EXCEL_APP_VERSION_TUGBOAT))
	{
		return FALSE;
	}	
#else
	ASSERT(FALSE);
#endif
#endif
	return TRUE;
}				


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
static int EXCEL_FIELD_COMPARE(
	const T a,
	const T b)
{
	if (a > b)
	{
		return 1;
	}
	else if (a < b)
	{
		return -1;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
static int EXCEL_ARRAY_COMPARE(
	const T * a,
	const T * b,
	unsigned int len)
{
	for (unsigned int ii = 0; ii < len; ++ii)
	{
		if (a[ii] > b[ii])
		{
			return 1;
		}
		else if (a[ii] < b[ii])
		{
			return -1;
		}
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
static int EXCEL_FIELD_COMPARE_STRING(
	const T a,
	const T b,
	unsigned int len)
{
	return PStrICmp(a, b, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
static int EXCEL_FIELD_COMPARE_DYNAMIC_STRING(
	const T a,
	const T b)
{
	ASSERT_RETVAL(a, -1);
	ASSERT_RETVAL(b, 1);
	return PStrICmp(a, b);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int EXCEL_KEY_COMPARE_STR(
	const EXCEL_INDEX_DESC & index, 
	const BYTE * fielddata,
	const BYTE * key)
{
	return PStrICmp((const char *)fielddata, (const char *)key, index.m_IndexFields[0]->m_nCount);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int EXCEL_KEY_COMPARE_DYNAMIC_STR(
	const EXCEL_INDEX_DESC & index, 
	const BYTE * fielddata,
	const BYTE * key)
{
	REF(index);
	const char * str = *(const char **)fielddata; 
	ASSERT_RETVAL(str, -1);
	return PStrICmp(str, (const char *)key);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int EXCEL_KEY_COMPARE_STRINT(
	const EXCEL_INDEX_DESC & index, 
	const BYTE * fielddata,
	const BYTE * key)
{
	REF(index);
	int a = *(const int *)fielddata;
	int b = CAST_PTR_TO_INT((key - (const BYTE *)0));
	if (a < b)
	{
		return -1;
	}
	else if (a > b)
	{
		return 1;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const BYTE * EXCEL_KEY_PREPROCESS_STRINT(
	const EXCEL_INDEX_DESC & index, 
	const BYTE * key)
{
	EXCEL_FIELD * field = (EXCEL_FIELD *)index.m_IndexFields[0];
	ASSERT_RETNULL(field && field->m_Type == EXCEL_FIELDTYPE_STRINT);

	EXCEL_LOCK(CWRAutoLockReader, &field->m_FieldData.csStrIntDict);

	const BYTE * retval = (const BYTE *)0 + EXCEL_LINK_INVALID;

	ASSERT_RETVAL(key, retval);
	if (!field->m_FieldData.strintdict)
	{
		return retval;
	}
	return (const BYTE *)0 + field->m_FieldData.strintdict->GetValByStr((const char *)key);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int EXCEL_INDEX_COMPARE(
	const EXCEL_INDEX_DESC & index,
	const BYTE * a,
	const BYTE * b)
{
	ASSERT(a);
	ASSERT(b);

	int result = 0;
	for (unsigned int ii = 0; ii < EXCEL_INDEX_DESC::EXCEL_MAX_INDEX_FIELDS; ++ii)
	{
		const EXCEL_FIELD * field = index.m_IndexFields[ii];
		if (!field)
		{
			return result;
		}
		
		const BYTE * fieldDataA = a + field->m_nOffset;
		const BYTE * fieldDataB = b + field->m_nOffset;

		switch (field->m_Type)
		{
		case EXCEL_FIELDTYPE_STRING:
			result = EXCEL_FIELD_COMPARE_STRING((const char *)fieldDataA, (const char *)fieldDataB, field->m_nCount); 
			break;
		case EXCEL_FIELDTYPE_DYNAMIC_STRING:
			result = EXCEL_FIELD_COMPARE_DYNAMIC_STRING(*(const char **)fieldDataA, *(const char **)fieldDataB); 
			break;
		case EXCEL_FIELDTYPE_STRINT:
			result = EXCEL_FIELD_COMPARE(*(int *)fieldDataA, *(int *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_CHAR:
			result = EXCEL_FIELD_COMPARE(*(char *)fieldDataA, *(char *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_BYTE:
			result = EXCEL_FIELD_COMPARE(*(BYTE *)fieldDataA, *(BYTE *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_SHORT:
			result = EXCEL_FIELD_COMPARE(*(short *)fieldDataA, *(short *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_WORD:
			result = EXCEL_FIELD_COMPARE(*(WORD *)fieldDataA, *(WORD *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_INT:
			result = EXCEL_FIELD_COMPARE(*(int *)fieldDataA, *(int *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_DWORD:
			result = EXCEL_FIELD_COMPARE(*(DWORD *)fieldDataA, *(DWORD *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_BOOL:
			result = EXCEL_FIELD_COMPARE(*(BOOL *)fieldDataA, *(BOOL *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_FLOAT:
			result = EXCEL_FIELD_COMPARE(*(float *)fieldDataA, *(float *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_BYTE_PERCENT:
			result = EXCEL_FIELD_COMPARE(*(BYTE *)fieldDataA, *(BYTE *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_FOURCC:
			result = EXCEL_FIELD_COMPARE(*(DWORD *)fieldDataA, *(DWORD *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_THREECC:
			result = EXCEL_FIELD_COMPARE(*(DWORD *)fieldDataA, *(DWORD *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_TWOCC:
			result = EXCEL_FIELD_COMPARE(*(WORD *)fieldDataA, *(WORD *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_ONECC:
			result = EXCEL_FIELD_COMPARE(*(BYTE *)fieldDataA, *(BYTE *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_LINK_INT:	
			result = EXCEL_FIELD_COMPARE(*(int *)fieldDataA, *(int *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_LINK_CHAR:		
			result = EXCEL_FIELD_COMPARE(*(char *)fieldDataA, *(char *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_LINK_SHORT:	
			result = EXCEL_FIELD_COMPARE(*(short *)fieldDataA, *(short *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_LINK_FOURCC:	
			result = EXCEL_FIELD_COMPARE(*(DWORD *)fieldDataA, *(DWORD *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_LINK_THREECC:	
			result = EXCEL_FIELD_COMPARE(*(DWORD *)fieldDataA, *(DWORD *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_LINK_TWOCC:	
			result = EXCEL_FIELD_COMPARE(*(WORD *)fieldDataA, *(WORD *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_LINK_ONECC:	
			result = EXCEL_FIELD_COMPARE(*(BYTE *)fieldDataA, *(BYTE *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_LINK_TABLE:
			result = EXCEL_FIELD_COMPARE(*(int *)fieldDataA, *(int *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_DICT_CHAR:												
			result = EXCEL_FIELD_COMPARE(*(char *)fieldDataA, *(char *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_DICT_SHORT:											
			result = EXCEL_FIELD_COMPARE(*(short *)fieldDataA, *(short *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_DICT_INT:												
			result = EXCEL_FIELD_COMPARE(*(int *)fieldDataA, *(int *)fieldDataB);
			break;
		case EXCEL_FIELDTYPE_DICT_INT_ARRAY:
			result = EXCEL_ARRAY_COMPARE((int *)fieldDataA, (int *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_CHAR_ARRAY:
			result = EXCEL_ARRAY_COMPARE((char *)fieldDataA, (char *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_BYTE_ARRAY:
			result = EXCEL_ARRAY_COMPARE((BYTE *)fieldDataA, (BYTE *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_SHORT_ARRAY:
			result = EXCEL_ARRAY_COMPARE((short *)fieldDataA, (short *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_WORD_ARRAY:
			result = EXCEL_ARRAY_COMPARE((WORD *)fieldDataA, (WORD *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_INT_ARRAY:
			result = EXCEL_ARRAY_COMPARE((int *)fieldDataA, (int *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_DWORD_ARRAY:
			result = EXCEL_ARRAY_COMPARE((DWORD *)fieldDataA, (DWORD *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_FLOAT_ARRAY:
			result = EXCEL_ARRAY_COMPARE((float *)fieldDataA, (float *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_LINK_BYTE_ARRAY:
			result = EXCEL_ARRAY_COMPARE((BYTE *)fieldDataA, (BYTE *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_LINK_WORD_ARRAY:
			result = EXCEL_ARRAY_COMPARE((WORD *)fieldDataA, (WORD *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_LINK_INT_ARRAY:
			result = EXCEL_ARRAY_COMPARE((int *)fieldDataA, (int *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_LINK_FOURCC_ARRAY:
		case EXCEL_FIELDTYPE_LINK_THREECC_ARRAY:
			result = EXCEL_ARRAY_COMPARE((DWORD *)fieldDataA, (DWORD *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_LINK_TWOCC_ARRAY:
			result = EXCEL_ARRAY_COMPARE((WORD *)fieldDataA, (WORD *)fieldDataB, field->m_nCount);
			break;
		case EXCEL_FIELDTYPE_LINK_ONECC_ARRAY:
			result = EXCEL_ARRAY_COMPARE((BYTE *)fieldDataA, (BYTE *)fieldDataB, field->m_nCount);
			break;
		default:
			ASSERT(0);
		}

		if (result != 0)
		{
			return result;
		}
	}
	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD::Free(
	void)
{
	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_STRINT:
		{
			if (m_FieldData.strintdict)
			{
				m_FieldData.strintdict->Free();
				FREE(NULL, m_FieldData.strintdict);
				m_FieldData.strintdict = NULL;
			}
			EXCEL_LOCK_FREE(m_FieldData.csStrIntDict);
		}
		break;
	case EXCEL_FIELDTYPE_DICT_CHAR:
	case EXCEL_FIELDTYPE_DICT_SHORT:
	case EXCEL_FIELDTYPE_DICT_INT:
	case EXCEL_FIELDTYPE_DICT_INT_ARRAY:
		{
			if (m_FieldData.strdict)
			{
				StrDictionaryFree(m_FieldData.strdict);
				m_FieldData.strdict = NULL;
			}
		}
		break;

	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelSetStringDefault(
	EXCEL_FIELD * field,
	const char * str)
{
	ASSERT_RETURN(field);
	field->m_Default.str = str;
	field->m_Default.strlen = str ? PStrLen(str) : 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD::SetDefaultValue(
	const void * defaultValue)
{
	if (!defaultValue)
	{
		return;
	}

	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:		sExcelSetStringDefault(this, (const char *)defaultValue);				break;
	case EXCEL_FIELDTYPE_STRINT:				m_Default.link = *(const EXCEL_LINK *)defaultValue;						break;
	case EXCEL_FIELDTYPE_FLAG:					m_Default.boolean = *(const BOOL *)defaultValue;						break;
	case EXCEL_FIELDTYPE_CHAR:			
	case EXCEL_FIELDTYPE_CHAR_ARRAY:			m_Default.charval = *(const char *)defaultValue;						break;
	case EXCEL_FIELDTYPE_BYTE:			
	case EXCEL_FIELDTYPE_BYTE_PERCENT:
	case EXCEL_FIELDTYPE_BYTE_ARRAY:			m_Default.byte = *(const BYTE *)defaultValue;							break;
	case EXCEL_FIELDTYPE_SHORT:			
	case EXCEL_FIELDTYPE_SHORT_ARRAY:			m_Default.shortval = *(const short *)defaultValue;						break;								
	case EXCEL_FIELDTYPE_WORD:			
	case EXCEL_FIELDTYPE_WORD_ARRAY:			m_Default.word = *(const WORD *)defaultValue;							break;								
	case EXCEL_FIELDTYPE_INT:			
	case EXCEL_FIELDTYPE_INT_ARRAY:				m_Default.intval = *(const int *)defaultValue;							break;								
	case EXCEL_FIELDTYPE_DWORD:			
	case EXCEL_FIELDTYPE_DWORD_ARRAY:			m_Default.dword = *(const DWORD *)defaultValue;							break;								
	case EXCEL_FIELDTYPE_BOOL:					m_Default.boolean = *(const BOOL *)defaultValue;						break;
	case EXCEL_FIELDTYPE_FLOAT:			
	case EXCEL_FIELDTYPE_FLOAT_ARRAY:			m_Default.floatval = *(const float *)defaultValue;						break;								
	case EXCEL_FIELDTYPE_FOURCC:				m_Default.fourcc = STRTOFOURCC((const char *)defaultValue);				break;
	case EXCEL_FIELDTYPE_THREECC:				m_Default.fourcc = STRTOTHREECC((const char *)defaultValue);			break;
	case EXCEL_FIELDTYPE_TWOCC:					m_Default.twocc = (WORD)STRTOTWOCC((const char *)defaultValue);			break;
	case EXCEL_FIELDTYPE_ONECC:					m_Default.onecc = (BYTE)STRTOONECC((const char *)defaultValue);			break;
	case EXCEL_FIELDTYPE_LINK_INT:			
	case EXCEL_FIELDTYPE_LINK_CHAR:			
	case EXCEL_FIELDTYPE_LINK_SHORT:		
	case EXCEL_FIELDTYPE_LINK_FOURCC:		
	case EXCEL_FIELDTYPE_LINK_THREECC:		
	case EXCEL_FIELDTYPE_LINK_TWOCC:		
	case EXCEL_FIELDTYPE_LINK_ONECC:
	case EXCEL_FIELDTYPE_LINK_STAT:
	case EXCEL_FIELDTYPE_DICT_CHAR:												
	case EXCEL_FIELDTYPE_DICT_SHORT:											
	case EXCEL_FIELDTYPE_DICT_INT:												
	case EXCEL_FIELDTYPE_DICT_INT_ARRAY:
	case EXCEL_FIELDTYPE_LINK_BYTE_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_WORD_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_INT_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_FOURCC_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_THREECC_ARRAY:			
	case EXCEL_FIELDTYPE_LINK_TWOCC_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_ONECC_ARRAY:		sExcelSetStringDefault(this, (const char *)defaultValue);				break;
	case EXCEL_FIELDTYPE_LINK_TABLE:			m_Default.link = *(const EXCEL_LINK *)defaultValue;						break;
	case EXCEL_FIELDTYPE_STR_TABLE:				
	case EXCEL_FIELDTYPE_STR_TABLE_ARRAY:		
	case EXCEL_FIELDTYPE_UNITTYPES:				sExcelSetStringDefault(this, (const char *)defaultValue);				break;
	case EXCEL_FIELDTYPE_DEFSTAT:				m_Default.intval = *(const int *)defaultValue;							break;
	case EXCEL_FIELDTYPE_DEFSTAT_FLOAT:			m_Default.floatval = *(const float *)defaultValue;							break;
	case EXCEL_FIELDTYPE_DEFCODE:
	case EXCEL_FIELDTYPE_LINK_FLAG_ARRAY:
	default:									ASSERT(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD::SetDataFlag(
	DWORD flags,
	unsigned int offset64)
{
	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
		ASSERT_RETURN(m_FieldData.flags == 0);
		m_FieldData.flags = flags;
		m_FieldData.offset64 = offset64;
		break;
	default:
		ASSERT(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD::SetFlagData(
	unsigned int bit)
{
	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_FLAG:
		ASSERT_RETURN(m_FieldData.bit == 0);
		m_FieldData.bit = bit;
		break;
	default:
		ASSERT(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD::SetFileIdData(
	const char * path,
	PAK_ENUM pakid)
{
	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_FILEID:		
	case EXCEL_FIELDTYPE_COOKED_FILEID:
		ASSERT_RETURN(m_FieldData.filepath == NULL);
		ASSERT_RETURN(m_FieldData.filepath == NULL);
		m_FieldData.filepath = path;
		m_FieldData.pakid = pakid;
		break;
	default:
		ASSERT(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD::SetLinkData(
	unsigned int in_table,
	unsigned int in_index,
	unsigned int in_stat)
{
	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_LINK_INT:			
	case EXCEL_FIELDTYPE_LINK_CHAR:			
	case EXCEL_FIELDTYPE_LINK_SHORT:		
	case EXCEL_FIELDTYPE_LINK_FOURCC:		
	case EXCEL_FIELDTYPE_LINK_THREECC:		
	case EXCEL_FIELDTYPE_LINK_TWOCC:		
	case EXCEL_FIELDTYPE_LINK_ONECC:	
	case EXCEL_FIELDTYPE_LINK_BYTE_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_WORD_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_INT_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_FOURCC_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_THREECC_ARRAY:			
	case EXCEL_FIELDTYPE_LINK_TWOCC_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_ONECC_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_FLAG_ARRAY:		
		ASSERT_RETURN(m_FieldData.idTable == 0);
		ASSERT_RETURN(m_FieldData.idIndex == 0);
		ASSERT_RETURN(in_table < EXCEL::EXCEL_MAX_TABLE_COUNT);
		ASSERT_RETURN(in_index < EXCEL_STRUCT::EXCEL_MAX_INDEXES);
		m_FieldData.idTable = in_table;
		m_FieldData.idIndex = in_index;
		break;
	case EXCEL_FIELDTYPE_LINK_STAT:
		ASSERT_RETURN(m_FieldData.idTable == 0);
		ASSERT_RETURN(m_FieldData.idIndex == 0);
		ASSERT_RETURN(m_FieldData.wLinkStat == 0);
		ASSERT_RETURN(in_table < EXCEL::EXCEL_MAX_TABLE_COUNT);
		ASSERT_RETURN(in_index < EXCEL_STRUCT::EXCEL_MAX_INDEXES);
		m_FieldData.idTable = in_table;
		m_FieldData.idIndex = in_index;
		m_FieldData.wLinkStat = in_stat;
		break;
	default:
		ASSERT(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD::SetDictData(
	STR_DICTIONARY * dictionary,
	unsigned int max)
{
	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_DICT_CHAR:												
	case EXCEL_FIELDTYPE_DICT_SHORT:											
	case EXCEL_FIELDTYPE_DICT_INT:												
	case EXCEL_FIELDTYPE_DICT_INT_ARRAY:
		ASSERT_RETURN(m_FieldData.strdict == NULL);
		StrDictionarySort(dictionary);
		m_FieldData.strdict = dictionary;
		m_FieldData.max = max;

		if (m_Default.str)
		{
			BOOL found;
			StrDictionaryFind(dictionary, m_Default.str, &found);
			if (found)
			{
				m_dwFlags |= EXCEL_FIELD_PARSE_NULL_TOKEN;
			}
		}
		break;
	default:
		ASSERT(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD::SetFloatMinMaxData(
	float min,
	float max)
{
	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_FLOAT:												
	case EXCEL_FIELDTYPE_FLOAT_ARRAY:											
		m_FieldData.fmin = min;
		m_FieldData.fmax = max;
		break;
	default:
		ASSERT(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD::SetParseData(
	EXCEL_PARSEFUNC * fp,
	int param)
{
	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_PARSE:												
		ASSERT_RETURN(m_FieldData.fpParseFunc == NULL);
		m_FieldData.fpParseFunc = fp;
		m_FieldData.param = param;
		break;
	default:
		ASSERT(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD::SetStatData(
	int wStat,
	DWORD dwParam)
{
	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_DEFSTAT:
	case EXCEL_FIELDTYPE_DEFSTAT_FLOAT:
		m_FieldData.wStat = wStat;
		m_FieldData.dwParam = dwParam;
		break;
	default:
		ASSERT(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_FIELD::IsLoadFirst(
	void) const
{
	return ((m_dwFlags & (EXCEL_FIELD_ISINDEX | EXCEL_FIELD_INHERIT_ROW | EXCEL_FIELD_VERSION_ROW)) != 0);
}


//----------------------------------------------------------------------------
// helper func for SetDefaultData()
//----------------------------------------------------------------------------
template <typename T>
void EXCEL_FIELD::SetStringData(
	BYTE * fielddata,
	const T * src,
	unsigned int len) const
{
	ASSERT(m_nElemSize == sizeof(T))
	PStrCopy((T *)fielddata, src, len);
	if (m_FieldData.flags & EXCEL_STRING_LOWER)
	{
		PStrLower((T *)fielddata, len);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD::SetDynamicStringData(
	EXCEL_TABLE * table,
	BYTE * fielddata,
	const char * src,
	unsigned int len) const
{
	ASSERT_RETURN(table);
	ASSERT_RETURN(fielddata);
	if (!src)
	{
		*(const char **)fielddata = EXCEL_BLANK;
		return;
	}
	ASSERT_RETURN(src[len] == 0);
	*(const char **)fielddata = table->m_DStrings.AddString(src, len);
}


//----------------------------------------------------------------------------
// helper func for SetDefaultData()
//----------------------------------------------------------------------------
template <typename T> 
void EXCEL_FIELD::SetDefaultDataArray(
	BYTE * fielddata,
	T defaultVal) const
{
	T * ptr = (T *)fielddata;
	for (unsigned int ii = 0; ii < m_nCount; ++ii, ++ptr)
	{
		*ptr = defaultVal;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_FIELD::LoadDirectTranslateStringTableArray(
	struct EXCEL_TABLE * table, 
	unsigned int row,
	BYTE * fielddata, 
	const char * token) const
{
	SetDefaultDataArray<int>(fielddata, EXCEL_STR_INVALID);

	static const char * blank = "";
	if (!token)
	{
		token = blank;
	}

	const char * cur = token;
	const char * subtoken;
	unsigned int toklen;
	int * ptr = (int *)fielddata;
	int * end = ptr + m_nCount;
	while (ptr < end && sExcelGetCommaDelimitedToken(cur, subtoken, toklen))
	{
		char ctok[255];
		if (!toklen || toklen >= arrsize(ctok) - 1)
		{
			continue;
		}
		PStrCopy(ctok, subtoken, toklen + 1);

		int link = StringTableCommonGetStringIndexByKey(LanguageGetCurrent(), ctok);
		if (link != EXCEL_STR_INVALID)
		{
			*ptr = link;
			ptr++;
		}
		else
		{
			if (sFieldReportErrorsForApp( table, row, *this ))
			{
				ExcelLog("EXCEL WARNING:  STRING NOT FOUND  TABLE: %s   ROW: %d   COLUMN: %s   KEY: %s", table->m_Name, row, m_ColumnName, ctok);
			}
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_FIELD::LoadDirectTranslateUnitTypes(
	struct EXCEL_TABLE * table, 
	unsigned int row,
	BYTE * fielddata, 
	const char * token) const
{
	memclear(fielddata, sizeof(int) * m_nCount);

	REF(row);

	unsigned int idUnitTypesTable = Excel().m_idUnitTypesTable;	// like many things here, not thread safe
	ASSERT_RETFALSE(idUnitTypesTable != EXCEL_LINK_INVALID);	
	ASSERT_RETFALSE(table->m_Index != idUnitTypesTable);			// deadlock

	static const char * blank = "";
	if (!token)
	{
		token = blank;
	}

	const char * cur = token;
	const char * subtoken;
	unsigned int toklen;
	int * ptr = (int *)fielddata;
	int * end = ptr + m_nCount;
	while (ptr < end && sExcelGetCommaDelimitedToken(cur, subtoken, toklen))
	{
		char ctok[255];
		if (!toklen || toklen >= arrsize(ctok) - 1)
		{
			continue;
		}
		PStrCopy(ctok, subtoken, toklen + 1);

		int bExclude = 0;
		if (ctok[0] == '!')
		{
			bExclude = 1;
		}

		int link;
		if (table->m_Index == idUnitTypesTable)
		{
			link = (int)table->GetLineByStrKey(ctok + bExclude);
		}
		else
		{
			link = (int)ExcelGetLineByStrKey(idUnitTypesTable, ctok + bExclude);
		}
		if (link > 0)
		{
			*ptr = (bExclude ? -link : link);
			ptr++;
		}
		else
		{
			ExcelWarn("EXCEL WARNING:  UNITTYPE NOT FOUND  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", table->m_Name, row, m_ColumnName, ctok);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelEatWhitespace(
	char * & str)
{
	while (*str == ' ')
	{
		++str;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelEatNumber(
	char * & str)
{
	while (*str >= '0' && *str <= '9')
	{
		++str;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLoadDirectTranslateVersionFlags(
	EXCEL_TABLE * table,
	const EXCEL_FIELD * field,
	unsigned int row,
	STR_DICTIONARY * dictionary,
	DWORD & flag,
	char * token)
{
	REF(row);
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(field);
	ASSERT_RETFALSE(dictionary);
	ASSERT_RETFALSE(token);

	BOOL bFirst = TRUE;
	char * cur = token;
	char * subtoken = NULL;
	unsigned int subtoklen = 0;
	while (sExcelGetCommaDelimitedToken(cur, subtoken, subtoklen))
	{
		EXCEL_TOKENIZE<char> tokenizer(subtoken, subtoklen);

		unsigned int mask = CAST_PTR_TO_INT(StrDictionaryFind(dictionary, subtoken));
		if (mask == 0)
		{
			ExcelWarn("EXCEL WARNING:  VERSION FLAG NOT FOUND  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", table->m_Name, row, field->m_ColumnName, subtoken);
			continue;
		}
		if (bFirst && subtoken[0] != '!')
		{
			flag = 0;
			bFirst = FALSE;
		}
		SET_MASKV(flag, mask, subtoken[0] != '!');
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLoadDirectTranslateVersionMinMax(
	EXCEL_TABLE * table,
	const EXCEL_FIELD * field,
	unsigned int row,
	WORD & minVersion,
	WORD & maxVersion,
	char * token)
{
	REF(row);
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(field);
	ASSERT_RETFALSE(token);

	if (token[0] == 0)
	{
		minVersion = 0;
		maxVersion = EXCEL_DATA_VERSION::EXCEL_MAX_VERSION;
		return TRUE;
	}

	if (token[0] == '-')
	{
		minVersion = 0;
		++token;
		sExcelEatWhitespace(token);
		int val;
		if (!PStrToInt(token, val) || val < 0 || val > EXCEL_DATA_VERSION::EXCEL_MAX_VERSION)
		{
			ExcelWarn("EXCEL WARNING:  INVALID VERSION NUMBER  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", table->m_Name, row, field->m_ColumnName, token);
			return FALSE;
		}
		maxVersion = (WORD)val;
		sExcelEatNumber(token);
	}
	else
	{
		int val;
		if (!PStrToInt(token, val, FALSE) || val < 0 || val > EXCEL_DATA_VERSION::EXCEL_MAX_VERSION)
		{
			ExcelWarn("EXCEL WARNING:  INVALID VERSION NUMBER  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", table->m_Name, row, field->m_ColumnName, token);
			return FALSE;
		}
		minVersion = (WORD)val;
		sExcelEatNumber(token);
		sExcelEatWhitespace(token);
		if (token[0] == 0)
		{
			maxVersion = EXCEL_DATA_VERSION::EXCEL_MAX_VERSION;
			return TRUE;
		}
		if (token[0] != '-')
		{
			minVersion = 0;
			ExcelWarn("EXCEL WARNING:  INVALID VERSION NUMBER  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", table->m_Name, row, field->m_ColumnName, token);
			return FALSE;
		}
		++token;
		sExcelEatWhitespace(token);
		if (!PStrToInt(token, val) || val < minVersion || val > EXCEL_DATA_VERSION::EXCEL_MAX_VERSION)
		{
			ExcelWarn("EXCEL WARNING:  INVALID VERSION NUMBER  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", table->m_Name, row, field->m_ColumnName, token);
			return FALSE;
		}
		maxVersion = (WORD)val;
		sExcelEatWhitespace(token);
	}

	sExcelEatNumber(token);

	if (token[0] != 0)
	{
		minVersion = 0;
		maxVersion = EXCEL_DATA_VERSION::EXCEL_MAX_VERSION;
		ExcelWarn("EXCEL WARNING:  INVALID VERSION NUMBER  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", table->m_Name, row, field->m_ColumnName, token);
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static EXCEL_LINK sLoadDirectTranslateLinkInt(
	EXCEL_TABLE * table, 
	unsigned int row,	
	const EXCEL_FIELD * field,
	const char * token)
{
	REF(row);
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(field, EXCEL_LINK_INVALID);

	EXCEL_LINK link = EXCEL_LINK_INVALID;
	if (!token)
	{
		return link;
	}

	if (field->m_FieldData.idTable == table->m_Index)
	{
		link = table->GetLineByStrKey(field->m_FieldData.idIndex, token);
	}
	else
	{
		link = ExcelGetLineByStrKey(field->m_FieldData.idTable, field->m_FieldData.idIndex, token);
	}
	if (link == EXCEL_LINK_INVALID && token[0] != 0)
	{
		ExcelWarn("EXCEL WARNING:  INDEX NOT FOUND(%d)  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", field->m_FieldData.idIndex, table->m_Name, row, field->m_ColumnName, token);
	}
	return link;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static EXCEL_LINK sLoadDirectTranslateDictLinkArray(
	EXCEL_TABLE * table, 
	unsigned int row,	
	const EXCEL_FIELD * field,
	const char * token)
{
	REF(row);
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(field, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(field->m_FieldData.strdict, EXCEL_LINK_INVALID);

	EXCEL_LINK link = EXCEL_LINK_INVALID;
	if (!token)
	{
		return link;
	}

	BOOL found;
	void * ptr = StrDictionaryFind(field->m_FieldData.strdict, token, &found);
	if (found)
	{
		link = CAST_PTR_TO_INT(ptr);
		unsigned int max = field->m_FieldData.max;
		ASSERT(link <= max);
		if (link > max)
		{
			link = EXCEL_LINK_INVALID;
		}
	}

	if (link == EXCEL_LINK_INVALID && token[0])
	{
		ExcelWarn("EXCEL WARNING:  DICT NOT FOUND  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", table->m_Name, row, field->m_ColumnName, token);
	}
	return link;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static EXCEL_LINK sLoadDirectTranslateLinkFourCC(
	EXCEL_TABLE * table,
	unsigned int row,	
	const EXCEL_FIELD * field,
	const char * token)
{
	REF(row);
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(field, EXCEL_LINK_INVALID);

	EXCEL_LINK link = EXCEL_LINK_INVALID;
	if (!token)
	{
		return link;
	}

	char tmp[5];
	PStrCopy(tmp, token, arrsize(tmp));
	DWORD fourCC = STRTOFOURCC(tmp);

	if (field->m_FieldData.idTable == table->m_Index)
	{
		link = table->GetLineByCC<DWORD>(field->m_FieldData.idIndex, fourCC);
	}
	else
	{
		link = ExcelGetLineByFourCC(field->m_FieldData.idTable, field->m_FieldData.idIndex, fourCC);
	}
	if (link == EXCEL_LINK_INVALID && token[0] != 0)
	{
		ExcelWarn("EXCEL WARNING:  INDEX NOT FOUND(%d)  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", field->m_FieldData.idIndex, table->m_Name, row, field->m_ColumnName, token);
	}
	return link;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static EXCEL_LINK sLoadDirectTranslateLinkThreeCC(
	EXCEL_TABLE * table,
	unsigned int row,	
	const EXCEL_FIELD * field,
	const char * token)
{
	REF(row);
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(field, EXCEL_LINK_INVALID);

	EXCEL_LINK link = EXCEL_LINK_INVALID;
	if (!token)
	{
		return link;
	}

	char tmp[4];
	PStrCopy(tmp, token, arrsize(tmp));
	DWORD threeCC = STRTOTHREECC(tmp);

	if (field->m_FieldData.idTable == table->m_Index)
	{
		link = table->GetLineByCC<DWORD>(field->m_FieldData.idIndex, threeCC);
	}
	else
	{
		link = ExcelGetLineByThreeCC(field->m_FieldData.idTable, field->m_FieldData.idIndex, threeCC);
	}
	if (link == EXCEL_LINK_INVALID && token[0] != 0)
	{
		ExcelWarn("EXCEL WARNING:  INDEX NOT FOUND(%d)  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", field->m_FieldData.idIndex, table->m_Name, row, field->m_ColumnName, token);
	}
	return link;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static EXCEL_LINK sLoadDirectTranslateLinkTwoCC(
	EXCEL_TABLE * table,
	unsigned int row,	
	const EXCEL_FIELD * field,
	const char * token)
{
	REF(row);
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(field, EXCEL_LINK_INVALID);

	EXCEL_LINK link = EXCEL_LINK_INVALID;
	if (!token)
	{
		return link;
	}

	char tmp[3];
	PStrCopy(tmp, token, arrsize(tmp));
	WORD twoCC = (WORD)STRTOTWOCC(tmp);

	if (field->m_FieldData.idTable == table->m_Index)
	{
		link = table->GetLineByCC<WORD>(field->m_FieldData.idIndex, twoCC);
	}
	else
	{
		link = ExcelGetLineByTwoCC(field->m_FieldData.idTable, field->m_FieldData.idIndex, twoCC);
	}
	if (link == EXCEL_LINK_INVALID && token[0] != 0)
	{
		ExcelWarn("EXCEL WARNING:  INDEX NOT FOUND(%d)  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", field->m_FieldData.idIndex, table->m_Name, row, field->m_ColumnName, token);
	}
	return link;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static EXCEL_LINK sLoadDirectTranslateLinkOneCC(
	EXCEL_TABLE * table,
	unsigned int row,	
	const EXCEL_FIELD * field,
	const char * token)
{
	REF(row);
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(field, EXCEL_LINK_INVALID);

	EXCEL_LINK link = EXCEL_LINK_INVALID;
	if (!token)
	{
		return link;
	}

	char tmp[2];
	PStrCopy(tmp, token, arrsize(tmp));
	BYTE oneCC = (BYTE)STRTOONECC(tmp);

	if (field->m_FieldData.idTable == table->m_Index)
	{
		link = table->GetLineByCC<BYTE>(field->m_FieldData.idIndex, oneCC);
	}
	else
	{
		link = ExcelGetLineByOneCC(field->m_FieldData.idTable, field->m_FieldData.idIndex, oneCC);
	}
	if (link == EXCEL_LINK_INVALID && token[0] != 0)
	{
		ExcelWarn("EXCEL WARNING:  INDEX NOT FOUND(%d)  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", field->m_FieldData.idIndex, table->m_Name, row, field->m_ColumnName, token);
	}
	return link;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static EXCEL_LINK sLoadDirectTranslateDictLink(
	EXCEL_TABLE * table,
	unsigned int row,	
	const EXCEL_FIELD * field,
	const char * token,
	unsigned int max)
{
	REF(row);
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(field, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(field->m_FieldData.strdict, EXCEL_LINK_INVALID);

	EXCEL_LINK link = EXCEL_LINK_INVALID;
	if (!token)
	{
		return link;
	}

	BOOL found;
	void * ptr = StrDictionaryFind(field->m_FieldData.strdict, token, &found);
	if (found)
	{
		link = CAST_PTR_TO_INT(ptr);
		ASSERT(link <= max);
		if (link > max)
		{
			link = EXCEL_LINK_INVALID;
		}
	}

	if (link == EXCEL_LINK_INVALID && token[0])
	{
		ExcelWarn("EXCEL WARNING:  DICT NOT FOUND  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", table->m_Name, row, field->m_ColumnName, token);
	}
	return link;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sExcelGetDefaultStat(
	const EXCEL_TABLE * table,
	unsigned int row,
	int wStat,
	DWORD dwParam)
{
	EXCEL_STATS_GET_STAT * fpStatsGetStat = Excel().m_fpStatsGetStat;
	ASSERT_RETZERO(fpStatsGetStat);

	struct STATS * stats = table->GetStatsList(row);
	if (!stats)
	{
		return 0;
	}
	return fpStatsGetStat(stats, wStat, dwParam);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sExcelGetDefaultStatFloat(
	const EXCEL_TABLE * table,
	unsigned int row,
	int wStat,
	DWORD dwParam)
{
	EXCEL_STATS_GET_STAT_FLOAT * fpStatsGetStatFloat = Excel().m_fpStatsGetStatFloat;
	ASSERT_RETZERO(fpStatsGetStatFloat);

	struct STATS * stats = table->GetStatsList(row);
	if (!stats)
	{
		return 0.0f;
	}
	return fpStatsGetStatFloat(NULL, stats, wStat, dwParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelSetDefaultStat(
	EXCEL_TABLE * table,
	unsigned int row,
	int wStat,
	DWORD dwParam,
	int value)
{
	EXCEL_STATS_SET_STAT * fpStatsSetStat = Excel().m_fpStatsSetStat;
	ASSERT_RETURN(fpStatsSetStat);

	if (value == 0)
	{
		return;
	}

	TABLE_DATA_WRITE_LOCK(table);

	STATS * stats = table->GetOrCreateStatsList(row);
	ASSERT_RETURN(stats);

	fpStatsSetStat(NULL, stats, wStat, dwParam, value);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelSetDefaultStatFloat(
	EXCEL_TABLE * table,
	unsigned int row,
	int wStat,
	DWORD dwParam,
	float value)
{
	EXCEL_STATS_SET_STAT_FLOAT * fpStatsSetStatFloat = Excel().m_fpStatsSetStatFloat;
	ASSERT_RETURN(fpStatsSetStatFloat);

	if (value == 0.0f)
	{
		return;
	}

	TABLE_DATA_WRITE_LOCK(table);

	STATS * stats = table->GetOrCreateStatsList(row);
	ASSERT_RETURN(stats);

	fpStatsSetStatFloat(NULL, stats, wStat, dwParam, value);
}


//----------------------------------------------------------------------------
// assumes rowdata is nulled
//----------------------------------------------------------------------------
void EXCEL_FIELD::SetDefaultData(
	EXCEL_TABLE * table,
	unsigned int row,
	BYTE * rowdata) const
{
	ASSERT_RETURN(rowdata);

	BYTE * fielddata = rowdata + m_nOffset;

	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
		if (m_Default.str)
		{
			SetStringData(fielddata, m_Default.str, m_nCount);
		}
		break;
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
		SetDynamicStringData(table, fielddata, m_Default.str, m_Default.strlen);
		break;
	case EXCEL_FIELDTYPE_STRINT:				*(int *)fielddata = (int)m_Default.link;							break;	
	case EXCEL_FIELDTYPE_FLAG:					SETBIT((BYTE *)fielddata, m_FieldData.bit, m_Default.boolean);		break;
	case EXCEL_FIELDTYPE_CHAR:					*(char *)fielddata = m_Default.charval;								break;
	case EXCEL_FIELDTYPE_BYTE:					*(BYTE *)fielddata = m_Default.byte;								break;
	case EXCEL_FIELDTYPE_SHORT:					*(short *)fielddata = m_Default.shortval;							break;
	case EXCEL_FIELDTYPE_WORD:					*(WORD *)fielddata = m_Default.word;								break;
	case EXCEL_FIELDTYPE_INT:					*(int *)fielddata = m_Default.intval;								break;
	case EXCEL_FIELDTYPE_DWORD:					*(DWORD *)fielddata = m_Default.dword;								break;
	case EXCEL_FIELDTYPE_BOOL:					*(BOOL *)fielddata = m_Default.boolean;								break;
	case EXCEL_FIELDTYPE_FLOAT:					*(float *)fielddata = m_Default.floatval;							break;
	case EXCEL_FIELDTYPE_BYTE_PERCENT:			*(BYTE *)fielddata = m_Default.byte;								break;
	case EXCEL_FIELDTYPE_FOURCC:
	case EXCEL_FIELDTYPE_THREECC:				*(DWORD *)fielddata = m_Default.fourcc;								break;
	case EXCEL_FIELDTYPE_TWOCC:					*(WORD *)fielddata = m_Default.twocc;								break;
	case EXCEL_FIELDTYPE_ONECC:					*(BYTE *)fielddata = m_Default.onecc;								break;
	case EXCEL_FIELDTYPE_LINK_INT:				
		*(int *)fielddata = (int)sLoadDirectTranslateLinkInt(table, row, this, m_Default.str);			break;
	case EXCEL_FIELDTYPE_LINK_CHAR:				*(char *)fielddata = (char)sLoadDirectTranslateLinkInt(table, row, this, m_Default.str);		break;
	case EXCEL_FIELDTYPE_LINK_SHORT:			*(short *)fielddata = (short)sLoadDirectTranslateLinkInt(table, row, this, m_Default.str);		break;
	case EXCEL_FIELDTYPE_LINK_FOURCC:			*(DWORD *)fielddata = (DWORD)sLoadDirectTranslateLinkFourCC(table, row, this, m_Default.str);	break;	
	case EXCEL_FIELDTYPE_LINK_THREECC:			*(DWORD *)fielddata = (DWORD)sLoadDirectTranslateLinkThreeCC(table, row, this, m_Default.str);	break;	
	case EXCEL_FIELDTYPE_LINK_TWOCC:			*(WORD *)fielddata = (WORD)sLoadDirectTranslateLinkTwoCC(table, row, this, m_Default.str);		break;	
	case EXCEL_FIELDTYPE_LINK_ONECC:			*(BYTE *)fielddata = (BYTE)sLoadDirectTranslateLinkOneCC(table, row, this, m_Default.str);		break;	
	case EXCEL_FIELDTYPE_LINK_TABLE:			*(int *)fielddata = (int)m_Default.link;							break;	
	case EXCEL_FIELDTYPE_LINK_STAT:																					break;
	case EXCEL_FIELDTYPE_DICT_CHAR:				*(char *)fielddata = (char)sLoadDirectTranslateDictLink(table, row, this, m_Default.str, UCHAR_MAX);	break;	
	case EXCEL_FIELDTYPE_DICT_SHORT:			*(short *)fielddata = (short)sLoadDirectTranslateDictLink(table, row, this, m_Default.str, USHRT_MAX);	break;	
	case EXCEL_FIELDTYPE_DICT_INT:				*(int *)fielddata = (int)sLoadDirectTranslateDictLink(table, row, this, m_Default.str, UINT_MAX - 1);	break;	
	case EXCEL_FIELDTYPE_CHAR_ARRAY:			SetDefaultDataArray<char>(fielddata, m_Default.charval);			break;
	case EXCEL_FIELDTYPE_BYTE_ARRAY:			SetDefaultDataArray<BYTE>(fielddata, m_Default.byte);				break;
	case EXCEL_FIELDTYPE_SHORT_ARRAY:			SetDefaultDataArray<short>(fielddata, m_Default.shortval);			break;
	case EXCEL_FIELDTYPE_WORD_ARRAY:			SetDefaultDataArray<WORD>(fielddata, m_Default.word);				break;
	case EXCEL_FIELDTYPE_INT_ARRAY:				SetDefaultDataArray<int>(fielddata, m_Default.intval);				break;
	case EXCEL_FIELDTYPE_DWORD_ARRAY:			SetDefaultDataArray<DWORD>(fielddata, m_Default.dword);				break;
	case EXCEL_FIELDTYPE_FLOAT_ARRAY:			SetDefaultDataArray<float>(fielddata, m_Default.floatval);			break;
	case EXCEL_FIELDTYPE_LINK_BYTE_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_WORD_ARRAY:				
	case EXCEL_FIELDTYPE_DICT_INT_ARRAY:
		{
			int link = (int)sLoadDirectTranslateDictLink(table, row, this, m_Default.str, UINT_MAX - 1);
			SetDefaultDataArray<int>(fielddata, link);
			break;			
		}	
	case EXCEL_FIELDTYPE_LINK_INT_ARRAY:				
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkInt(table, row, this, m_Default.str);
			SetDefaultDataArray<EXCEL_LINK>(fielddata, link);			
			break;
		}
	case EXCEL_FIELDTYPE_LINK_FOURCC_ARRAY:				
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkFourCC(table, row, this, m_Default.str);
			SetDefaultDataArray<EXCEL_LINK>(fielddata, link);			
			break;
		}
	case EXCEL_FIELDTYPE_LINK_THREECC_ARRAY:			
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkThreeCC(table, row, this, m_Default.str);
			SetDefaultDataArray<EXCEL_LINK>(fielddata, link);			
			break;
		}
	case EXCEL_FIELDTYPE_LINK_TWOCC_ARRAY:				
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkTwoCC(table, row, this, m_Default.str);
			SetDefaultDataArray<EXCEL_LINK>(fielddata, link);			
			break;
		}
	case EXCEL_FIELDTYPE_LINK_ONECC_ARRAY:		
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkOneCC(table, row, this, m_Default.str);
			SetDefaultDataArray<EXCEL_LINK>(fielddata, link);			
			break;
		}
	case EXCEL_FIELDTYPE_LINK_FLAG_ARRAY:		memclear(fielddata, m_nElemSize);								break;
	case EXCEL_FIELDTYPE_STR_TABLE:				
		{
			if (m_Default.str)
			{
				int nStringTable = LanguageGetCurrent();
				*(int *)fielddata = StringTableCommonGetStringIndexByKey(nStringTable, m_Default.str);
			}
			else
			{
				*(int *)fielddata = EXCEL_STR_INVALID;
			}
			break;
		}
	case EXCEL_FIELDTYPE_STR_TABLE_ARRAY:
		{
			LoadDirectTranslateStringTableArray(table, row, fielddata, m_Default.str);
			break;
		}
	case EXCEL_FIELDTYPE_FILEID:	
	case EXCEL_FIELDTYPE_COOKED_FILEID:			*(FILEID *)fielddata = INVALID_FILEID;							break;
	case EXCEL_FIELDTYPE_CODE:					*(PCODE *)fielddata = (PCODE)(NULL_CODE);						break;
	case EXCEL_FIELDTYPE_DEFCODE:
		break;
	case EXCEL_FIELDTYPE_DEFSTAT:
		{
			if (m_Default.intval == 0)
			{
				break;
			}
			sExcelSetDefaultStat(table, row, m_FieldData.wStat, m_FieldData.dwParam, m_Default.intval);
		}
		break;
	case EXCEL_FIELDTYPE_DEFSTAT_FLOAT:
		{
			if (m_Default.floatval == 0.0f)
			{
				break;
			}
			sExcelSetDefaultStatFloat(table, row, m_FieldData.wStat, m_FieldData.dwParam, m_Default.floatval);
		}
		break;
	case EXCEL_FIELDTYPE_UNITTYPES:				
		{
			LoadDirectTranslateUnitTypes(table, row, fielddata, m_Default.str);		
			break;
		}
	case EXCEL_FIELDTYPE_PARSE:
		{
			ASSERT_BREAK(m_FieldData.fpParseFunc);
			m_FieldData.fpParseFunc(table, this, row, fielddata, "", 0, m_FieldData.param);
			break;
		}
	case EXCEL_FIELDTYPE_INHERIT_ROW:			*(unsigned int *)fielddata = EXCEL_LINK_INVALID;				break;
	case EXCEL_FIELDTYPE_VERSION_APP:
		{
			ASSERT_BREAK(table);
			ASSERT_BREAK(row < table->m_Data.Count());
			EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(table->m_Data[row]);
			ASSERT_BREAK(version);
			version->m_VersionApp = EXCEL_APP_VERSION_DEFAULT;
			switch (table->m_Files[table->m_CurFile].m_AppGame)
			{
			case APP_GAME_HELLGATE:		version->m_VersionApp = EXCEL_APP_VERSION_HELLGATE;		break;
			case APP_GAME_TUGBOAT:		version->m_VersionApp = EXCEL_APP_VERSION_TUGBOAT;		break;
			default:					version->m_VersionApp = EXCEL_APP_VERSION_ALL;			break;
			}
		}
		break;
	case EXCEL_FIELDTYPE_VERSION_PACKAGE:
		{
			ASSERT_BREAK(table);
			ASSERT_BREAK(row < table->m_Data.Count());
			EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(table->m_Data[row]);
			ASSERT_BREAK(version);
			version->m_VersionPackage = EXCEL_PACKAGE_VERSION_DEFAULT;
		}
		break;
	case EXCEL_FIELDTYPE_VERSION_MAJOR:
		{
			ASSERT_BREAK(table);
			ASSERT_BREAK(row < table->m_Data.Count());
			EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(table->m_Data[row]);
			ASSERT_BREAK(version);
			version->m_VersionMajorMin = 0; 
			version->m_VersionMajorMax = EXCEL_DATA_VERSION::EXCEL_MAX_VERSION;  
		}
		break;
	case EXCEL_FIELDTYPE_VERSION_MINOR:
		{
			ASSERT_BREAK(table);
			ASSERT_BREAK(row < table->m_Data.Count());
			EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(table->m_Data[row]);
			ASSERT_BREAK(version);
			version->m_VersionMinorMin = 0; 
			version->m_VersionMinorMax = EXCEL_DATA_VERSION::EXCEL_MAX_VERSION;  
		}
		break;
	default:
		ASSERT_RETURN(0);
	}
}


//----------------------------------------------------------------------------
// helper func for IsBlank()
//----------------------------------------------------------------------------
template <typename T> 
BOOL EXCEL_FIELD::TestDefaultDataArray(
	const BYTE * fielddata,
	T defaultVal) const
{
	T * ptr = (T *)fielddata;
	for (unsigned int ii = 0; ii < m_nCount; ++ii, ++ptr)
	{
		if (*ptr != defaultVal)
		{
			return FALSE;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// used for indexes
//----------------------------------------------------------------------------
BOOL EXCEL_FIELD::IsBlank(
	EXCEL_TABLE * table,
	unsigned int row,
	const BYTE * fielddata) const
{
	ASSERT_RETFALSE(fielddata);

	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:				
		{
			if (!m_Default.str)
			{
				return *(char *)fielddata == 0;
			}
			else
			{
				return (PStrCmp(m_Default.str, (const char *)fielddata) == 0);
			}
		}
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
		{
			ASSERT_RETFALSE(*(const char **)fielddata != NULL);
			if (!m_Default.str)
			{
				return (*(const char **)fielddata)[0] == 0;
			}
			else
			{
				return (PStrCmp(m_Default.str, *(const char **)fielddata) == 0);
			}
		}
	case EXCEL_FIELDTYPE_STRINT:				return *(int *)fielddata == EXCEL_LINK_INVALID;
	case EXCEL_FIELDTYPE_FLAG:					return TESTBIT((BYTE *)fielddata, m_FieldData.bit) == m_Default.boolean;
	case EXCEL_FIELDTYPE_CHAR:					return *(char *)fielddata == m_Default.charval;
	case EXCEL_FIELDTYPE_BYTE:					return *(BYTE *)fielddata == m_Default.byte;
	case EXCEL_FIELDTYPE_SHORT:					return *(short *)fielddata == m_Default.shortval;
	case EXCEL_FIELDTYPE_WORD:					return *(WORD *)fielddata == m_Default.word;
	case EXCEL_FIELDTYPE_INT:					return *(int *)fielddata == m_Default.intval;
	case EXCEL_FIELDTYPE_DWORD:					return *(DWORD *)fielddata == m_Default.dword;
	case EXCEL_FIELDTYPE_BOOL:					return *(BOOL *)fielddata == m_Default.boolean;
	case EXCEL_FIELDTYPE_FLOAT:					return *(float *)fielddata == m_Default.floatval;
	case EXCEL_FIELDTYPE_BYTE_PERCENT:			return *(BYTE *)fielddata == m_Default.byte;
	case EXCEL_FIELDTYPE_FOURCC:
	case EXCEL_FIELDTYPE_THREECC:				return *(DWORD *)fielddata == m_Default.fourcc;
	case EXCEL_FIELDTYPE_TWOCC:					return *(WORD *)fielddata == m_Default.twocc;
	case EXCEL_FIELDTYPE_ONECC:					return *(BYTE *)fielddata == m_Default.onecc;
	case EXCEL_FIELDTYPE_LINK_INT:				return *(int *)fielddata == (int)sLoadDirectTranslateLinkInt(table, row, this, m_Default.str);
	case EXCEL_FIELDTYPE_LINK_CHAR:				return *(char *)fielddata == (char)sLoadDirectTranslateLinkInt(table, row, this, m_Default.str);
	case EXCEL_FIELDTYPE_LINK_SHORT:			return *(short *)fielddata == (short)sLoadDirectTranslateLinkInt(table, row, this, m_Default.str);
	case EXCEL_FIELDTYPE_LINK_FOURCC:			return *(DWORD *)fielddata == (DWORD)sLoadDirectTranslateLinkFourCC(table, row, this, m_Default.str);
	case EXCEL_FIELDTYPE_LINK_THREECC:			return *(DWORD *)fielddata == (DWORD)sLoadDirectTranslateLinkThreeCC(table, row, this, m_Default.str);
	case EXCEL_FIELDTYPE_LINK_TWOCC:			return *(WORD *)fielddata == (WORD)sLoadDirectTranslateLinkTwoCC(table, row, this, m_Default.str);
	case EXCEL_FIELDTYPE_LINK_ONECC:			return *(BYTE *)fielddata == (BYTE)sLoadDirectTranslateLinkOneCC(table, row, this, m_Default.str);
	case EXCEL_FIELDTYPE_LINK_TABLE:			return *(int *)fielddata == (int)m_Default.link;
	case EXCEL_FIELDTYPE_LINK_STAT:				return FALSE;
	case EXCEL_FIELDTYPE_DICT_CHAR:				return *(char *)fielddata == (char)sLoadDirectTranslateDictLink(table, row, this, m_Default.str, UCHAR_MAX);
	case EXCEL_FIELDTYPE_DICT_SHORT:			return *(short *)fielddata == (short)sLoadDirectTranslateDictLink(table, row, this, m_Default.str, USHRT_MAX);
	case EXCEL_FIELDTYPE_DICT_INT:				return *(int *)fielddata == (int)sLoadDirectTranslateDictLink(table, row, this, m_Default.str, UINT_MAX - 1);
	case EXCEL_FIELDTYPE_DICT_INT_ARRAY:		return TestDefaultDataArray<int>(fielddata, (int)sLoadDirectTranslateDictLink(table, row, this, m_Default.str, UINT_MAX - 1));
	case EXCEL_FIELDTYPE_CHAR_ARRAY:			return TestDefaultDataArray<char>(fielddata, m_Default.charval);
	case EXCEL_FIELDTYPE_BYTE_ARRAY:			return TestDefaultDataArray<BYTE>(fielddata, m_Default.byte);
	case EXCEL_FIELDTYPE_SHORT_ARRAY:			return TestDefaultDataArray<short>(fielddata, m_Default.shortval);
	case EXCEL_FIELDTYPE_WORD_ARRAY:			return TestDefaultDataArray<WORD>(fielddata, m_Default.word);
	case EXCEL_FIELDTYPE_INT_ARRAY:				return TestDefaultDataArray<int>(fielddata, m_Default.intval);
	case EXCEL_FIELDTYPE_DWORD_ARRAY:			return TestDefaultDataArray<DWORD>(fielddata, m_Default.dword);
	case EXCEL_FIELDTYPE_FLOAT_ARRAY:			return TestDefaultDataArray<float>(fielddata, m_Default.floatval);
	case EXCEL_FIELDTYPE_LINK_BYTE_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_WORD_ARRAY:				
	case EXCEL_FIELDTYPE_LINK_INT_ARRAY:			
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkInt(table, row, this, m_Default.str);
			return TestDefaultDataArray<EXCEL_LINK>(fielddata, link);
		}
	case EXCEL_FIELDTYPE_LINK_FOURCC_ARRAY:				
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkFourCC(table, row, this, m_Default.str);
			return TestDefaultDataArray<EXCEL_LINK>(fielddata, link);
		}
	case EXCEL_FIELDTYPE_LINK_THREECC_ARRAY:			
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkThreeCC(table, row, this, m_Default.str);
			return TestDefaultDataArray<EXCEL_LINK>(fielddata, link);
		}
	case EXCEL_FIELDTYPE_LINK_TWOCC_ARRAY:				
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkTwoCC(table, row, this, m_Default.str);
			return TestDefaultDataArray<EXCEL_LINK>(fielddata, link);
		}
	case EXCEL_FIELDTYPE_LINK_ONECC_ARRAY:
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkOneCC(table, row, this, m_Default.str);
			return TestDefaultDataArray<EXCEL_LINK>(fielddata, link);
		}
	case EXCEL_FIELDTYPE_LINK_FLAG_ARRAY:
		{
			for (unsigned int ii = 0; ii < m_nElemSize / 4; ++ii)
			{
				if (fielddata[ii])
				{
					return FALSE;
				}
			}
			return TRUE;
		}
	case EXCEL_FIELDTYPE_STR_TABLE:				
		{
			if (m_Default.str)
			{
				int nStringTable = LanguageGetCurrent();			
				return *(int *)fielddata == StringTableCommonGetStringIndexByKey(nStringTable, m_Default.str);
			}
			else
			{
				return *(int *)fielddata == EXCEL_STR_INVALID;
			}
		}
	case EXCEL_FIELDTYPE_STR_TABLE_ARRAY:
		{
			int defaultstr = EXCEL_STR_INVALID;
			if (m_Default.str)
			{
				int nStringTable = LanguageGetCurrent();			
				defaultstr = StringTableCommonGetStringIndexByKey(nStringTable, m_Default.str);
			}

			for (unsigned int ii = 0; ii < m_nCount; ++ii)
			{
				if (((int *)fielddata)[ii] != defaultstr)
				{
					return FALSE;
				}
			}
			return TRUE;
		}
	case EXCEL_FIELDTYPE_FILEID:	
	case EXCEL_FIELDTYPE_COOKED_FILEID:			return *(FILEID *)fielddata == INVALID_FILEID;
	case EXCEL_FIELDTYPE_CODE:					return *(PCODE *)fielddata == (PCODE)(NULL_CODE);
	case EXCEL_FIELDTYPE_DEFCODE:
		return FALSE;
	case EXCEL_FIELDTYPE_DEFSTAT:				return sExcelGetDefaultStat(table, row, m_FieldData.wStat, m_FieldData.dwParam) == 0;
	case EXCEL_FIELDTYPE_DEFSTAT_FLOAT:			return sExcelGetDefaultStatFloat(table, row, m_FieldData.wStat, m_FieldData.dwParam) == 0.0f;
	case EXCEL_FIELDTYPE_UNITTYPES:				
		{
			for (unsigned int ii = 0; ii < m_nCount; ++ii)
			{
				if (((int *)fielddata)[ii] != 0)
				{
					return FALSE;
				}
			}
		}
	case EXCEL_FIELDTYPE_PARSE:					return FALSE;
	case EXCEL_FIELDTYPE_INHERIT_ROW:			return *(unsigned int *)fielddata == EXCEL_LINK_INVALID;
	default:
		ASSERT_RETFALSE(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK EXCEL_FIELD::LoadDirectTranslateLink(
	const struct EXCEL_TABLE * table, 
	unsigned int row,
	const char * token) const
{
	REF(row);
	EXCEL_LINK link = EXCEL_LINK_INVALID;
	if (m_FieldData.idTable == table->m_Index)
	{
		link = table->GetLineByStrKey(m_FieldData.idIndex, token);
	}
	else
	{
		link = ExcelGetLineByStrKey(m_FieldData.idTable, m_FieldData.idIndex, token);
	}
	if (link == EXCEL_LINK_INVALID && token[0] != 0)
	{
		ExcelWarn("EXCEL WARNING:  INDEX NOT FOUND(%d)  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", m_FieldData.idIndex, table->m_Name, row, m_ColumnName, token);
	}
	return link;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
BOOL EXCEL_FIELD::LoadDirectTranslateBasicType(
	BYTE * fielddata,
	const char * token) const
{
	T val = (T)PStrToInt(token);
	*(T *)fielddata = val;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
BOOL EXCEL_FIELD::LoadDirectTranslateBasicTypeArray(
	unsigned int row,
	BYTE * fielddata,
	char * token,
	T defaultVal) const
{
	REF(row);

	T * dest = (T *)fielddata;
	T * end = dest + m_nCount;
	char * cur = token;
	char * subtoken = NULL;
	unsigned int subtoklen = 0;
	while (dest < end && sExcelGetCommaDelimitedToken(cur, subtoken, subtoklen))
	{
		EXCEL_TOKENIZE<char> tokenizer(subtoken, subtoklen);
		T val = (T)PStrToInt(subtoken);
		*dest = val;
		dest++;
	}
	while (dest < end)
	{
		*dest = defaultVal;
		dest++;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
BOOL EXCEL_FIELD::LoadDirectTranslateLinkArray(
	EXCEL_TABLE * table,
	unsigned int row,	
	BYTE * fielddata,
	char * token,
	EXCEL_TRANSLATE_LINK fpTranslate,
	EXCEL_LINK max) const
{
    ASSERT_RETFALSE(fielddata);
	T * dest = (T *)fielddata;
	T * end = dest + m_nCount;
	char * cur = token;
	char * subtoken = NULL;
	unsigned int subtoklen = 0;
	while (dest < end && sExcelGetCommaDelimitedToken(cur, subtoken, subtoklen))
	{
		EXCEL_TOKENIZE<char> tokenizer(subtoken, subtoklen);
		EXCEL_LINK link = fpTranslate(table, row, this, subtoken);
		if (link != EXCEL_LINK_INVALID)
		{
			ASSERT(link < max);
			*dest = (T)link;
			dest++;
		}
	}

	EXCEL_LINK defaultLink = EXCEL_LINK_INVALID;
	if (dest < end && m_Default.str)
	{
		defaultLink = fpTranslate(table, row, this, m_Default.str);
		ASSERT(defaultLink == EXCEL_LINK_INVALID || defaultLink < max);
	}

	while (dest < end)
	{
		*dest = (T)defaultLink;
		dest++;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelIsTokenNone(
	char * token,
	unsigned int toklen)
{
	if (toklen != 4)
	{
		return FALSE;
	}
	if (token[0] == 'n' && token[1] == 'o' && token[2] == 'n' && token[3] == 'e')
	{
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_FIELD::LoadDirectTranslate(
	struct EXCEL_TABLE * table, 
	unsigned int row,
	BYTE * rowdata, 
	char * token,
	unsigned int toklen) const
{
	ASSERT_RETFALSE(rowdata);
	ASSERT_RETFALSE(token);

	char blanktok[2] = { 0, 0 };
	
	if (!(m_dwFlags & EXCEL_FIELD_NONE_IS_NOT_ZERO) && sExcelIsTokenNone(token, toklen))
	{
		token = blanktok;
		toklen = 0;
	}
	if (token[0] == 0)
	{
		if (!(m_dwFlags & EXCEL_FIELD_PARSE_NULL_TOKEN))
		{
			SetDefaultData(table, row, rowdata);
			return TRUE;
		}
	}

	BYTE * fielddata = rowdata + m_nOffset;

	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
		{
			SetStringData(fielddata, token, m_nCount);
			break;
		}
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
		{
			SetDynamicStringData(table, fielddata, token, toklen);
			break;
		}
	case EXCEL_FIELDTYPE_STRINT:
		{
			EXCEL_FIELD * field = (EXCEL_FIELD *)this;
			EXCEL_LOCK(CWRAutoLockWriter, &field->m_FieldData.csStrIntDict);
			if (!field->m_FieldData.strintdict)
			{
				field->m_FieldData.strintdict = (STRINT_DICT *)MALLOCZ(NULL, sizeof(STRINT_DICT));
				field->m_FieldData.strintdict->Init();
			}
			*(int *)fielddata = field->m_FieldData.strintdict->Add(token);
			break;
		}
	case EXCEL_FIELDTYPE_FLAG:
		{
			int val = 0;
			if (token[0] == 't')
			{
				val = 1;
			}
			else
			{
				val = PStrToInt(token);
			}
			ASSERT(val >= 0 && val <= 1);
			SETBIT(fielddata, m_FieldData.bit, val);
			break;
		}
	case EXCEL_FIELDTYPE_CHAR:
		return LoadDirectTranslateBasicType<char>(fielddata, token);
	case EXCEL_FIELDTYPE_BYTE:
		return LoadDirectTranslateBasicType<BYTE>(fielddata, token);
	case EXCEL_FIELDTYPE_SHORT:
		return LoadDirectTranslateBasicType<short>(fielddata, token);
	case EXCEL_FIELDTYPE_WORD:
		return LoadDirectTranslateBasicType<WORD>(fielddata, token);
	case EXCEL_FIELDTYPE_INT:
		return LoadDirectTranslateBasicType<int>(fielddata, token);
	case EXCEL_FIELDTYPE_DWORD:
		return LoadDirectTranslateBasicType<DWORD>(fielddata, token);
	case EXCEL_FIELDTYPE_BOOL:
		{
			int val = 0;
			if (token[0] == 't')
			{
				val = 1;
			}
			else
			{
				val = PStrToInt(token);
			}
			ASSERT(val >= 0 && val <= 1);
			*(BOOL *)fielddata = (val ? TRUE : FALSE);
			break;
		}
	case EXCEL_FIELDTYPE_FLOAT:
		{
			float floatval = (float)PStrToFloat(token);
			if (m_FieldData.fmin != 0.0f || m_FieldData.fmax != 0.0f)
			{
				if (floatval < m_FieldData.fmin || floatval > m_FieldData.fmax)
				{
					ASSERTV_MSG("Value for '%s' must be between 0.0 and 100.0 in table '%s'", m_ColumnName, table->m_Name);
					floatval = m_Default.floatval;
				}
			}
			*(float *)fielddata = floatval;
			break;
		}
	case EXCEL_FIELDTYPE_BYTE_PERCENT:
		{
			float floatval = (float)PStrToFloat(token);
			if (floatval < 0.0f || floatval > 100.0f)
			{
				ASSERTV_MSG("Value for '%s' must be between 0.0 and 100.0 in table '%s'", m_ColumnName, table->m_Name);
				*(BYTE *)fielddata = m_Default.byte;
				break;
			}
			int nPercent = (BYTE)(UCHAR_MAX * floatval / 100.0f  + 0.5);
			BYTE bytePercent = (BYTE)PIN(nPercent, 0, UCHAR_MAX);
			*(BYTE *)fielddata = bytePercent;
			break;
		}
	case EXCEL_FIELDTYPE_FOURCC:		
		{
			char tmp[5];
			PStrCopy(tmp, token, arrsize(tmp));
			*(DWORD *)fielddata = STRTOFOURCC(tmp);
			break;
		}
	case EXCEL_FIELDTYPE_THREECC:		
		{
			char tmp[4];
			PStrCopy(tmp, token, arrsize(tmp));
			*(DWORD *)fielddata = STRTOTHREECC(tmp);
			break;
		}
	case EXCEL_FIELDTYPE_TWOCC:		
		{
			char tmp[3];
			PStrCopy(tmp, token, arrsize(tmp));
			*(WORD *)fielddata = (WORD)STRTOTWOCC(tmp);
			break;
		}
	case EXCEL_FIELDTYPE_ONECC:	
		{
			*(BYTE *)fielddata = token[0];
			break;
		}
	case EXCEL_FIELDTYPE_LINK_CHAR:			
		{
			EXCEL_LINK link = LoadDirectTranslateLink(table, row, token);
			ASSERT(link == EXCEL_LINK_INVALID || link <= CHAR_MAX);
			*(char *)fielddata = (char)link;
			break;
		}
	case EXCEL_FIELDTYPE_LINK_SHORT:		
		{
			EXCEL_LINK link = LoadDirectTranslateLink(table, row, token);
			ASSERT(link == EXCEL_LINK_INVALID || link <= SHRT_MAX);
			*(short *)fielddata = (short)link;
			break;
		}
	case EXCEL_FIELDTYPE_LINK_INT:		
		{
			EXCEL_LINK link = LoadDirectTranslateLink(table, row, token);
			ASSERT(link == EXCEL_LINK_INVALID || link <= INT_MAX);
			*(int *)fielddata = (int)link;
			break;
		}
	case EXCEL_FIELDTYPE_LINK_FOURCC:		
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkFourCC(table, row, this, token);
			*(DWORD *)fielddata = (DWORD)link;
			break;
		}
	case EXCEL_FIELDTYPE_LINK_THREECC:		
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkThreeCC(table, row, this, token);
			*(DWORD *)fielddata = (DWORD)link;
			break;
		}
	case EXCEL_FIELDTYPE_LINK_TWOCC:		
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkTwoCC(table, row, this, token);
			ASSERT(link == EXCEL_LINK_INVALID || link < USHRT_MAX);
			*(WORD *)fielddata = (WORD)link;
			break;
		}
	case EXCEL_FIELDTYPE_LINK_ONECC:	
		{
			EXCEL_LINK link = sLoadDirectTranslateLinkOneCC(table, row, this, token);
			ASSERT(link == EXCEL_LINK_INVALID || link < UCHAR_MAX);
			*(BYTE *)fielddata = (BYTE)link;
			break;
		}
	case EXCEL_FIELDTYPE_LINK_TABLE:
		{
			EXCEL_LINK link = Excel().GetTableIdByName(token);
			ASSERT(link == EXCEL_LINK_INVALID || link < INT_MAX);
			*(int *)fielddata = (int)link;
			break;
		}
	case EXCEL_FIELDTYPE_LINK_STAT:
		{
			EXCEL_LINK link = LoadDirectTranslateLink(table, row, token);
			ASSERT(link == EXCEL_LINK_INVALID || link <= INT_MAX);
			if (link == EXCEL_LINK_INVALID)
			{
				break;
			}

			sExcelSetDefaultStat(table, row, m_FieldData.wLinkStat, 0, (int)link);
			break;
		}
	case EXCEL_FIELDTYPE_DICT_CHAR:		
		{
			ASSERT_BREAK(m_FieldData.strdict);
			int index = CAST_PTR_TO_INT(StrDictionaryFind(m_FieldData.strdict, token));
			ASSERT(index <= CHAR_MAX);
			if ((index < 0 || index > CHAR_MAX) && !(m_dwFlags & EXCEL_FIELD_DICTIONARY_ALLOW_NEGATIVE))
			{
				index = INVALID_LINK;
				if (token[0] != 0)
				{
					ExcelWarn("EXCEL WARNING:  DICT NOT FOUND  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", table->m_Name, row, m_ColumnName, token);
				}
			}
			*(char *)fielddata = (char)index;
			break;
		}
	case EXCEL_FIELDTYPE_DICT_SHORT:	
		{
			ASSERT_BREAK(m_FieldData.strdict);
			int index = CAST_PTR_TO_INT(StrDictionaryFind(m_FieldData.strdict, token));
			ASSERT(index <= SHRT_MAX);
			if ((index < 0 || index > SHRT_MAX) && !(m_dwFlags & EXCEL_FIELD_DICTIONARY_ALLOW_NEGATIVE))
			{
				index = INVALID_LINK;
				if (token[0] != 0)
				{
					ExcelWarn("EXCEL WARNING:  DICT NOT FOUND  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", table->m_Name, row, m_ColumnName, token);
				}
			}
			*(short *)fielddata = (short)index;
			break;
		}
	case EXCEL_FIELDTYPE_DICT_INT:
	case EXCEL_FIELDTYPE_DICT_INT_ARRAY:
		{
			ASSERT_BREAK(m_FieldData.strdict);
			int index = CAST_PTR_TO_INT(StrDictionaryFind(m_FieldData.strdict, token));
			ASSERT(index <= INT_MAX);
			if ((index < 0 || index > INT_MAX) && !(m_dwFlags & EXCEL_FIELD_DICTIONARY_ALLOW_NEGATIVE))
			{
				index = INVALID_LINK;
				if (token[0] != 0)
				{
					ExcelWarn("EXCEL WARNING:  DICT NOT FOUND  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", table->m_Name, row, m_ColumnName, token);
				}
			}
			if (m_Type == EXCEL_FIELDTYPE_DICT_INT)
			{
				*(int *)fielddata = (int)index;
			}
			else
			{
				return LoadDirectTranslateLinkArray<int>(table, row, fielddata, token, sLoadDirectTranslateDictLinkArray, (EXCEL_LINK)INT_MAX);
			}
			break;
		}
	case EXCEL_FIELDTYPE_CHAR_ARRAY:	
		return LoadDirectTranslateBasicTypeArray<char>(row, fielddata, token, m_Default.charval);
	case EXCEL_FIELDTYPE_BYTE_ARRAY:	
		return LoadDirectTranslateBasicTypeArray<BYTE>(row, fielddata, token, m_Default.byte);
	case EXCEL_FIELDTYPE_SHORT_ARRAY:	
		return LoadDirectTranslateBasicTypeArray<short>(row, fielddata, token, m_Default.shortval);
	case EXCEL_FIELDTYPE_WORD_ARRAY:	
		return LoadDirectTranslateBasicTypeArray<WORD>(row, fielddata, token, m_Default.word);
	case EXCEL_FIELDTYPE_INT_ARRAY:	
		return LoadDirectTranslateBasicTypeArray<int>(row, fielddata, token, m_Default.intval);
	case EXCEL_FIELDTYPE_DWORD_ARRAY:	
		return LoadDirectTranslateBasicTypeArray<DWORD>(row, fielddata, token, m_Default.dword);
	case EXCEL_FIELDTYPE_FLOAT_ARRAY:
		{
			float * dest = (float *)fielddata;
			float * end = dest + m_nCount;
			char * cur = token;
			char * subtoken = NULL;
			unsigned int subtoklen = 0;
			while (dest < end && sExcelGetCommaDelimitedToken(cur, subtoken, subtoklen))
			{
				EXCEL_TOKENIZE<char> tokenizer(subtoken, subtoklen);
				float val = (float)PStrToFloat(subtoken);
				*dest = val;
				dest++;
			}
			while (dest < end)
			{
				*dest = m_Default.floatval;
				dest++;
			}
			break;
		}
	case EXCEL_FIELDTYPE_LINK_BYTE_ARRAY:				
		return LoadDirectTranslateLinkArray<BYTE>(table, row, fielddata, token, sLoadDirectTranslateLinkInt, (EXCEL_LINK)(UCHAR_MAX - 1));
	case EXCEL_FIELDTYPE_LINK_WORD_ARRAY:				
		return LoadDirectTranslateLinkArray<WORD>(table, row, fielddata, token, sLoadDirectTranslateLinkInt, (EXCEL_LINK)(USHRT_MAX - 1));
	case EXCEL_FIELDTYPE_LINK_INT_ARRAY:
		return LoadDirectTranslateLinkArray<int>(table, row, fielddata, token, sLoadDirectTranslateLinkInt, (EXCEL_LINK)INT_MAX);
	case EXCEL_FIELDTYPE_LINK_FOURCC_ARRAY:	
		return LoadDirectTranslateLinkArray<DWORD>(table, row, fielddata, token, sLoadDirectTranslateLinkFourCC, (EXCEL_LINK)(UINT_MAX - 1));
	case EXCEL_FIELDTYPE_LINK_THREECC_ARRAY:
		return LoadDirectTranslateLinkArray<DWORD>(table, row, fielddata, token, sLoadDirectTranslateLinkThreeCC, (EXCEL_LINK)(UINT_MAX - 1));
	case EXCEL_FIELDTYPE_LINK_TWOCC_ARRAY:	
		return LoadDirectTranslateLinkArray<WORD>(table, row, fielddata, token, sLoadDirectTranslateLinkTwoCC, (EXCEL_LINK)(USHRT_MAX - 1));
	case EXCEL_FIELDTYPE_LINK_ONECC_ARRAY:	
		return LoadDirectTranslateLinkArray<BYTE>(table, row, fielddata, token, sLoadDirectTranslateLinkOneCC, (EXCEL_LINK)(UCHAR_MAX - 1));
	case EXCEL_FIELDTYPE_LINK_FLAG_ARRAY:
		{
			unsigned int max = m_nCount;

			char * cur = token;
			char * subtoken = NULL;
			unsigned int subtoklen = 0;
			while (sExcelGetCommaDelimitedToken(cur, subtoken, subtoklen))
			{
				EXCEL_TOKENIZE<char> tokenizer(subtoken, subtoklen);
				EXCEL_LINK link = sLoadDirectTranslateLinkInt(table, row, this, subtoken);
				if (link != EXCEL_LINK_INVALID)
				{
					ASSERT_CONTINUE(link <= max);
					SETBIT(fielddata, link);
				}
			}
			break;
		}
	case EXCEL_FIELDTYPE_STR_TABLE:				
		{	
			int nStringTable = LanguageGetCurrent();				
			*(int *)fielddata = StringTableCommonGetStringIndexByKey(nStringTable, token);
			if (*(int *)fielddata == EXCEL_STR_INVALID)
			{
				if (sFieldReportErrorsForApp( table, row, *this ))
				{
					ExcelLog("EXCEL WARNING:  STRING NOT FOUND  TABLE: %s   FIELD: %s   ROW: %d   KEY: %s", table->m_Name, m_ColumnName, row, token);
					ShowDataWarning(WARNING_TYPE_STRINGS, "string table entry not found for: table:%s  field:%s  row:%d  key:%s", table->m_Name, m_ColumnName, row, token);
				}
			}
		}
		break;
	case EXCEL_FIELDTYPE_STR_TABLE_ARRAY:
		{
			LoadDirectTranslateStringTableArray(table, row, fielddata, token);
			break;
		}
	case EXCEL_FIELDTYPE_FILEID:
	case EXCEL_FIELDTYPE_COOKED_FILEID:			
	{	
			ASSERT_BREAK(m_FieldData.filepath);

			char filename[MAX_PATH];
			PStrCopy(filename, token, arrsize(filename));

			if (m_Type == EXCEL_FIELDTYPE_COOKED_FILEID)
			{
				PStrCat(filename, ".", arrsize(filename));			
				PStrCat(filename, COOKED_FILE_EXTENSION, arrsize(filename));
			}

			char fullname[MAX_PATH];
			PStrPrintf(fullname, MAX_PATH, "%s%s", m_FieldData.filepath, filename);

			*(FILEID *)fielddata = PakFileGetFileId(fullname, m_FieldData.pakid);			
			break;
		}
	case EXCEL_FIELDTYPE_CODE:					
		{
			EXCEL_SCRIPTPARSE * fpScriptParse = Excel().m_fpScriptParseFunc;
			ASSERT_BREAK(fpScriptParse);

			char strError[1024];
#if ISVERSION(DEVELOPMENT)
			PStrPrintf(strError, arrsize(strError), "EXCEL WARNING:  PARSE ERROR  TABLE: %s   LINE: %d   FIELD: %s   TOKEN: %s\n", table->m_Name, row, m_ColumnName, token);
#endif

			TABLE_DATA_WRITE_LOCK(table);

			unsigned int buflen;
			BYTE * buf = table->GetCodeBufferSpace(buflen);
			ASSERT_BREAK(buf);

			BYTE * end = fpScriptParse(NULL, token, buf, buflen, EXCEL_LOG, strError);
			if (!end || end == buf)
			{
#if ISVERSION(DEBUG_VERSION)
				/*if( AppIsTugboat()  &&
					table->m_Name != "properties" )
				{
					ASSERT_MSG( strError )	
					
				}*/
#endif
				*(PCODE *)fielddata = NULL_CODE;
				break;
			}
			*(PCODE *)fielddata = table->SetCodeBuffer(end);
			break;
		}
	case EXCEL_FIELDTYPE_DEFCODE:
		{
			ASSERT_BREAK(table->m_Struct && table->m_Struct->m_hasStatsList);
			EXCEL_SCRIPTPARSE * fpScriptParse = Excel().m_fpScriptParseFunc;
			ASSERT_BREAK(fpScriptParse);

			EXCEL_SCRIPT_EXECUTE * fpScriptExecute = Excel().m_fpScriptExecute;
			ASSERT_BREAK(fpScriptExecute);

			static const int EXCEL_DEFCODE_BUFSIZE = EXCEL_TABLE::EXCEL_CODE_BUFSIZE;

			BYTE buf[EXCEL_DEFCODE_BUFSIZE];
			unsigned int buflen = arrsize(buf);

			char strError[1024];
			strError[0] = 0;
#if ISVERSION(DEVELOPMENT)
			PStrPrintf(strError, arrsize(strError), "EXCEL WARNING:  PARSE ERROR  TABLE: %s   LINE: %d   FIELD: %s   TOKEN: %s\n", table->m_Name, row, m_ColumnName, token);
#endif
			BYTE * end = fpScriptParse(NULL, token, buf, buflen, EXCEL_LOG, strError);
			if (!end || end == buf)
			{
				ASSERTV_MSG("table:%s  line:%d  field:%s  code:%s\n", table->m_Name, row, m_ColumnName, token);
				break;
			}
			
			TABLE_DATA_WRITE_LOCK(table);

			STATS * stats = table->GetOrCreateStatsList(row);
			ASSERT_BREAK(stats);

#if ISVERSION(DEVELOPMENT)
			PStrPrintf(strError, arrsize(strError), "EXCEL WARNING:  EXECUTION ERROR  TABLE: %s   FIELD: %s   TOKEN: %s\n", table->m_Name, m_ColumnName, token);
#endif
			fpScriptExecute(NULL, NULL, NULL, stats, buf, SIZE_TO_INT(end - buf), strError);
			break;
		}
	case EXCEL_FIELDTYPE_DEFSTAT:
		{
			int val = PStrToInt(token);
			if (val == 0)
			{
				break;
			}

			sExcelSetDefaultStat(table, row, m_FieldData.wStat, m_FieldData.dwParam, val);
			break;
		}
	case EXCEL_FIELDTYPE_DEFSTAT_FLOAT:
		{
			float val = PStrToFloat(token);
			if (val == 0.0f)
			{
				break;
			}

			sExcelSetDefaultStatFloat(table, row, m_FieldData.wStat, m_FieldData.dwParam, val);
			break;
		}
	case EXCEL_FIELDTYPE_UNITTYPES:
		{
			LoadDirectTranslateUnitTypes(table, row, fielddata, token);
			break;
		}
	case EXCEL_FIELDTYPE_PARSE:
		{
			ASSERT_BREAK(m_FieldData.fpParseFunc);
			m_FieldData.fpParseFunc(table, this, row, fielddata, token, PStrLen(token), m_FieldData.param);
			break;
		}
	case EXCEL_FIELDTYPE_INHERIT_ROW:
		break;
	case EXCEL_FIELDTYPE_VERSION_APP:
		{
			ASSERT_RETFALSE(row < table->m_Data.Count());
			EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(table->m_Data[row]);
			ASSERT_BREAK(version);
			sLoadDirectTranslateVersionFlags(table, this, row, Excel().m_VersionAppDict, version->m_VersionApp, token);
			break;
		}
	case EXCEL_FIELDTYPE_VERSION_PACKAGE:
		{
			ASSERT_RETFALSE(row < table->m_Data.Count());
			EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(table->m_Data[row]);
			ASSERT_BREAK(version);
			sLoadDirectTranslateVersionFlags(table, this, row, Excel().m_VersionPackageFlagsDict, version->m_VersionPackage, token);
			break;
		}
	case EXCEL_FIELDTYPE_VERSION_MAJOR:
		{
			ASSERT_RETFALSE(row < table->m_Data.Count());
			EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(table->m_Data[row]);
			ASSERT_BREAK(version);
			sLoadDirectTranslateVersionMinMax(table, this, row, version->m_VersionMajorMin, version->m_VersionMajorMax, token);
			break;
		}
	case EXCEL_FIELDTYPE_VERSION_MINOR:
		{
			ASSERT_RETFALSE(row < table->m_Data.Count());
			EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(table->m_Data[row]);
			ASSERT_BREAK(version);
			sLoadDirectTranslateVersionMinMax(table, this, row, version->m_VersionMinorMin, version->m_VersionMinorMax, token);
			break;
		}
	default:
		ASSERT_RETFALSE(0);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
int EXCEL_FIELD::LookupArray(
	const EXCEL_CONTEXT & context,
	const BYTE * fielddata,
	unsigned int arrayindex) const
{
	REF(context);

	ASSERT_RETZERO(arrayindex < m_nCount);

	const T * ptr = (const T *)fielddata + arrayindex;
	return (int)*ptr;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int EXCEL_FIELD::Lookup(
	const EXCEL_CONTEXT & context,
	const struct EXCEL_TABLE * table, 
	unsigned int row,
	const BYTE * fielddata,
	unsigned int arrayindex) const
{
	REF(context);

	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:				
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:		ASSERT_RETZERO(0);										
	case EXCEL_FIELDTYPE_STRINT:				return (int)*(const int *)fielddata;	
	case EXCEL_FIELDTYPE_FLAG:					return (int)TESTBIT(fielddata, m_FieldData.bit);
	case EXCEL_FIELDTYPE_CHAR:					return (int)*(const char *)fielddata;	
	case EXCEL_FIELDTYPE_BYTE:					return (int)*(const BYTE *)fielddata;	
	case EXCEL_FIELDTYPE_SHORT:					return (int)*(const short *)fielddata;	
	case EXCEL_FIELDTYPE_WORD:					return (int)*(const WORD *)fielddata;	
	case EXCEL_FIELDTYPE_INT:					return (int)*(const int *)fielddata;	
	case EXCEL_FIELDTYPE_DWORD:					return (int)*(const DWORD *)fielddata;	
	case EXCEL_FIELDTYPE_BOOL:					return (int)*(const BOOL *)fielddata;	
	case EXCEL_FIELDTYPE_FLOAT:					return (int)*(const float *)fielddata;	
	case EXCEL_FIELDTYPE_BYTE_PERCENT:			return (int)*(const BYTE *)fielddata;	
	case EXCEL_FIELDTYPE_FOURCC:		
	case EXCEL_FIELDTYPE_THREECC:				return (int)*(const DWORD *)fielddata;	
	case EXCEL_FIELDTYPE_TWOCC:					return (int)*(const WORD *)fielddata;	
	case EXCEL_FIELDTYPE_ONECC:					return (int)*(const BYTE *)fielddata;	
	case EXCEL_FIELDTYPE_LINK_INT:				return (int)*(const int *)fielddata;	
	case EXCEL_FIELDTYPE_LINK_CHAR:				return (int)*(const char *)fielddata;	
	case EXCEL_FIELDTYPE_LINK_SHORT:			return (int)*(const short *)fielddata;	
	case EXCEL_FIELDTYPE_LINK_FOURCC:			return (int)*(const DWORD *)fielddata;	
	case EXCEL_FIELDTYPE_LINK_THREECC:			return (int)*(const DWORD *)fielddata;	
	case EXCEL_FIELDTYPE_LINK_TWOCC:			return (int)*(const WORD *)fielddata;	
	case EXCEL_FIELDTYPE_LINK_ONECC:			return (int)*(const BYTE *)fielddata;	
	case EXCEL_FIELDTYPE_LINK_TABLE:			return (int)*(const int *)fielddata;	
	case EXCEL_FIELDTYPE_LINK_STAT:				return sExcelGetDefaultStat(table, row, m_FieldData.wLinkStat, 0);
	case EXCEL_FIELDTYPE_DICT_CHAR:				return (int)*(const char *)fielddata;						
	case EXCEL_FIELDTYPE_DICT_SHORT:			return (int)*(const short *)fielddata;						
	case EXCEL_FIELDTYPE_DICT_INT:				return (int)*(const int *)fielddata;						
	case EXCEL_FIELDTYPE_DICT_INT_ARRAY:		return LookupArray<int>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_CHAR_ARRAY:			return LookupArray<char>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_BYTE_ARRAY:			return LookupArray<BYTE>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_SHORT_ARRAY:			return LookupArray<short>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_WORD_ARRAY:			return LookupArray<WORD>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_INT_ARRAY:				return LookupArray<int>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_DWORD_ARRAY:			return LookupArray<DWORD>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_FLOAT_ARRAY:			return LookupArray<float>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_LINK_BYTE_ARRAY:		return LookupArray<BYTE>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_LINK_WORD_ARRAY:		return LookupArray<WORD>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_LINK_INT_ARRAY:		return LookupArray<int>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_LINK_FOURCC_ARRAY:		return LookupArray<DWORD>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_LINK_THREECC_ARRAY:	return LookupArray<DWORD>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_LINK_TWOCC_ARRAY:		return LookupArray<WORD>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_LINK_ONECC_ARRAY:		return LookupArray<BYTE>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_STR_TABLE:				return (int)*(const int *)fielddata;						
	case EXCEL_FIELDTYPE_STR_TABLE_ARRAY:		return LookupArray<int>(context, fielddata, arrayindex);	
	case EXCEL_FIELDTYPE_FILEID:				ASSERT_RETZERO(0);										
	case EXCEL_FIELDTYPE_COOKED_FILEID:			ASSERT_RETZERO(0);										
	case EXCEL_FIELDTYPE_CODE:
		{
			return table->EvalScriptNoLock(context, NULL, NULL, NULL, m_nOffset, row);
		}
	case EXCEL_FIELDTYPE_UNITTYPES:				return LookupArray<int>(context, fielddata, arrayindex);
	case EXCEL_FIELDTYPE_DEFCODE:				ASSERT_RETZERO(0);
	case EXCEL_FIELDTYPE_DEFSTAT:				return sExcelGetDefaultStat(table, row, m_FieldData.wStat, m_FieldData.dwParam);
	case EXCEL_FIELDTYPE_DEFSTAT_FLOAT:			return (int)sExcelGetDefaultStatFloat(table, row, m_FieldData.wStat, m_FieldData.dwParam);
	case EXCEL_FIELDTYPE_PARSE:					ASSERT_RETZERO(0);
	default:									ASSERT(0);
	}
	return 0;
}


//----------------------------------------------------------------------------
// TraceData() helper function
//----------------------------------------------------------------------------
#if EXCEL_TRACE
void EXCEL_FIELD::TraceLink(
	const struct EXCEL_TABLE * table, 
	EXCEL_LINK link) const
{
	ASSERT_RETURN(table);
	if (link == EXCEL_LINK_INVALID)
	{
		ExcelTrace("null");
		return;
	}

	if (m_FieldData.idTable == table->m_Index)
	{
		table->TraceLink(m_FieldData.idIndex, link);			
	}
	else
	{
		Excel().TraceLink(m_FieldData.idTable, m_FieldData.idIndex, link);
	}
}
#endif


//----------------------------------------------------------------------------
// TraceData() helper function
//----------------------------------------------------------------------------
#if EXCEL_TRACE
void EXCEL_FIELD::TraceLinkTable(
	const struct EXCEL_TABLE * table, 
	EXCEL_LINK link) const
{
	ASSERT_RETURN(table);
	if (link == EXCEL_LINK_INVALID)
	{
		ExcelTrace("null");
		return;
	}

	if ((unsigned int)link == table->m_Index)
	{
		ASSERT_RETURN(table->m_Name);
		ExcelTrace("[%s]", table->m_Name);
	}
	else
	{
		Excel().TraceLinkTable((unsigned int)link);
	}
}
#endif


//----------------------------------------------------------------------------
// TraceData() helper function
//----------------------------------------------------------------------------
#if EXCEL_TRACE
void EXCEL_FIELD::TraceDict(
	EXCEL_LINK link) const
{
	if (link == EXCEL_LINK_INVALID)
	{
		ExcelTrace("null");
		return;
	}

	ASSERT_RETURN(m_FieldData.strdict);
	const char * key = StrDictionaryFindNameByValue(m_FieldData.strdict, CAST_TO_VOIDPTR(link));
	if (!key)
	{
		ExcelTrace("null");
		return;
	}
	ExcelTrace("[%s]", key);
}
#endif

//----------------------------------------------------------------------------
// TraceData() helper function
//----------------------------------------------------------------------------
#if EXCEL_TRACE
template <typename T>
void EXCEL_FIELD::TraceDictArray(
	const struct EXCEL_TABLE * /*table*/,
	const BYTE * fielddata) const
{
	ExcelTrace("[");
	const T * ptr = (const T *)fielddata;
	const T * end = ptr + m_nCount;
	while (ptr < end - 1)
	{
		const char * key = StrDictionaryFindNameByValue(m_FieldData.strdict, CAST_TO_VOIDPTR(*ptr));
		if (!key)
		{
			ExcelTrace("null");
		}
		ExcelTrace("[%s]", key);
		ExcelTrace(", ");
		++ptr;
	}
	if (ptr < end)
	{
		const char * key = StrDictionaryFindNameByValue(m_FieldData.strdict, CAST_TO_VOIDPTR(*ptr));
		if (!key)
		{
			ExcelTrace("null");
		}
		ExcelTrace("[%s]", key);
	}
	ExcelTrace("]");
	
}
#endif

//----------------------------------------------------------------------------
// TraceData() helper function
//----------------------------------------------------------------------------
#if EXCEL_TRACE
template <typename T>
void EXCEL_FIELD::TraceBasicTypeArray(
	const BYTE * fielddata,
	const char * fmt) const
{
	ExcelTrace("[");
	const T * ptr = (const T *)fielddata;
	const T * end = ptr + m_nCount;
	while (ptr < end - 1)
	{
		ExcelTrace(fmt, *ptr);
		ExcelTrace(", ");
		++ptr;
	}
	if (ptr < end)
	{
		ExcelTrace(fmt, *ptr);
	}
	ExcelTrace("]");
}
#endif


//----------------------------------------------------------------------------
// TraceData() helper function
//----------------------------------------------------------------------------
#if EXCEL_TRACE
template <typename T>
void EXCEL_FIELD::TraceLinkArray(
	const struct EXCEL_TABLE * table,
	const BYTE * fielddata) const
{
	ExcelTrace("[");
	const T * ptr = (const T *)fielddata;
	const T * end = ptr + m_nCount;
	BOOL bComma = FALSE;
	while (ptr < end)
	{
		EXCEL_LINK link = sExcelExpandLink<T>(*ptr);
		if (link != EXCEL_LINK_INVALID)
		{
			if (bComma)
			{
				ExcelTrace(", ");
			}
			TraceLink(table, link);
			bComma = TRUE;
		}
		++ptr;
	}
	ExcelTrace("]");
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if EXCEL_TRACE
void EXCEL_FIELD::TraceData(
	const struct EXCEL_TABLE * table, 
	unsigned int row,
	const BYTE * rowdata) const
{
	ASSERT_RETURN(table);
	ASSERT_RETURN(rowdata);

	const BYTE * fielddata = rowdata + m_nOffset;

	char tmp[256];

	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:				ExcelTrace((const char *)fielddata);								break;
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:		ExcelTrace(*(const char **)fielddata);								break;
	case EXCEL_FIELDTYPE_FLAG:					ExcelTrace("%s", TESTBIT(fielddata, m_FieldData.bit) ? "T" : "F");	break;
	case EXCEL_FIELDTYPE_CHAR:					ExcelTrace("%d", *(const char *)fielddata);							break;
	case EXCEL_FIELDTYPE_BYTE:					ExcelTrace("%u", *(const BYTE *)fielddata);							break;
	case EXCEL_FIELDTYPE_SHORT:					ExcelTrace("%d", *(const short *)fielddata);						break;
	case EXCEL_FIELDTYPE_WORD:					ExcelTrace("%u", *(const WORD *)fielddata);							break;
	case EXCEL_FIELDTYPE_INT:					ExcelTrace("%d", *(const int *)fielddata);							break;
	case EXCEL_FIELDTYPE_DWORD:					ExcelTrace("%u", *(const DWORD *)fielddata);						break;
	case EXCEL_FIELDTYPE_BOOL:					ExcelTrace("%s", *(const BOOL *)fielddata ? "T" : "F");				break;
	case EXCEL_FIELDTYPE_FLOAT:					ExcelTrace("%.2f", *(const float *)fielddata);						break;
	case EXCEL_FIELDTYPE_BYTE_PERCENT:			ExcelTrace("%.2f%", (float)*(const BYTE *)fielddata / 256.0f);		break;
	case EXCEL_FIELDTYPE_FOURCC:		
	case EXCEL_FIELDTYPE_THREECC:				ExcelTrace("%s", FOURCCTOSTR(*(const DWORD *)fielddata, tmp));		break;
	case EXCEL_FIELDTYPE_TWOCC:					ExcelTrace("%s", TWOCCTOSTR(*(const WORD *)fielddata, tmp));		break;
	case EXCEL_FIELDTYPE_ONECC:					ExcelTrace("%s", ONECCTOSTR(*(const BYTE *)fielddata, tmp));		break;
	case EXCEL_FIELDTYPE_LINK_INT:				TraceLink(table, (EXCEL_LINK)*(const int *)fielddata);				break;
	case EXCEL_FIELDTYPE_LINK_CHAR:				TraceLink(table, (EXCEL_LINK)*(const char *)fielddata);				break;
	case EXCEL_FIELDTYPE_LINK_SHORT:			TraceLink(table, (EXCEL_LINK)*(const short *)fielddata);			break;
	case EXCEL_FIELDTYPE_LINK_FOURCC:			TraceLink(table, (EXCEL_LINK)*(const DWORD *)fielddata);			break;
	case EXCEL_FIELDTYPE_LINK_THREECC:			TraceLink(table, (EXCEL_LINK)*(const DWORD *)fielddata);			break;
	case EXCEL_FIELDTYPE_LINK_TWOCC:			TraceLink(table, (EXCEL_LINK)*(const WORD *)fielddata);				break;
	case EXCEL_FIELDTYPE_LINK_ONECC:			TraceLink(table, (EXCEL_LINK)*(const BYTE *)fielddata);				break;
	case EXCEL_FIELDTYPE_LINK_TABLE:			TraceLinkTable(table, (EXCEL_LINK)*(const int *)fielddata);			break;
	case EXCEL_FIELDTYPE_LINK_STAT:				TraceLink(table, (EXCEL_LINK)Lookup(NULL, table, row, fielddata));	break;
	case EXCEL_FIELDTYPE_DICT_CHAR:				TraceDict(*(const char *)fielddata);								break;
	case EXCEL_FIELDTYPE_DICT_SHORT:			TraceDict(*(const short *)fielddata);								break;
	case EXCEL_FIELDTYPE_DICT_INT:				TraceDict(*(const int *)fielddata);									break;
	case EXCEL_FIELDTYPE_DICT_INT_ARRAY:		TraceDictArray<int>(table, fielddata);									break;
	case EXCEL_FIELDTYPE_CHAR_ARRAY:			TraceBasicTypeArray<char>(fielddata, "%d");							break;
	case EXCEL_FIELDTYPE_BYTE_ARRAY:			TraceBasicTypeArray<BYTE>(fielddata, "%u");							break;
	case EXCEL_FIELDTYPE_SHORT_ARRAY:			TraceBasicTypeArray<short>(fielddata, "%d");						break;
	case EXCEL_FIELDTYPE_WORD_ARRAY:			TraceBasicTypeArray<WORD>(fielddata, "%u");							break;
	case EXCEL_FIELDTYPE_INT_ARRAY:				TraceBasicTypeArray<int>(fielddata, "%d");							break;
	case EXCEL_FIELDTYPE_DWORD_ARRAY:			TraceBasicTypeArray<DWORD>(fielddata, "%u");						break;
	case EXCEL_FIELDTYPE_FLOAT_ARRAY:			TraceBasicTypeArray<float>(fielddata, "%.2f");						break;
	case EXCEL_FIELDTYPE_LINK_BYTE_ARRAY:		TraceLinkArray<BYTE>(table, fielddata);								break;
	case EXCEL_FIELDTYPE_LINK_WORD_ARRAY:		TraceLinkArray<WORD>(table, fielddata);								break;
	case EXCEL_FIELDTYPE_LINK_INT_ARRAY:		TraceLinkArray<int>(table, fielddata);								break;
	case EXCEL_FIELDTYPE_LINK_FOURCC_ARRAY:		TraceLinkArray<DWORD>(table, fielddata);							break;
	case EXCEL_FIELDTYPE_LINK_THREECC_ARRAY:	TraceLinkArray<DWORD>(table, fielddata);							break;
	case EXCEL_FIELDTYPE_LINK_TWOCC_ARRAY:		TraceLinkArray<WORD>(table, fielddata);								break;
	case EXCEL_FIELDTYPE_LINK_ONECC_ARRAY:		TraceLinkArray<BYTE>(table, fielddata);								break;
	case EXCEL_FIELDTYPE_STR_TABLE:
	case EXCEL_FIELDTYPE_STR_TABLE_ARRAY:
		{
			int stridx = *(const int *)fielddata;
			if (stridx == EXCEL_STR_INVALID)
			{
				ExcelTrace("null");
				break;
			}
			int nStringTable = LanguageGetCurrent();			
			const WCHAR * str = StringTableCommonGetStringByIndex(nStringTable, stridx);
			if (!str)
			{
				ExcelTrace("null");
				break;
			}
			char tmp[256];
			PStrCvt(tmp, str, arrsize(tmp));
			ExcelTrace("[%d:%s]", stridx, tmp);
			break;
		}
	case EXCEL_FIELDTYPE_FILEID:
	case EXCEL_FIELDTYPE_COOKED_FILEID:			
		{
			FILEID fileid = *(const FILEID *)fielddata;
			if (fileid == INVALID_FILEID)
			{
				ExcelTrace("nullfile");
				break;
			}

			char filename[MAX_PATH];
			ASSERT_RETURN(PakFileGetFileName(fileid, filename, arrsize(filename)));
			ExcelTrace(filename);
			break;
		}
	case EXCEL_FIELDTYPE_CODE:
		{
			PCODE code = *(PCODE *)fielddata;
			if (code == NULL_CODE)
			{
				ExcelTrace("null");
			}
			else
			{
				ExcelTrace("$%d", (DWORD)code);
			}
			break;
		}
	case EXCEL_FIELDTYPE_UNITTYPES:
		{
			ExcelTrace("[");
			for (unsigned int ii = 0; ii < m_nCount; ++ii)
			{
			}
			ExcelTrace("]");
			break;
		}
	case EXCEL_FIELDTYPE_DEFSTAT:
		{
			ExcelTrace("%d", sExcelGetDefaultStat(table, row, m_FieldData.wStat, m_FieldData.dwParam));
			break;
		}
	case EXCEL_FIELDTYPE_DEFSTAT_FLOAT:
		{
			ExcelTrace("%.2f", sExcelGetDefaultStatFloat(table, row, m_FieldData.wStat, m_FieldData.dwParam));
			break;
		}
	case EXCEL_FIELDTYPE_DEFCODE:
	case EXCEL_FIELDTYPE_PARSE:							
		ExcelTrace("?");
		break;
	default:						ASSERT(0);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD sExcelComputeStringCRC(
	const char * str,
	DWORD crc)
{
	if (!str)
	{
		return crc;
	}
	return CRC_STRING(crc, str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD sExcelComputeStrDictionaryCRC(
	const STR_DICTIONARY * dict,
	DWORD crc)
{
	if (!dict)
	{
		return crc;
	}
	unsigned int count = (unsigned int)StrDictionaryGetCount(dict);
	crc = CRC(crc, (const BYTE *)&count, sizeof(count));
	for (unsigned int ii = 0; ii < count; ++ii)
	{
		const char * str = StrDictionaryGetStr(dict, (int)ii);
		ASSERT_CONTINUE(str);
		crc = CRC_STRING(crc, str);
		int value = CAST_PTR_TO_INT(StrDictionaryGet(dict, (int)ii)) - 1;
		crc = CRC(crc, (const BYTE *)&value, sizeof(value));
	}
	return crc;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD EXCEL_FIELD::ComputeCRC(
	void) const
{
	DWORD crc = 0;

	crc = CRC_STRING(crc, m_ColumnName);
	crc = CRC(crc, (const BYTE *)&m_Type, sizeof(m_Type));
	if (m_Type != EXCEL_FIELDTYPE_DYNAMIC_STRING)
	{
		crc = CRC(crc, (const BYTE *)&m_nOffset, sizeof(m_nOffset));
		crc = CRC(crc, (const BYTE *)&m_nElemSize, sizeof(m_nElemSize));
	}
	crc = CRC(crc, (const BYTE *)&m_nCount, sizeof(m_nCount));
	crc = CRC(crc, (const BYTE *)&m_dwFlags, sizeof(m_dwFlags));

	switch (m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_LINK_CHAR:			
	case EXCEL_FIELDTYPE_LINK_SHORT:		
	case EXCEL_FIELDTYPE_LINK_INT:			
	case EXCEL_FIELDTYPE_LINK_FOURCC:		
	case EXCEL_FIELDTYPE_LINK_THREECC:		
	case EXCEL_FIELDTYPE_LINK_TWOCC:		
	case EXCEL_FIELDTYPE_LINK_ONECC:		
	case EXCEL_FIELDTYPE_LINK_STAT:
	case EXCEL_FIELDTYPE_LINK_BYTE_ARRAY:	
	case EXCEL_FIELDTYPE_LINK_WORD_ARRAY:	
	case EXCEL_FIELDTYPE_LINK_INT_ARRAY:	
	case EXCEL_FIELDTYPE_LINK_FOURCC_ARRAY:	
	case EXCEL_FIELDTYPE_LINK_THREECC_ARRAY:
	case EXCEL_FIELDTYPE_LINK_TWOCC_ARRAY:	
	case EXCEL_FIELDTYPE_LINK_ONECC_ARRAY:	
	case EXCEL_FIELDTYPE_STR_TABLE:
	case EXCEL_FIELDTYPE_STR_TABLE_ARRAY:
	case EXCEL_FIELDTYPE_UNITTYPES:
		{
			crc = sExcelComputeStringCRC(m_Default.str, crc);
			crc = CRC(crc, (const BYTE *)&m_FieldData, sizeof(m_FieldData));
			break;
		}
	case EXCEL_FIELDTYPE_STRINT:
		{
			crc = CRC(crc, (const BYTE *)&m_Default, sizeof(m_Default));
			crc = sExcelComputeStrDictionaryCRC(m_FieldData.strdict, crc);
			break;
		}
	case EXCEL_FIELDTYPE_FILEID:
	case EXCEL_FIELDTYPE_COOKED_FILEID:
		crc = sExcelComputeStringCRC(m_FieldData.filepath, crc);
		break;
	case EXCEL_FIELDTYPE_DICT_CHAR:
	case EXCEL_FIELDTYPE_DICT_SHORT:
	case EXCEL_FIELDTYPE_DICT_INT:
	case EXCEL_FIELDTYPE_DICT_INT_ARRAY:
		{
			crc = sExcelComputeStringCRC(m_Default.str, crc);
			crc = sExcelComputeStrDictionaryCRC(m_FieldData.strdict, crc);
			break;
		}
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
		{
			crc = sExcelComputeStringCRC(m_Default.str, crc);
			crc = CRC(crc, (const BYTE *)&m_FieldData, sizeof(m_FieldData));
		}
		break;
	case EXCEL_FIELDTYPE_PARSE:
		break;
	default:
		crc = CRC(crc, (const BYTE *)&m_Default, sizeof(m_Default));
		crc = CRC(crc, (const BYTE *)&m_FieldData, sizeof(m_FieldData));
		break;
	}
	return crc;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD::InitDictField(
	char * str)
{
	ASSERT_RETURN(str);
	ASSERT_RETURN(!m_FieldData.strdict);

	char * subtoken = NULL;
	unsigned int subtoklen = 0;
	while (sExcelGetCommaDelimitedToken(str, subtoken, subtoklen))
	{
		EXCEL_TOKENIZE<char> tokenizer(subtoken, subtoklen);

		if (!m_FieldData.strdict)
		{
			m_FieldData.strdict = StrDictionaryInit(8, TRUE);
		}

		if (!StrDictionaryFind(m_FieldData.strdict, subtoken))
		{
			StrDictionaryAdd(m_FieldData.strdict, subtoken, CAST_TO_VOIDPTR(StrDictionaryGetCount(m_FieldData.strdict) + 1));
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_FIELD_DESTROY(
	MEMORYPOOL * pool,
	EXCEL_FIELD & field)
{
	REF(pool);
	field.Free();
}


//----------------------------------------------------------------------------
// EXCEL_INDEX_DESC:: FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_INDEX_DESC::Free(
	void)
{
}


//----------------------------------------------------------------------------
// EXCEL_STRUCT:: FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_STRUCT::Free(
	void)
{
	m_Fields.Destroy(NULL, EXCEL_FIELD_DESTROY);
	m_Ancestors.Destroy(NULL);
	m_Dependents.Destroy(NULL);

	// destroy indexes
	for (unsigned int ii = 0; ii < EXCEL_STRUCT::EXCEL_MAX_INDEXES; ++ii)
	{
		m_Indexes[ii].Free();
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_FIELD * EXCEL_STRUCT::GetFieldByName(
	const char * fieldname)
{
	ASSERT_RETNULL(fieldname);
	ASSERT_RETNULL(fieldname[0]);

	EXCEL_FIELD key(fieldname);
	return m_Fields.FindExact(key);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const EXCEL_FIELD * EXCEL_STRUCT::GetFieldByNameConst(
	const char * fieldname,
	unsigned int strlen) const
{
	ASSERT_RETNULL(fieldname && strlen > 0);

	unsigned int min = 0;
	unsigned int max = m_Fields.Count();
	unsigned int ii = min + (max - min) / 2;

	while (max > min)
	{
		int comp = PStrICmp(m_Fields[ii].m_ColumnName, fieldname, strlen);
		if (comp > 0 || (comp == 0 && m_Fields[ii].m_ColumnName[strlen] != 0))
		{
			max = ii;
		}
		else if (comp < 0)
		{
			min = ii + 1;
		}
		else
		{
			return &m_Fields[ii];
		}
		ii = min + (max - min) / 2;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_STRUCT::AddField(
	EXCEL_FIELD & field)
{
	ASSERT_RETURN(m_Fields.Count() < EXCEL_MAX_FIELDS);
	ASSERT_RETURN(field.m_ColumnName);
	ASSERT_RETURN(field.m_Type < NUM_EXCEL_FIELDTYPE);
	ASSERT_RETURN(field.m_nOffset < m_Size);
	switch (field.m_Type)
	{
	case EXCEL_FIELDTYPE_FLAG:
		ASSERT_RETURN(field.m_nOffset + field.m_nElemSize/8 <= m_Size);
		break;
	case EXCEL_FIELDTYPE_LINK_FLAG_ARRAY:
		ASSERT_RETURN(field.m_nOffset + field.m_nElemSize <= m_Size);
		break;
	case EXCEL_FIELDTYPE_CODE:
	case EXCEL_FIELDTYPE_DEFCODE:
		m_hasScriptField = TRUE;
		ASSERT_RETURN(field.m_nOffset + field.m_nElemSize * field.m_nCount <= m_Size);
		break;
	default:
		ASSERT_RETURN(field.m_nOffset + field.m_nElemSize * field.m_nCount <= m_Size);
		break;
	}
	field.m_nDefinedOrder = m_Fields.Count();

	if (field.m_ColumnName[0] == 0)
	{
		m_Fields.InsertAllowDuplicate(NULL, field);
	}
	else
	{
		ASSERT(m_Fields.Insert(NULL, field));
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_STRUCT::AddIndex(
	unsigned int idIndex,
	DWORD flags,
	const char * fieldname1,
	const char * fieldname2,
	const char * fieldname3)
{
	ASSERT_RETURN(idIndex < EXCEL_MAX_INDEXES);
	ASSERT_RETURN(fieldname1 && fieldname1[0]);

	ASSERT_RETURN(m_Indexes[idIndex].m_Type == EXCEL_INDEXTYPE_INVALID);
	structclear(m_Indexes[idIndex]);

	EXCEL_FIELD * field1 = GetFieldByName(fieldname1);
	ASSERT_RETURN(field1 && !(field1->m_dwFlags & EXCEL_FIELD_INHERIT_ROW));

	EXCEL_FIELD * field2 = NULL;
	if (fieldname2 && fieldname2[0])
	{
		field2 = GetFieldByName(fieldname2);
		ASSERT_RETURN(field2 && !(field2->m_dwFlags & EXCEL_FIELD_INHERIT_ROW));
	}

	EXCEL_FIELD * field3 = NULL;
	if (fieldname3 && fieldname3[0])
	{
		field3 = GetFieldByName(fieldname3);
		ASSERT_RETURN(field3 && !(field3->m_dwFlags & EXCEL_FIELD_INHERIT_ROW));
	}

	m_Indexes[idIndex].m_dwFlags = flags;

	m_Indexes[idIndex].m_IndexFields[0] = field1;
	m_Indexes[idIndex].m_IndexFields[0]->m_dwFlags |= EXCEL_FIELD_ISINDEX;

	if (field2)
	{
		m_Indexes[idIndex].m_IndexFields[1] = field2;
		m_Indexes[idIndex].m_IndexFields[1]->m_dwFlags |= EXCEL_FIELD_ISINDEX;
	}

	if (field3)
	{
		m_Indexes[idIndex].m_IndexFields[2] = field3;	
		m_Indexes[idIndex].m_IndexFields[2]->m_dwFlags |= EXCEL_FIELD_ISINDEX;
	}

	m_Indexes[idIndex].m_Type = EXCEL_INDEXTYPE_DEFAULT;
	m_isIndexed = TRUE;
	m_Indexes[idIndex].m_fpRowCompare = EXCEL_INDEX_COMPARE;
	m_Indexes[idIndex].m_fpKeyCompare = EXCEL_INDEX_COMPARE;

	if (field2 == NULL)
	{
		switch (field1->m_Type)
		{
		case EXCEL_FIELDTYPE_STRING:
			if (m_StringIndex == EXCEL_LINK_INVALID)
			{
				m_StringIndex = idIndex;
			}
			m_Indexes[idIndex].m_Type = EXCEL_INDEXTYPE_STRING;
			m_Indexes[idIndex].m_fpKeyCompare = EXCEL_KEY_COMPARE_STR;
			break;
		case EXCEL_FIELDTYPE_DYNAMIC_STRING:
			if (m_StringIndex == EXCEL_LINK_INVALID)
			{
				m_StringIndex = idIndex;
			}
			m_Indexes[idIndex].m_Type = EXCEL_INDEXTYPE_DYNAMIC_STRING;
			m_Indexes[idIndex].m_fpKeyCompare = EXCEL_KEY_COMPARE_DYNAMIC_STR;
			break;
		case EXCEL_FIELDTYPE_STRINT:
			if (m_StringIndex == EXCEL_LINK_INVALID)
			{
				m_StringIndex = idIndex;
			}
			m_Indexes[idIndex].m_Type = EXCEL_INDEXTYPE_STRINT;
			m_Indexes[idIndex].m_fpKeyPreprocess = EXCEL_KEY_PREPROCESS_STRINT;
			m_Indexes[idIndex].m_fpKeyCompare = EXCEL_KEY_COMPARE_STRINT;
			break;
		case EXCEL_FIELDTYPE_FOURCC:
		case EXCEL_FIELDTYPE_THREECC:
			if (m_CodeIndex == EXCEL_LINK_INVALID)
			{
				m_CodeIndex = idIndex;
			}
			m_Indexes[idIndex].m_Type = EXCEL_INDEXTYPE_FOURCC;
			break;
		case EXCEL_FIELDTYPE_TWOCC:
			if (m_CodeIndex == EXCEL_LINK_INVALID)
			{
				m_CodeIndex = idIndex;
			}
			m_Indexes[idIndex].m_Type = EXCEL_INDEXTYPE_TWOCC;
			break;
		case EXCEL_FIELDTYPE_ONECC:
			if (m_CodeIndex == EXCEL_LINK_INVALID)
			{
				m_CodeIndex = idIndex;
			}
			m_Indexes[idIndex].m_Type = EXCEL_INDEXTYPE_ONECC;
			break;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_STRUCT::SetNameField(
	const char * fieldname)
{
	ASSERT_RETURN(fieldname && fieldname[0]);

	const EXCEL_FIELD * field = GetFieldByName(fieldname);	
	ASSERT_RETURN(field);
	ASSERT_RETURN(field->m_Type == EXCEL_FIELDTYPE_STR_TABLE);
	m_NameField = field;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_STRUCT::SetIsAField(
	const char * fieldname)
{
	ASSERT_RETURN(fieldname && fieldname[0]);
	ASSERT_RETURN(m_IsAField == NULL);

	const EXCEL_FIELD * field = GetFieldByName(fieldname);	
	ASSERT_RETURN(field);
	ASSERT_RETURN(field->m_Type == EXCEL_FIELDTYPE_LINK_INT_ARRAY);		// some other possibilities might also work
	m_IsAField = field;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_STRUCT::RegisterEnd(
	void)
{
	if (m_usesInheritRow)
	{
		ASSERT(m_isIndexed);
	}

	// set a name field if we don't have one
	if (!m_NameField)
	{
		unsigned int order = UINT_MAX;
		for (unsigned int ii = 0; ii < m_Fields.Count(); ++ii)
		{
			EXCEL_FIELD & field = m_Fields[ii];
			if (field.m_Type == EXCEL_FIELDTYPE_STR_TABLE && field.m_nDefinedOrder < order)
			{
				m_NameField = &field;
				order = field.m_nDefinedOrder;
			}
		}
	}

	ComputeCRC();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_STRUCT::ComputeCRC(
	void)
{
	ExcelCRCTrace("EXCEL_STRUCT::ComputeCRC()\t%s\t%d\n", m_Name, m_Size);

	m_CRC32 = m_Size + EXCEL_TABLE_VERSION;
	m_CRC32 += sizeof(EXCEL_DATA_VERSION);

	unsigned int count = m_Fields.Count();
	for (unsigned int ii = 0; ii < count; ++ii)
	{
		m_CRC32 += m_Fields[ii].ComputeCRC();
		ExcelCRCTrace("\t%2d\t0x%08x\t%s\n", ii, m_CRC32, m_Fields[ii].m_ColumnName);
	}

	if (m_hasScriptField)
	{
		EXCEL_SCRIPT_GET_IMPORT_CRC * fpScriptGetImportCRC = Excel().m_fpScriptGetImportCRC;
		if (fpScriptGetImportCRC)
		{
			m_CRC32 += fpScriptGetImportCRC();
		}
	}
}


//----------------------------------------------------------------------------
// EXCEL_INDEX:: FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void EXCEL_INDEX_BSORT(
	const EXCEL_INDEX & index,
	EXCEL_INDEX_NODE * list,
	unsigned int left,
	unsigned int right)
{
	EXCEL_ROW_COMPARE * fpCompare = index.m_IndexDesc->m_fpRowCompare;
	ASSERT_RETURN(fpCompare);

	BOOL hasSwapped;
	do
	{
		hasSwapped = FALSE;
		for (unsigned int ii = left; ii < right; ++ii)
		{
			BYTE * rowdataA = ExcelGetDataFromRowData(list[ii]);
			BYTE * rowdataB = ExcelGetDataFromRowData(list[ii + 1]);
			ASSERT(rowdataA && rowdataB);
			int cmp = fpCompare(*index.m_IndexDesc, rowdataA, rowdataB);
			if (cmp > 0 || (cmp == 0 && rowdataA > rowdataB))
			{
				SWAP(list[ii], list[ii + 1]);
				hasSwapped = TRUE;
			}
		}
		if (right <= left)
		{
			break;
		}
		--right;		// largest element is in right, so don't bother sorting it
	} while (hasSwapped);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void EXCEL_INDEX_QSORT(
	const EXCEL_INDEX & index,
	EXCEL_INDEX_NODE * list,
	unsigned int left,
	unsigned int right)
{
	if (right - left < 4)	// special case small list (4 items or less)
	{
		EXCEL_INDEX_BSORT(index, list, left, right);
		return;
	}

	EXCEL_ROW_COMPARE * fpCompare = index.m_IndexDesc->m_fpRowCompare;
	ASSERT_RETURN(fpCompare);

	unsigned int oldLeft = left;
	unsigned int oldRight = right;
	unsigned int pivotIndex = left + (right - left + 1) / 2;
	EXCEL_INDEX_NODE pivot = list[pivotIndex];

	while (left < right)
	{
		// decrement right so long as the right node is greater than or equal to the pivot
		while (left < right)
		{
			BYTE * rowdata = ExcelGetDataFromRowData(list[right]);
			BYTE * pivotdata = ExcelGetDataFromRowData(pivot);
			int cmp = fpCompare(*index.m_IndexDesc, rowdata, pivotdata);
			if (cmp < 0 || (cmp == 0 && rowdata <= pivotdata))
			{
				break;
			}
			--right;
		}

		// exchange the left and right nodes (right node is less than pivot), increment left
		if (left != right && right != pivotIndex)
		{
			SWAP(list[left], list[right]);
			if (left == pivotIndex)
			{
				pivotIndex = right;
			}
			++left;
		}

		// increment left so long as the left node is less than or equal to the pivot
		while (left < right)
		{
			BYTE * rowdata = ExcelGetDataFromRowData(list[left]);
			BYTE * pivotdata = ExcelGetDataFromRowData(pivot);
			int cmp = fpCompare(*index.m_IndexDesc, rowdata, pivotdata);
			if (cmp > 0 || (cmp == 0 && rowdata >= pivotdata))
			{
				break;
			}
			++left;
		}

		// exchange the left and right nodes (left node is greater than pivot), decrement right
		if (left != right && left != pivotIndex)
		{
			SWAP(list[left], list[right]);
			if (right == pivotIndex)
			{
				pivotIndex = left;
			}
			--right;
		}
	}

	// we end with left == position we should put pivot value
	left = oldLeft;
	right = oldRight;
	if (left < pivotIndex)
	{
		EXCEL_INDEX_QSORT(index, list, left, pivotIndex - 1);
	}
	if (right > pivotIndex)
	{
		EXCEL_INDEX_QSORT(index, list, pivotIndex + 1, right);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_INDEX::Init(
	const EXCEL_INDEX_DESC * desc)
{
	m_IndexDesc = desc;
	m_IndexNodes.Init();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_INDEX::Free(
	void)
{
	m_IndexDesc = NULL;
	m_IndexNodes.Destroy(NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_INDEX::IsBlank(
	EXCEL_TABLE * table,
	unsigned int row,
	const BYTE * rowdata) const
{
	ASSERT_RETFALSE(m_IndexDesc);
	ASSERT_RETFALSE(rowdata);

	for (unsigned int ii = 0; ii < EXCEL_INDEX_DESC::EXCEL_MAX_INDEX_FIELDS; ++ii)
	{
		const EXCEL_FIELD * field = m_IndexDesc->m_IndexFields[ii];
		if (!field)
		{
			return TRUE;
		}
		const BYTE * fielddata = rowdata + field->m_nOffset;
		if (!field->IsBlank(table, row, fielddata))
		{
			return FALSE;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_INDEX::AddIndexNode(
	unsigned int row, 
	BYTE * rowdata,
	BYTE * data,
	unsigned int * duperow)
{
	BOOL bExact;
	unsigned int index = FindClosestIndex(data, bExact);
	if (bExact)
	{
		if (duperow)
		{
			*duperow = m_IndexNodes[index].m_RowIndex;
		}
		return FALSE;
	}

	EXCEL_INDEX_NODE node(row, rowdata);
	m_IndexNodes.InsertAtIndex(MEMORY_FUNC_FILELINE(NULL, index, node));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_INDEX::Sort(
	void)
{
	ASSERT_RETURN(m_IndexDesc);
	if (m_IndexNodes.Count() <= 1)
	{
		return;
	}

	EXCEL_ROW_COMPARE * fpCompare = m_IndexDesc->m_fpRowCompare;
	ASSERT_RETURN(fpCompare);

	EXCEL_INDEX_QSORT(*this, m_IndexNodes.GetPointer(), 0, m_IndexNodes.Count() - 1);

#if ISVERSION(DEVELOPMENT) && !ISVERSION(OPTIMIZED_VERSION)
	for (unsigned int ii = 0; ii < m_IndexNodes.Count() - 1; ++ii)
	{
		BYTE * rowdataA = ExcelGetDataFromRowData(m_IndexNodes[ii]);
		BYTE * rowdataB = ExcelGetDataFromRowData(m_IndexNodes[ii + 1]);
		ASSERT(fpCompare(*m_IndexDesc, rowdataA, rowdataB) <= 0);
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const EXCEL_INDEX_NODE * EXCEL_INDEX::Find(
	const BYTE * key) const
{
	ASSERT_RETNULL(m_IndexDesc);

	EXCEL_ROW_COMPARE * fpCompare = m_IndexDesc->m_fpRowCompare;
	ASSERT_RETNULL(fpCompare);

	EXCEL_INDEX_NODE * min = m_IndexNodes.GetPointer();
	EXCEL_INDEX_NODE * max = min + m_IndexNodes.Count();
	EXCEL_INDEX_NODE * ii = min + (max - min) / 2;

	while (max > min)
	{
		BYTE * rowdata = ExcelGetDataFromRowData(*ii);
		int cmp = fpCompare(*m_IndexDesc, rowdata, key);
		if (cmp > 0)
		{
			max = ii;
		}
		else if (cmp < 0)
		{
			min = ii + 1;
		}
		else
		{
			return ii;
		}
		ii = min + (max - min) / 2;
	}
	return NULL;
}


//----------------------------------------------------------------------------
// find closest node >= key
//----------------------------------------------------------------------------
const EXCEL_INDEX_NODE * EXCEL_INDEX::FindClosest(
	const BYTE * key,
	BOOL & bExact) const
{
	bExact = FALSE;
	ASSERT_RETNULL(m_IndexDesc);

	EXCEL_ROW_COMPARE * fpCompare = m_IndexDesc->m_fpRowCompare;
	ASSERT_RETNULL(fpCompare);

	EXCEL_INDEX_NODE * min = m_IndexNodes.GetPointer();
	EXCEL_INDEX_NODE * max = min + m_IndexNodes.Count();
	EXCEL_INDEX_NODE * ii = min + (max - min) / 2;

	while (max > min)
	{
		BYTE * rowdata = ExcelGetDataFromRowData(*ii);
		int cmp = fpCompare(*m_IndexDesc, rowdata, key);
		if (cmp > 0)
		{
			max = ii;
		}
		else if (cmp < 0)
		{
			min = ii + 1;
		}
		else
		{
			bExact = TRUE;
			return ii;
		}
		ii = min + (max - min) / 2;
	}
	return (max < m_IndexNodes.GetPointer() + m_IndexNodes.Count()) ? max : NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int EXCEL_INDEX::FindClosestIndex(
	const BYTE * key, 
	BOOL & bExact) const
{
	bExact = FALSE;
	ASSERT_RETNULL(m_IndexDesc);

	EXCEL_ROW_COMPARE * fpCompare = m_IndexDesc->m_fpRowCompare;
	ASSERT_RETNULL(fpCompare);

	EXCEL_INDEX_NODE * min = m_IndexNodes.GetPointer();
	EXCEL_INDEX_NODE * max = min + m_IndexNodes.Count();
	EXCEL_INDEX_NODE * ii = min + (max - min) / 2;

	while (max > min)
	{
		BYTE * rowdata = ExcelGetDataFromRowData(*ii);
		int cmp = fpCompare(*m_IndexDesc, rowdata, key);
		if (cmp > 0)
		{
			max = ii;
		}
		else if (cmp < 0)
		{
			min = ii + 1;
		}
		else
		{
			bExact = TRUE;
			return (unsigned int)(ii - m_IndexNodes.GetPointer());
		}
		ii = min + (max - min) / 2;
	}
	return (unsigned int)(max - m_IndexNodes.GetPointer());
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const EXCEL_INDEX_NODE * EXCEL_INDEX::FindByStrKey(
	const char * key) const
{
	ASSERT_RETNULL(m_IndexDesc);

	EXCEL_KEY_COMPARE * fpKeyCompare = m_IndexDesc->m_fpKeyCompare;
	ASSERT_RETNULL(fpKeyCompare);

	const EXCEL_FIELD * field = m_IndexDesc->m_IndexFields[0];
	ASSERT_RETNULL(field);

	const BYTE * keyptr = (BYTE *)key;
	if (m_IndexDesc->m_fpKeyPreprocess)
	{
		keyptr = m_IndexDesc->m_fpKeyPreprocess(*m_IndexDesc, (const BYTE *)key);
	}

	unsigned int offset = field->m_nOffset;

	EXCEL_INDEX_NODE * min = m_IndexNodes.GetPointer();
	EXCEL_INDEX_NODE * max = min + m_IndexNodes.Count();
	EXCEL_INDEX_NODE * ii = min + (max - min) / 2;

	while (max > min)
	{
		BYTE * rowdata = ExcelGetDataFromRowData(*ii);
		const BYTE * fielddata = rowdata + offset;

		int cmp = fpKeyCompare(*m_IndexDesc, fielddata, keyptr);
		if (cmp > 0)
		{
			max = ii;
		}
		else if (cmp < 0)
		{
			min = ii + 1;
		}
		else
		{
			return ii;
		}
		ii = min + (max - min) / 2;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const EXCEL_INDEX_NODE * EXCEL_INDEX::FindClosestByStrKey(
	const char * key,
	BOOL & bExact) const
{
	bExact = FALSE;
	ASSERT_RETNULL(m_IndexDesc);

	EXCEL_KEY_COMPARE * fpKeyCompare = m_IndexDesc->m_fpKeyCompare;
	ASSERT_RETNULL(fpKeyCompare);

	const EXCEL_FIELD * field = m_IndexDesc->m_IndexFields[0];
	ASSERT_RETNULL(field);

	const BYTE * keyptr = (BYTE *)key;
	if (m_IndexDesc->m_fpKeyPreprocess)
	{
		keyptr = m_IndexDesc->m_fpKeyPreprocess(*m_IndexDesc, (const BYTE *)key);
	}

	unsigned int offset = field->m_nOffset;

	EXCEL_INDEX_NODE * min = m_IndexNodes.GetPointer();
	EXCEL_INDEX_NODE * max = min + m_IndexNodes.Count();
	EXCEL_INDEX_NODE * ii = min + (max - min) / 2;

	while (max > min)
	{
		BYTE * rowdata = ExcelGetDataFromRowData(*ii);
		const BYTE * fielddata = rowdata + offset;

		int cmp = fpKeyCompare(*m_IndexDesc, fielddata, keyptr);
		if (cmp > 0)
		{
			max = ii;
		}
		else if (cmp < 0)
		{
			min = ii + 1;
		}
		else
		{
			bExact = TRUE;
			return ii;
		}
		ii = min + (max - min) / 2;
	}
	return (max < m_IndexNodes.GetPointer() + m_IndexNodes.Count()) ? max : NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
const EXCEL_INDEX_NODE * EXCEL_INDEX::FindByCC(
	const EXCEL_TABLE * table,
	T cc) const
{
	ASSERT_RETNULL(table);
//	ASSERT_RETNULL(table->m_Key);

	const EXCEL_FIELD * field = m_IndexDesc->m_IndexFields[0];
	ASSERT_RETNULL(field);
	ASSERT_RETNULL(field->m_nElemSize == sizeof(T))
	ASSERT_RETNULL(m_IndexDesc->m_IndexFields[1] == NULL);

	BYTE key[32768];
	DBG_ASSERT(arrsize(key) >= field->m_nOffset + 8);
	*(T *)(key + field->m_nOffset) = cc;

	return Find(key);
}


//----------------------------------------------------------------------------
// EXCEL_TABLE:: FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_DATA_FREE(
	MEMORYPOOL * pool,
	EXCEL_DATA & row,
	void * vpTable)
{
	EXCEL_TABLE * table = (EXCEL_TABLE *)vpTable;
	ASSERT_RETURN(table);
	ASSERT_RETURN(table->m_Struct);

	EXCEL_DATA * curr = NULL;
	EXCEL_DATA * next = &row;
	while ((curr = next) != NULL)
	{
		next = curr->m_Prev;
		if (curr->m_RowData)
		{
			if (table->m_Struct->m_fpRowFree)
			{
				table->m_Struct->m_fpRowFree(table, ExcelGetDataFromRowData(*curr));
			}
			if (TEST_MASK(curr->m_dwFlags, EXCEL_DATA_FLAG_FREE))
			{
				FREE(pool, curr->m_RowData);
			}
		}
		if (curr->m_Stats)
		{
			EXCEL_STATSLIST_FREE * fpStatsListFree = Excel().m_fpStatsListFree;
			ASSERT(fpStatsListFree);
			if (fpStatsListFree)
			{
				fpStatsListFree(NULL, curr->m_Stats);
			}
		}
		if (curr != &row)
		{
			FREE(pool, curr);
		}
	}
}


//----------------------------------------------------------------------------
// free a dynamically allocated field
//----------------------------------------------------------------------------
void EXCEL_DYNAMIC_FIELD_DESTROY(
	MEMORYPOOL * pool,
	EXCEL_FIELD * field)
{
	ASSERT_RETURN(field);
	if (field->m_ColumnName)
	{
		FREE(pool, field->m_ColumnName);
	}
	FREE(pool, field);
}


//----------------------------------------------------------------------------
// call back function for destroying column headers after parsing
//----------------------------------------------------------------------------
void EXCEL_HEADER_DESTROY(
	MEMORYPOOL * pool,
	EXCEL_HEADER & header)
{
	if (header.m_Field && TESTBIT(header.m_dwFieldFlags, EXCEL_HEADER_FREE))
	{
		EXCEL_DYNAMIC_FIELD_DESTROY(pool, (EXCEL_FIELD *)header.m_Field);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_TABLE_FILE_DESTROY(
	MEMORYPOOL * pool,
	EXCEL_TABLE_FILE & file)
{
	if (file.m_TextBuffer)
	{
		FREE(pool, file.m_TextBuffer);
		file.m_TextBuffer = NULL;
	}
	file.m_TextBufferSize = 0;
	file.m_TextDataStart = NULL;
	file.m_TextHeaders.Destroy(pool, EXCEL_HEADER_DESTROY);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_TABLE_BUFFER_FREE(
	MEMORYPOOL * pool,
	const BYTE * & buffer)
{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	REF(pool);
	FREE(g_StaticAllocator, buffer);
#else
	FREE(pool, buffer);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_TABLE::Free(
	void)
{
	FreeParseData();

	m_Files.Destroy(NULL, EXCEL_TABLE_FILE_DESTROY);

	if (m_Struct && m_Struct->m_fpTableFree)
	{
		m_Struct->m_fpTableFree(this);
	}

	m_CodeBufferBase.Free();
	m_CodeBuffer = &m_CodeBufferBase;

	m_DStrings.Free();

	m_Data.Destroy(NULL, EXCEL_DATA_FREE, (void *)this);
	m_DataToFree.Destroy(NULL, EXCEL_TABLE_BUFFER_FREE);

	if (m_DataEx)
	{
		FREE(NULL, m_DataEx);
	}

	if (m_IsATable)
	{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		FREE(g_StaticAllocator, m_IsATable);
#else	
		FREE(NULL, m_IsATable);
#endif
	}

	if (m_Key)
	{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		FREE(g_StaticAllocator, m_Key);
#else	
		FREE(NULL, m_Key);
#endif
	}

	for (unsigned int ii = 0; ii < EXCEL_STRUCT::EXCEL_MAX_INDEXES; ++ii)
	{
		m_Indexes[ii].Free();
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::AddFile(
	const char * name,
	APP_GAME appGame,
	EXCEL_TABLE_SCOPE scope,
	unsigned int data1)
{
	ASSERT_RETFALSE(name);

	EXCEL_TABLE_FILE file;
	structclear(file);
	file.m_Name = name;
	file.m_AppGame = appGame;
	file.m_Scope = scope;
	ConstructFilename(file);
	file.m_TextHeaders.Init();
	file.m_bLoad = TRUE;
	file.m_Data1 = data1;

	if (m_Files.Count() > 0)
	{
		m_Files[0].m_bLoad = FALSE;
	}

	m_Files.InsertEnd(MEMORY_FUNC_FILELINE(NULL, file));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE * EXCEL_TABLE::GetCodeBufferSpace(
	unsigned int & len)
{
	ASSERT_RETNULL(m_CodeBuffer);
	if (m_CodeBuffer->GetPos() == 0)
	{
		m_CodeBuffer->AllocateSpace(EXCEL_CODE_BUFSIZE);
		m_CodeBuffer->Advance(1);
	}
	return m_CodeBuffer->GetWriteBuffer(len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PCODE EXCEL_TABLE::SetCodeBuffer(
	const BYTE * end)
{
	ASSERT_RETNULL(m_CodeBuffer);

	unsigned int len;
	BYTE * buf = m_CodeBuffer->GetWriteBuffer(len);
	ASSERT_RETVAL(end > buf, NULL_CODE);
	unsigned int codelen = (unsigned int)(end - buf);
	ASSERT_RETVAL(codelen <= len, NULL_CODE);
	unsigned int pos = m_CodeBuffer->GetPos();
	ASSERT_RETVAL(pos > 0, NULL_CODE);
	m_CodeBuffer->Advance(codelen);
	return (PCODE)pos;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_TABLE::PushRow(
	unsigned int index)
{
	if (index >= m_Data.Count())
	{
		return;
	}
	if (m_Data[index].m_RowData == NULL)
	{
		return;
	}

	EXCEL_DATA & data = m_Data[index];

	data.m_Prev = (EXCEL_DATA *)MALLOCZ(NULL, sizeof(EXCEL_DATA));
	*data.m_Prev = m_Data[index];
	data.m_RowData = NULL;
	data.m_Stats = NULL;
	data.m_dwFlags = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelVersionInit(
	EXCEL_DATA_VERSION * version)
{
	ASSERT_RETURN(version);

	version->m_VersionApp = EXCEL_APP_VERSION_DEFAULT;
	version->m_VersionPackage = EXCEL_PACKAGE_VERSION_DEFAULT;
	version->m_VersionMajorMin = 0; 
	version->m_VersionMajorMax = EXCEL_DATA_VERSION::EXCEL_MAX_VERSION;  
	version->m_VersionMinorMin = 0; 
	version->m_VersionMinorMax = EXCEL_DATA_VERSION::EXCEL_MAX_VERSION;  
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::AddRow(
	unsigned int index)
{
	ASSERT_RETFALSE(index < EXCEL_DATA_MAXROWS);
	ASSERT_RETFALSE(m_Struct);
	ASSERT_RETFALSE(index < m_Struct->m_nMaxRows);

	unsigned int rowsize = ExcelGetRowDataSize(this);
	BYTE * data = (BYTE *)MALLOCZ(NULL, rowsize); 
	EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(data);
	sExcelVersionInit(version);

	EXCEL_DATA row(data, TRUE);

	TABLE_DATA_WRITE_LOCK(this);

	PushRow(index);
	m_Data.OverwriteIndex(MEMORY_FUNC_FILELINE(NULL, index, row));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::AddRowIfUndefined(
	unsigned int index)
{
	ASSERT_RETFALSE(index < EXCEL_DATA_MAXROWS);
	ASSERT_RETFALSE(m_Struct);
	ASSERT_RETFALSE(index < m_Struct->m_nMaxRows);

	TABLE_DATA_WRITE_LOCK(this);

	unsigned int rowsize = ExcelGetRowDataSize(this);

	if (m_Data.Count() > index)
	{
		if (m_Data[index].m_RowData == NULL)
		{
			m_Data[index].m_RowData = (BYTE *)MALLOCZ(NULL, rowsize); 
		}
		return TRUE;
	}

	BYTE * data = (BYTE *)MALLOCZ(NULL, rowsize); 
	EXCEL_DATA row(data, TRUE);

	m_Data.OverwriteIndex(MEMORY_FUNC_FILELINE(NULL, index, row));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct STATS * EXCEL_TABLE::GetStatsList(
	unsigned int row) const
{
	ASSERT_RETNULL(m_Struct && m_Struct->m_hasStatsList);

	TABLE_DATA_READ_LOCK(this);

	ASSERT_RETNULL(m_Data.Count() > row);

	return m_Data[row].m_Stats;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct STATS * EXCEL_TABLE::GetOrCreateStatsList(
	unsigned int row)
{
	ASSERT_RETNULL(m_Struct && m_Struct->m_hasStatsList);

	ASSERT_RETNULL(m_Data.Count() > row);

	if (m_Data[row].m_Stats)
	{
		return m_Data[row].m_Stats;
	}

	EXCEL_STATSLIST_INIT * fpStatsListInit = Excel().m_fpStatsListInit;
	ASSERT_RETNULL(fpStatsListInit);

	STATS * stats = fpStatsListInit(NULL);
	ASSERT_RETNULL(stats);
	
	m_Data[row].m_Stats = stats;

	return stats;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelConstructFilename(
	char * filename,
	char * filenameCooked,
	unsigned int len,
	const char * basename,
	EXCEL_TABLE_SCOPE scope)
{
	const char * directory = FILE_PATH_EXCEL;
	if (scope == EXCEL_TABLE_SHARED)
	{
		directory = FILE_PATH_EXCEL_COMMON;
	}
	PStrPrintf(filename, len, "%s%s.txt", directory, basename);
	PStrFixFilename(filename, len);

#if ISVERSION(DEVELOPMENT)
	if(!AppCommonIsAnyFillpak() && (gtAppCommon.m_dwExcelManifestPackage || gtAppCommon.m_wExcelVersionMajor || gtAppCommon.m_wExcelVersionMinor))
	{
		PStrPrintf(filenameCooked, len, "%s%s.%d.%d.%d.txt.%s", directory, basename, gtAppCommon.m_dwExcelManifestPackage, gtAppCommon.m_wExcelVersionMajor, gtAppCommon.m_wExcelVersionMinor, COOKED_FILE_EXTENSION);
		PStrFixFilename(filenameCooked, len);
	}
	else
#endif
	{
		PStrPrintf(filenameCooked, len, "%s%s.txt.%s", directory, basename, COOKED_FILE_EXTENSION);
		PStrFixFilename(filenameCooked, len);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_TABLE::ConstructFilename(
	EXCEL_TABLE_FILE & file)
{
	sExcelConstructFilename(file.m_Filename, file.m_FilenameCooked, arrsize(file.m_Filename), file.m_Name, file.m_Scope);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelLoadsFile(
	EXCEL_TABLE_FILE & file)
{
	if (!file.m_bLoad)
	{
		return FALSE;
	}
	if (file.m_AppGame == APP_GAME_ALL || file.m_AppGame == AppGameGet())
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::AppLoadsTable(
	void)
{
	for (unsigned int ii = 0; ii < m_Files.Count(); ++ii)
	{
		if (sExcelLoadsFile(m_Files[ii]))
		{
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::AddRowToIndexes(
	unsigned int row, 
	BYTE * rowdata)
{
	BYTE * data = ExcelGetDataFromRowData(rowdata);

	TABLE_INDEX_WRITE_LOCK(this);

	for (unsigned int ii = 0; ii < EXCEL_STRUCT::EXCEL_MAX_INDEXES; ++ii)
	{
		if (!m_Indexes[ii].m_IndexDesc)
		{
			continue;
		}
		
		if (m_Indexes[ii].m_IndexDesc->m_dwFlags & EXCEL_INDEX_DONT_ADD_BLANK)
		{
			if (m_Indexes[ii].IsBlank(this, row, data))
			{
				continue;
			}
		}

		unsigned int duperow;
		if (!m_Indexes[ii].AddIndexNode(row, rowdata, data, &duperow))
		{
			if (m_Indexes[ii].m_IndexDesc->m_dwFlags & EXCEL_INDEX_WARN_DUPLICATE)
			{
				;
			}
			else if (m_Indexes[ii].m_IndexDesc->m_dwFlags & EXCEL_INDEX_WARN_DUPLICATE_NONBLANK)
			{
				if (m_Indexes[ii].IsBlank(this, row, data))
				{
					continue;
				}
			}
			else
			{
				continue;
			}
			
			char buf[5];
			const char * keystr = NULL;

			switch (m_Indexes[ii].m_IndexDesc->m_Type)
			{
			case EXCEL_INDEXTYPE_STRING:
				keystr = (const char *)(data + m_Indexes[ii].m_IndexDesc->m_IndexFields[0]->m_nOffset);
				break;
			case EXCEL_INDEXTYPE_DYNAMIC_STRING:
				keystr = *(const char **)(data + m_Indexes[ii].m_IndexDesc->m_IndexFields[0]->m_nOffset);
				break;
			case EXCEL_INDEXTYPE_STRINT:
				{
					EXCEL_FIELD * field = (EXCEL_FIELD *)m_Indexes[ii].m_IndexDesc->m_IndexFields[0];
					ASSERT_BREAK(field && field->m_Type == EXCEL_FIELDTYPE_STRINT);
					EXCEL_LOCK(CWRAutoLockReader, &field->m_FieldData.csStrIntDict);

					int link = *(int *)(data + m_Indexes[ii].m_IndexDesc->m_IndexFields[0]->m_nOffset);
					if (link == EXCEL_LINK_INVALID)
					{
						break;
					}
					if (!field->m_FieldData.strintdict)
					{
						break;
					}
					keystr = field->m_FieldData.strintdict->GetStrByVal(link);
					break;
				}
			case EXCEL_INDEXTYPE_FOURCC:
				{
					FOURCCTOSTR(*(const DWORD *)(data + m_Indexes[ii].m_IndexDesc->m_IndexFields[0]->m_nOffset), buf);
					keystr = buf;
					break;
				}
			case EXCEL_INDEXTYPE_TWOCC:
				{
					TWOCCTOSTR(*(const WORD *)(data + m_Indexes[ii].m_IndexDesc->m_IndexFields[0]->m_nOffset), buf);
					keystr = buf;
					break;
				}
			case EXCEL_INDEXTYPE_ONECC:
				{
					ONECCTOSTR(*(const BYTE *)(data + m_Indexes[ii].m_IndexDesc->m_IndexFields[0]->m_nOffset), buf);
					keystr = buf;
					break;
				}
			default:
				break;
			}

			ExcelWarn("EXCEL WARNING:  DUPLICATE INDEX  TABLE: %s   INDEX: %d   ROW: %d, %d   %s '%s'", m_Name, ii, duperow, row, (keystr ? "KEY:" : ""), keystr);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// sort indexes
//----------------------------------------------------------------------------
void EXCEL_TABLE::PostIndexLoad(
	void)
{
	TABLE_INDEX_WRITE_LOCK(this);

	if (m_Struct->m_fpPostIndex)
	{
		m_Struct->m_fpPostIndex(this);
	}
}


//----------------------------------------------------------------------------
// read out a line until we get to EOL or EOF
//----------------------------------------------------------------------------
void EXCEL_TABLE::LoadDirectReadOutLine(
	char * & cur)
{
	ASSERT_RETURN(cur);
	while (*cur && *cur != '\n')
	{
		cur++;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE * EXCEL_TABLE::GetFieldData(
	unsigned int row,
	const EXCEL_FIELD * field)
{
	ASSERT_RETNULL(field);
	ASSERT_RETNULL(row < m_Data.Count());
	BYTE * data = (BYTE *)ExcelGetDataFromRowData(m_Data[row]);
	ASSERT_RETNULL(data);
	return data + field->m_nOffset;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::GetInheritToken(
	unsigned int row, 
	const EXCEL_FIELD * field,
	char * & token,
	unsigned int & toklen)
{
	static const unsigned int MAX_INHERIT_ITERATIONS = 10;

	if (!m_InheritField || field == m_InheritField)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(field);

	unsigned int oRow = row;
	REF(oRow);
	unsigned int iteration = 0;
	while (iteration++ < MAX_INHERIT_ITERATIONS)
	{		
		BYTE * fielddata = GetFieldData(row, m_InheritField);
		ASSERT_RETFALSE(fielddata);

		EXCEL_LINK link = (EXCEL_LINK)*(int *)fielddata;
		if (link == EXCEL_LINK_INVALID)
		{
			return FALSE;
		}
		ASSERTV_RETFALSE(
			row != link, 
			"Index '%d' in table '%s' attempting to inherit from its own base row",
			row,
			m_Name);
		row = (unsigned int)link;

		ASSERT_RETFALSE(row < m_TextFields.Count());
		EXCEL_TEXTFIELD_LINE & textFieldLine = m_TextFields[row];

		for (unsigned int ii = 0; ii < textFieldLine.m_Fields.Count(); ++ii)
		{
			EXCEL_TEXTFIELD & textField = textFieldLine.m_Fields[ii];
			if (textField.m_Field != field)
			{
				continue;
			}
			if (textField.m_Token && textField.m_Toklen > 0)
			{
				token = textField.m_Token;
				toklen = textField.m_Toklen;
				return TRUE;
			}
			break;
		}
	}
	ASSERTV_MSG("EXCEL WARNING:  MAXIMUM INHERIT ROW DEPTH EXCEEDED  TABLE: %s   ROW: %d\n", m_Name, oRow);
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelLoadTranslateVersion(
	const EXCEL_DATA_VERSION * version)
{
	if (!version)
	{
		return TRUE;
	}
	ASSERT(version->m_VersionApp != 0);

	const EXCEL_LOAD_MANIFEST & manifest = Excel().m_LoadManifest;
	if ((manifest.m_VersionApp & version->m_VersionApp) == 0)
	{
		return FALSE;
	}
	if ((manifest.m_VersionPackage & version->m_VersionPackage) == 0)
	{
		return FALSE;
	}
	if (manifest.m_VersionMajorMax < version->m_VersionMajorMin)
	{
		return FALSE;
	}
	if (manifest.m_VersionMajorMin > version->m_VersionMajorMax)
	{
		return FALSE;
	}
	if (manifest.m_VersionMinorMax < version->m_VersionMinorMin)
	{
		return FALSE;
	}
	if (manifest.m_VersionMinorMin > version->m_VersionMinorMax)
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelAddRowToIndexPrivate(
	EXCEL_TABLE * table,
	unsigned int row)
{
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(table->m_Struct);

#if EXCEL_FILE_LINE
	BYTE * data = (BYTE *)table->GetData(row, __FILE__, __LINE__);
#else
	BYTE * data = (BYTE *)table->GetData(row);
#endif
	ASSERT_RETFALSE(data);

	// post process
	if (table->m_Struct->m_fpPostProcessRow)
	{
		table->m_Struct->m_fpPostProcessRow(table, row, data);
	}

	// add the row to the index
	if (table->m_Struct->m_isIndexed && table->m_UpdateState < EXCEL_UPDATE_STATE_INDEX_BUILT)
	{
		EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(table->m_Data[row]);
		if (sExcelLoadTranslateVersion(version))
		{
			table->AddRowToIndexes(row, table->m_Data[row].m_RowData);
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// read a text line and translate into a data line
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadDirectTranslateDataLine(
	unsigned int row)
{
	ASSERT_RETFALSE(row < EXCEL_DATA_MAXROWS);
	if (!m_bRowsAdded)
	{
		ASSERT_RETFALSE(AddRow(row));
	}

#if EXCEL_FILE_LINE
	BYTE * data = (BYTE *)GetData(row, __FILE__, __LINE__);
#else
	BYTE * data = (BYTE *)GetData(row);
#endif
	ASSERT_RETFALSE(data);

	EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(m_Data[row]);
	BOOL bVersionApp = sExcelLoadTranslateVersion(version);

	BOOL bUsesInheritRow = m_Struct->m_usesInheritRow;

	EXCEL_TEXTFIELD_LINE & textFieldLine = m_TextFields[row];
	m_CurFile = textFieldLine.m_CurFile;
	for (unsigned int ii = 0; ii < textFieldLine.m_Fields.Count(); ++ii)
	{
		EXCEL_TEXTFIELD & textField = textFieldLine.m_Fields[ii];
		ASSERT_RETFALSE(textField.m_Field);

		const EXCEL_FIELD * field = textField.m_Field;
		if(field->m_dwFlags & EXCEL_FIELD_DO_NOT_TRANSLATE)
		{
			continue;
		}
		if (field->IsLoadFirst() && m_UpdateState >= EXCEL_UPDATE_STATE_INDEX_BUILT)
		{
			continue;
		}

		char * token = textField.m_Token;
		unsigned int toklen = textField.m_Toklen;

		if (toklen == 0 && bUsesInheritRow)
		{
			GetInheritToken(row, textField.m_Field, token, toklen);
		}

		EXCEL_TOKENIZE<char> tokenizer(token, toklen);
		if (bVersionApp)
		{
			field->LoadDirectTranslate(this, row, data, token, toklen);
		}
		else
		{
			field->SetDefaultData(this, row, data);
		}
	}

	// post process
	if (m_Struct->m_fpPostProcessRow)
	{
		m_Struct->m_fpPostProcessRow(this, row, data);
	}

	// add the row to the index
	if (m_Struct->m_isIndexed && m_UpdateState < EXCEL_UPDATE_STATE_INDEX_BUILT)
	{
		if (bVersionApp)
		{
			AddRowToIndexes(row, m_Data[row].m_RowData);
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadDirectTranslateInheritRow(
	void)
{
	if (!m_InheritField)
	{
		return TRUE;
	}

	for (unsigned int row = 0; row < m_TextFields.Count(); ++row)
	{
		EXCEL_TEXTFIELD_LINE & textFieldLine = m_TextFields[row];
		m_CurFile = textFieldLine.m_CurFile;
		unsigned int idField = EXCEL_LINK_INVALID;

		for (unsigned int ii = 0; ii < textFieldLine.m_Fields.Count(); ++ii)
		{
			EXCEL_TEXTFIELD & textField = textFieldLine.m_Fields[ii];
			if (!textField.m_Field || textField.m_Field->m_Type != EXCEL_FIELDTYPE_INHERIT_ROW)
			{
				continue;
			}
			idField = ii;
			break;
		}

		BYTE * fielddata = GetFieldData(row, m_InheritField);
		ASSERT_RETFALSE(fielddata);

		EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(m_Data[row]);
		if (!sExcelLoadTranslateVersion(version))
		{
			*(int *)fielddata = (int)EXCEL_LINK_INVALID;
			continue;
		}

		ASSERT_DO(idField < textFieldLine.m_Fields.Count())
		{
			*(int *)fielddata = (int)EXCEL_LINK_INVALID;
			continue;
		}

		EXCEL_TEXTFIELD & textField = textFieldLine.m_Fields[idField];

		if (textField.m_Toklen == 0)
		{
			*(int *)fielddata = (int)EXCEL_LINK_INVALID;
			continue;
		}

		{
			EXCEL_TOKENIZE<char> tokenizer(textField.m_Token, textField.m_Toklen);
			EXCEL_LINK link = GetLineByStrKey(textField.m_Token);
			if (link == EXCEL_LINK_INVALID)
			{
				ExcelWarn("EXCEL WARNING:  INHERIT ROW NOT FOUND  TABLE: %s   ROW: %d   COLUMN: %s   KEY: '%s'", m_Name, row, m_InheritField->m_ColumnName, textField.m_Token);
				*(int *)fielddata = (int)EXCEL_LINK_INVALID;
				continue;
			}
			*(int *)fielddata = (int)link;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadDirectTranslateData(
	void)
{
	if (m_Struct->m_usesInheritRow)
	{
		ASSERT_RETFALSE(LoadDirectTranslateInheritRow());
	}

	for (unsigned int row = 0; row < m_TextFields.Count(); ++row)
	{
		if (!LoadDirectTranslateDataLine(row))
		{
			break;
		}
	}

	if (m_Struct->m_isIndexed && m_UpdateState < EXCEL_UPDATE_STATE_INDEX_BUILT)
	{
		m_UpdateState = EXCEL_UPDATE_STATE_INDEX_BUILT;
		PostIndexLoad();
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadDirectIndexLine(
	unsigned int row)
{
	ASSERT_RETFALSE(AddRow(row));

#if EXCEL_FILE_LINE
	BYTE * rowdata = (BYTE *)GetData(row, __FILE__, __LINE__);
#else
	BYTE * rowdata = (BYTE *)GetData(row);
#endif
	ASSERT_RETFALSE(rowdata);

	EXCEL_TEXTFIELD_LINE & textFieldLine = m_TextFields[row];
	m_CurFile = textFieldLine.m_CurFile;

	for (unsigned int ii = 0; ii < textFieldLine.m_Fields.Count(); ++ii)
	{
		EXCEL_TEXTFIELD & textField = textFieldLine.m_Fields[ii];

		if (textField.m_Field && (textField.m_Field->m_dwFlags & EXCEL_FIELD_VERSION_ROW) != 0)
		{
			EXCEL_TOKENIZE<char> tokenizer(textField.m_Token, textField.m_Toklen);
			textField.m_Field->LoadDirectTranslate(this, row, rowdata, textField.m_Token, textField.m_Toklen);
		}
	}

	for (unsigned int ii = 0; ii < textFieldLine.m_Fields.Count(); ++ii)
	{
		EXCEL_TEXTFIELD & textField = textFieldLine.m_Fields[ii];

		if (textField.m_Field && (textField.m_Field->m_dwFlags & EXCEL_FIELD_ISINDEX) != 0)
		{
			ASSERT((textField.m_Field->m_dwFlags & EXCEL_FIELD_VERSION_ROW) == 0);

			EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(m_Data[row]);
			if (sExcelLoadTranslateVersion(version))
			{
				EXCEL_TOKENIZE<char> tokenizer(textField.m_Token, textField.m_Toklen);
				textField.m_Field->LoadDirectTranslate(this, row, rowdata, textField.m_Token, textField.m_Toklen);
			}
		}
	}

	EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(m_Data[row]);
	if (sExcelLoadTranslateVersion(version))
	{
		AddRowToIndexes(row, m_Data[row].m_RowData);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// just parse and load indexes
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadCookedIndexes(
	void)
{
	BOOL result = LoadFromPak();
	if (result)
	{
		m_UpdateState = EXCEL_UPDATE_STATE_UPTODATE_INDEX_BUILT;
	}
	return result;
}


//----------------------------------------------------------------------------
// just parse and load indexes
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadDirectIndexes(
	void)
{
	ASSERT_RETFALSE(LoadDirectText());

	for (unsigned int row = 0; row < m_TextFields.Count(); ++row)
	{
		if (!LoadDirectIndexLine(row))
		{
			break;
		}
	}

	PostIndexLoad();

	m_UpdateState = EXCEL_UPDATE_STATE_INDEX_BUILT;
	m_bRowsAdded = TRUE;
	return TRUE;
}


//----------------------------------------------------------------------------
// just parse and load indexes
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadIndexes(
	void)
{
	if (!m_Struct->m_isIndexed && !m_bHasVersion)
	{
		return TRUE;
	}

	switch (m_UpdateState)
	{
	case EXCEL_UPDATE_STATE_INDEX_BUILT:
	case EXCEL_UPDATE_STATE_UPDATED:
	case EXCEL_UPDATE_STATE_UPTODATE_INDEX_BUILT:
	case EXCEL_UPDATE_STATE_UPTODATE_LOADED:
		return TRUE;
	}

	if (m_bNoLoad)
	{
		return TRUE;
	}

	// go through all my index fields and make sure that their indexes are loaded
	for (unsigned int ii = 0; ii < m_Struct->m_Fields.Count(); ++ii)
	{
		const EXCEL_FIELD & field = m_Struct->m_Fields[ii];
		if ((field.m_dwFlags & EXCEL_FIELD_ISINDEX) && (field.m_dwFlags & EXCEL_FIELD_ISLINK) &&
			field.m_FieldData.idTable != m_Index)
		{
			EXCEL_TABLE * table = Excel().GetTable(field.m_FieldData.idTable);
			ASSERT_CONTINUE(table);
			if (table->AppLoadsTable())
			{
				table->LoadIndexes();
			}
		}
	}

	if (m_UpdateState == EXCEL_UPDATE_STATE_UPTODATE)
	{
		return LoadCookedIndexes();
	}
	else
	{
		return LoadDirectIndexes();
	}
}


//----------------------------------------------------------------------------
// call back function for destroying temp data used during parsing
//----------------------------------------------------------------------------
void EXCEL_TEXTFIELD_DESTROY(
	MEMORYPOOL * pool,
	EXCEL_TEXTFIELD & line)
{
	if (line.m_Field && TESTBIT(line.m_dwFieldFlags, EXCEL_HEADER_FREE))
	{
		EXCEL_DYNAMIC_FIELD_DESTROY(pool, (EXCEL_FIELD *)line.m_Field);
	}
}


//----------------------------------------------------------------------------
// call back function for destroying temp data used during parsing
//----------------------------------------------------------------------------
void EXCEL_TEXTFIELD_LINE_DESTROY(
	MEMORYPOOL * pool,
	EXCEL_TEXTFIELD_LINE & line)
{
	line.m_Fields.Destroy(pool, EXCEL_TEXTFIELD_DESTROY);
}


//----------------------------------------------------------------------------
// free temporary data constructed for parsing
//----------------------------------------------------------------------------
void EXCEL_TABLE::FreeParseData(
	void)
{
	// free up anything
	m_TextFields.Destroy(NULL, EXCEL_TEXTFIELD_LINE_DESTROY);
	for (unsigned int ii = 0; ii < m_Files.Count(); ++ii)
	{
		EXCEL_TABLE_FILE_DESTROY(NULL, m_Files[ii]);
	}
}


//----------------------------------------------------------------------------
// parse tokens into text fields
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadDirectBuildTextFields(
	EXCEL_TABLE_FILE & file)
{
	ASSERT_RETFALSE(m_Struct);
	ASSERT_RETFALSE(file.m_TextDataStart);

	char * cur = file.m_TextDataStart;
	while (*cur)
	{
		EXCEL_TEXTFIELD_LINE newTextFieldLine;
		newTextFieldLine.m_CurFile = m_CurFile;
		unsigned int row = m_TextFields.Count();
		m_TextFields.InsertEnd(MEMORY_FUNC_FILELINE(NULL, newTextFieldLine));
		ASSERT_RETFALSE(row < m_TextFields.Count());

		BOOL s_UsedFields[EXCEL_STRUCT::EXCEL_MAX_FIELDS];
		memclear(s_UsedFields, sizeof(s_UsedFields));

		char * token = NULL;
		unsigned int toklen = 0;
		unsigned int column = 0;

		while (ExcelGetToken(cur, token, toklen) == EXCEL_TOK_TOKEN)
		{
			if (column >= file.m_TextHeaders.Count())
			{
				LoadDirectReadOutLine(cur);
				continue;
			}
			if (file.m_TextHeaders[column].m_Field)
			{
				DWORD flags = file.m_TextHeaders[column].m_dwFieldFlags;
				if (TESTBIT(flags, EXCEL_HEADER_FREE))
				{
					CLEARBIT(file.m_TextHeaders[column].m_dwFieldFlags, EXCEL_HEADER_FREE);
				}
				EXCEL_TEXTFIELD textfield(token, toklen, file.m_TextHeaders[column].m_Field, flags);
				m_TextFields[row].m_Fields.InsertEnd(MEMORY_FUNC_FILELINE(NULL, textfield));

				if (!TESTBIT(flags, EXCEL_HEADER_DYNAMIC))
				{
					unsigned int fieldid = (unsigned int)(file.m_TextHeaders[column].m_Field - m_Struct->m_Fields.GetPointer());
					ASSERT_RETFALSE(fieldid < m_Struct->m_Fields.Count());
                    ASSERT_RETFALSE(fieldid < ARRAYSIZE(s_UsedFields));
					s_UsedFields[fieldid] = TRUE;
				}
			}

			++column;
		}

		// insert unused fields
		for (unsigned int ii = 0; ii < m_Struct->m_Fields.Count(); ++ii)
		{
			if (!s_UsedFields[ii])
			{
				EXCEL_TEXTFIELD textfield(EXCEL_BLANK, 0, &m_Struct->m_Fields[ii], 0);
				m_TextFields[row].m_Fields.InsertEnd(MEMORY_FUNC_FILELINE(NULL, textfield));				
			}
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_FIELD * EXCEL_TABLE::LoadDirectTranslateHeaderCreateDynamicField(
	EXCEL_HEADER & header,
	EXCEL_FIELDTYPE type, 
	const char * token,
	unsigned int toklen)
{
	ASSERT_RETNULL(token);
	char * colname = (char *)MALLOC(NULL, toklen + 1);
	PStrCopy(colname, token, toklen + 1);

	EXCEL_FIELD * field = (EXCEL_FIELD *)MALLOCZ(NULL, sizeof(EXCEL_FIELD));
	field->m_ColumnName = (const char *)colname;
	field->m_Type = type;
	field->m_nCount = 1;
	field->m_nDefinedOrder = UINT_MAX;

	header.m_Field = field;
	SETBIT(header.m_dwFieldFlags, EXCEL_HEADER_DYNAMIC);
	SETBIT(header.m_dwFieldFlags, EXCEL_HEADER_FREE);

	return field;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadDirectTranslateHeaderDynamicStats(
	EXCEL_HEADER & header,
	const char * token,
	unsigned int toklen)
{
	if (!m_Struct->m_hasStatsList)
	{
		return FALSE;
	}

	EXCEL_STATS_PARSE_TOKEN * fpStatsParseToken = Excel().m_fpStatsParseToken;
	ASSERT_RETFALSE(fpStatsParseToken);

	int wStat;
	DWORD dwParam;
	if (!fpStatsParseToken(token, wStat, dwParam))
	{
		return FALSE;
	}

	EXCEL_FIELD * field = LoadDirectTranslateHeaderCreateDynamicField(header, EXCEL_FIELDTYPE_DEFSTAT, token, toklen);
	field->SetStatData(wStat, dwParam);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadDirectTranslateHeaderVersion(
	EXCEL_HEADER & header,
	const char * token,
	unsigned int toklen)
{
	if (PStrICmp(token, "VERSION_APP") == 0)
	{
		EXCEL_FIELD * field = LoadDirectTranslateHeaderCreateDynamicField(header, EXCEL_FIELDTYPE_VERSION_APP, token, toklen);
		field->m_dwFlags |= EXCEL_FIELD_VERSION_ROW;
		return TRUE;
	}
	if (PStrICmp(token, "VERSION_PACKAGE") == 0)
	{
		EXCEL_FIELD * field = LoadDirectTranslateHeaderCreateDynamicField(header, EXCEL_FIELDTYPE_VERSION_PACKAGE, token, toklen);
		field->m_dwFlags |= EXCEL_FIELD_VERSION_ROW;
		return TRUE;
	}
	if (PStrICmp(token, "VERSION_MAJOR") == 0)
	{
		EXCEL_FIELD * field = LoadDirectTranslateHeaderCreateDynamicField(header, EXCEL_FIELDTYPE_VERSION_MAJOR, token, toklen);
		field->m_dwFlags |= EXCEL_FIELD_VERSION_ROW;
		return TRUE;
	}
	if (PStrICmp(token, "VERSION_MINOR") == 0)
	{
		EXCEL_FIELD * field = LoadDirectTranslateHeaderCreateDynamicField(header, EXCEL_FIELDTYPE_VERSION_MINOR, token, toklen);
		field->m_dwFlags |= EXCEL_FIELD_VERSION_ROW;
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadDirectTranslateHeader(
	EXCEL_TABLE_FILE & file)
{
	if (!file.m_TextBuffer)
	{
		return TRUE;
	}
	if (file.m_TextBufferSize <= 0)
	{
		return TRUE;
	}

	BOOL bFieldUsed[EXCEL_STRUCT::EXCEL_MAX_FIELDS];
	structclear(bFieldUsed);

	// parse the first line of the text buffer and generate m_TextHeaders
	char * cur = file.m_TextBuffer;
	char * token = NULL;
	unsigned int toklen = 0;
	while (ExcelGetToken(cur, token, toklen) == EXCEL_TOK_TOKEN)
	{
		EXCEL_HEADER header;

		if (toklen > 0)
		{
			EXCEL_TOKENIZE<char> tokenizer(token, toklen);
			header.m_Field = m_Struct->GetFieldByNameConst(token, toklen);
			if (header.m_Field)
			{
				bFieldUsed[header.m_Field - m_Struct->m_Fields.GetPointer()] = TRUE;
				if (header.m_Field->m_Type == EXCEL_FIELDTYPE_INHERIT_ROW)
				{
					m_InheritField = header.m_Field;
				}
			}
			else if (LoadDirectTranslateHeaderVersion(header, token, toklen))
			{
				m_bHasVersion = TRUE;
			}
			else if (LoadDirectTranslateHeaderDynamicStats(header, token, toklen))
			{
				;
			}
		}
		file.m_TextHeaders.InsertEnd(MEMORY_FUNC_FILELINE(NULL, header));
	}

	file.m_TextDataStart = cur;
	if (!*cur)
	{
		return TRUE;
	}

	// read sub header
	if (PStrICmp(cur, "/*") != 0)
	{
		return TRUE;
	}

	for (unsigned int column = 1; ExcelGetToken(cur, token, toklen) == EXCEL_TOK_TOKEN; ++column)
	{
		EXCEL_TOKENIZE<char> tokenizer(token, toklen);
		ASSERT_CONTINUE(column < file.m_TextHeaders.Count());
		if (!file.m_TextHeaders[column].m_Field)
		{
			continue;
		}
		switch (file.m_TextHeaders[column].m_Field->m_Type)
		{
		case EXCEL_FIELDTYPE_DICT_CHAR:
		case EXCEL_FIELDTYPE_DICT_SHORT:
		case EXCEL_FIELDTYPE_DICT_INT:
		case EXCEL_FIELDTYPE_DICT_INT_ARRAY:
/*
			file.m_TextHeaders[column].m_Field->InitDictField(token);
*/
			ASSERT(0);
			break;
		}
	}

	file.m_TextDataStart = cur;
	return TRUE;
}


//----------------------------------------------------------------------------
// remove trailing blank lines from m_TextBuffer
//----------------------------------------------------------------------------
void EXCEL_TABLE::RemoveTrailingBlankLines(
	EXCEL_TABLE_FILE & file)
{
	if (file.m_TextBufferSize <= 0)
	{
		return;
	}

	char * cur = file.m_TextBuffer + file.m_TextBufferSize - 1;
	char * lastend = cur;
	while (cur > file.m_TextBuffer)
	{
		switch (*cur)
		{
		case '\n':
		case '\r':
			lastend = cur;
			break;
		case ' ':
		case '\t':
			break;
		default:
			goto _done;
		}
		--cur;
	}
_done:
	*lastend = 0;
	file.m_TextBufferSize = (unsigned int)(cur - file.m_TextBuffer);
}


//----------------------------------------------------------------------------
// load a txt file from disk and parse the header
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadDirectText(
	void)
{
	if (m_UpdateState == EXCEL_UPDATE_STATE_ERROR)
	{
		return FALSE;
	}
	// read text file
	if (m_UpdateState >= EXCEL_UPDATE_STATE_TEXT_READ)
	{
		return TRUE;
	}

	FreeParseData();

	for (m_CurFile = 0; m_CurFile < m_Files.Count(); ++m_CurFile)
	{
		EXCEL_TABLE_FILE & file = m_Files[m_CurFile];

		if (!sExcelLoadsFile(file))
		{
			continue;
		}
		ASSERT_RETFALSE(file.m_Filename[0]);
		file.m_TextBuffer = (char *)FileLoad(MEMORY_FUNC_FILELINE(NULL, file.m_Filename, &file.m_TextBufferSize));
		if (!file.m_TextBuffer || file.m_TextBufferSize <= 0)
		{
			ExcelWarn("EXCEL WARNING:  FAILED TO OPEN FILE: '%s'",	file.m_Filename);
			continue;
		}
		RemoveTrailingBlankLines(file);
		if (!file.m_TextBuffer || file.m_TextBufferSize <= 0)
		{
			continue;
		}
		ASSERT_GOTO(LoadDirectTranslateHeader(file), _error);
		ASSERT_GOTO(file.m_TextDataStart, _error);

		ASSERT_GOTO(LoadDirectBuildTextFields(file), _error);

		// no longer need text headers
		file.m_TextHeaders.Destroy(NULL, EXCEL_HEADER_DESTROY);
	}

	m_UpdateState = EXCEL_UPDATE_STATE_TEXT_READ;
	return TRUE;

_error:
	m_UpdateState = EXCEL_UPDATE_STATE_ERROR;
	FreeParseData();
	return FALSE;
}


//----------------------------------------------------------------------------
	// insert symbol table marker
//----------------------------------------------------------------------------
static void sExcelTableLoadDirectScriptInsertMaker(
	EXCEL_TABLE * table)
{
	if (!table->m_Struct->m_hasScriptField)
	{
		return;
	}
	EXCEL_SCRIPT_ADD_MARKER * fpScriptAddMarker = Excel().m_fpScriptAddMarker;
	ASSERT_RETURN(fpScriptAddMarker);
	fpScriptAddMarker(table->m_Index);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadDirect(
	void)
{
#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEBUG_VERSION)
	if (PStrCmp(this->m_Struct->m_Name,"NIL_DEFINITION") == 0)
		return TRUE;
	TraceError("ERROR: Excel data system trying to load table \"%s\" directly from disk.", this->m_Name);
	RaiseException(0xE0000000, 0, 0, NULL);	//	0xE marks user defined error, all 0s because this is the only place i know of we raise exceptions
#endif

	ExcelLog("EXCEL_TABLE::LoadDirect()   TABLE: %s", m_Name); 

	ASSERT_RETFALSE(this);
	ASSERT_RETFALSE(m_Struct);

	sExcelTableLoadDirectScriptInsertMaker(this);

	// make sure all indexes are read in
	for (unsigned int ii = 0; ii < m_Struct->m_Ancestors.Count(); ++ii)
	{
		unsigned int idTable = m_Struct->m_Ancestors[ii];
		EXCEL_TABLE * table = Excel().GetTable(idTable);
		ASSERT_CONTINUE(table);
		if (table->AppLoadsTable())
		{
			table->LoadIndexes();
		}
	}
	ASSERT_RETFALSE(m_UpdateState < EXCEL_UPDATE_STATE_UPTODATE);
	if ((m_bSelfIndex || m_bHasVersion) && m_UpdateState < EXCEL_UPDATE_STATE_INDEX_BUILT)
	{
		LoadIndexes();
	}

	ASSERT_RETFALSE(LoadDirectText());
	if(m_UpdateState == EXCEL_UPDATE_STATE_TEXT_READ && m_bHasVersion)
	{
		LoadIndexes();
	}

	BOOL retval = FALSE;

	// read lines
	ASSERT_GOTO(LoadDirectTranslateData(), _done);

	if (m_Struct->m_IsAField)
	{
		CreateIsATable();
	}

	// save cooked file & add to pak
	SaveCooked();

	retval = TRUE;

_done:
	FreeParseData();
	return retval;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableSaveCookedIsATable(
	EXCEL_TABLE * table,
	WRITE_BUF_DYNAMIC & fbuf)
{
	ASSERT_RETFALSE(fbuf.PushInt((DWORD)EXCEL_TABLE_MAGIC));

	ASSERT_RETFALSE(fbuf.PushInt(table->m_IsARowSize));
	ASSERT_RETFALSE(fbuf.PushInt(table->m_IsARowCount));
	if (table->m_IsARowCount)
	{
		ASSERT_RETFALSE(fbuf.PushBuf(table->m_IsATable, table->m_IsARowCount * table->m_IsARowSize * sizeof(DWORD)));
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableSaveCookedCodeBuffer(
	EXCEL_TABLE * table,
	WRITE_BUF_DYNAMIC & fbuf)
{
	ASSERT_RETFALSE(fbuf.PushInt((DWORD)EXCEL_TABLE_MAGIC));

	ASSERT_RETFALSE(table->m_CodeBuffer);
	ASSERT_RETFALSE(fbuf.PushInt(table->m_CodeBuffer->cur));
	ASSERT_RETFALSE(fbuf.PushBuf(table->m_CodeBuffer->buf, table->m_CodeBuffer->cur));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableSaveCookedScriptSymbols(
	EXCEL_TABLE * table,
	WRITE_BUF_DYNAMIC & fbuf)
{
	if (!table->m_Struct->m_hasScriptField)
	{
		return TRUE;
	}
	ASSERT_RETFALSE(fbuf.PushInt((DWORD)EXCEL_TABLE_MAGIC));

	EXCEL_SCRIPT_WRITE_TO_PAK * fpScriptWriteToPak = Excel().m_fpScriptWriteToPak;
	ASSERT_RETFALSE(fpScriptWriteToPak);

	return fpScriptWriteToPak(table->m_Index, fbuf);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableSaveCookedIndexes(
	EXCEL_TABLE * table,
	WRITE_BUF_DYNAMIC & fbuf)
{
	for (int ii = 0; ii < EXCEL_STRUCT::EXCEL_MAX_INDEXES; ii++)
	{
		EXCEL_INDEX & index = table->m_Indexes[ii];

		ASSERT_RETFALSE(fbuf.PushInt((DWORD)EXCEL_TABLE_MAGIC));

		ASSERT_RETFALSE(fbuf.PushInt(index.m_IndexNodes.Count()));

		for (unsigned int jj = 0; jj < index.m_IndexNodes.Count(); jj++)
		{
			EXCEL_INDEX_NODE & index_node = index.m_IndexNodes[jj];

			ASSERT_RETFALSE(fbuf.PushInt(index_node.m_RowIndex));
		}		
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableSaveCookedStrDicts(
	EXCEL_TABLE * table,
	WRITE_BUF_DYNAMIC & fbuf)
{
	for (unsigned int ii = 0; ii < table->m_Struct->m_Fields.Count(); ii++)
	{
		const EXCEL_FIELD & field = table->m_Struct->m_Fields[ii];

		if (field.m_Type == EXCEL_FIELDTYPE_STRINT)
		{
			STRINT_DICT * dict = field.m_FieldData.strintdict;
			if (!dict)
			{
				fbuf.PushInt(0);
			}
			else
			{
				dict->SaveToBuffer(fbuf);
			}
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableSaveCookedDefaultStats(
	EXCEL_TABLE * table,
	WRITE_BUF_DYNAMIC & fbuf,
	STATS * stats)
{
	if (!table->m_Struct->m_hasStatsList)
	{
		return TRUE;
	}

	EXCEL_STATS_WRITE_STATS * fpWriteStats = Excel().m_fpStatsWriteStats;
	ASSERT_RETFALSE(fpWriteStats);

	unsigned int buflen = 0;
	if (!stats)
	{
		ASSERT_RETFALSE(fbuf.PushInt(buflen));
		return TRUE;
	}
			
	BYTE buffer[EXCEL_MAX_STATS_BUFSIZE];
	ZeroMemory(buffer, sizeof(buffer));
	BIT_BUF buf(buffer, arrsize(buffer));
	ASSERT_RETFALSE(fpWriteStats(NULL, stats, buf, STATS_WRITE_METHOD_PAK));

	buflen = buf.GetLen();
	ASSERT_RETFALSE(fbuf.PushInt(buflen));
	ASSERT_RETFALSE(fbuf.PushBuf(buffer, buflen));

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableSaveCookedDataDynamicStringsFixup(
	EXCEL_TABLE * table,
	BYTE * src,
	BYTE * dest)
{
	for (unsigned int ii = 0; ii < table->m_Struct->m_Fields.Count(); ++ii)
	{
		const EXCEL_FIELD & field = table->m_Struct->m_Fields[ii];
		if (field.m_Type != EXCEL_FIELDTYPE_DYNAMIC_STRING)
		{
			continue;
		}
		BYTE * src_fielddata = src + field.m_nOffset;
		unsigned int link = table->m_DStrings.GetStringOffset(*(const char **)src_fielddata);

		ASSERT(field.m_FieldData.offset64 >= field.m_nOffset);
		BYTE * dest_fielddata = dest + field.m_FieldData.offset64;
		ZeroMemory(dest_fielddata, 8);
		*(int *)dest_fielddata = link;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableSaveCookedData(
	EXCEL_TABLE * table,
	WRITE_BUF_DYNAMIC & fbuf)
{
	ASSERT_RETFALSE(fbuf.PushInt((DWORD)EXCEL_TABLE_MAGIC));
	ASSERT_RETFALSE(fbuf.PushInt(table->m_Data.Count()));
	if (table->m_Data.Count() == 0)
	{
		return TRUE;
	}

	for (unsigned int ii = 0; ii < table->m_Data.Count(); ++ii)
	{
		EXCEL_DATA & data = table->m_Data[ii];
		ASSERT_RETFALSE(data.m_RowData);

		EXCEL_DATA_VERSION * version = ExcelGetVersionFromRowData(data.m_RowData);
		ASSERT_RETFALSE(version);
		ASSERT_RETFALSE(fbuf.PushBuf(version, sizeof(EXCEL_DATA_VERSION)));
		BYTE * rowdata = ExcelGetDataFromRowData(data);
		ASSERT_RETFALSE(rowdata);
		ASSERT_RETFALSE(MemoryCopy(table->m_Key, table->m_Struct->m_Size, rowdata, table->m_Struct->m_Size));
		ASSERT_RETFALSE(sExcelTableSaveCookedDataDynamicStringsFixup(table, rowdata, table->m_Key));

		ASSERT_RETFALSE(fbuf.PushBuf(table->m_Key, table->m_Struct->m_Size));
	}

	ASSERT_RETFALSE(fbuf.PushInt((DWORD)EXCEL_TABLE_MAGIC));
	for (unsigned int ii = 0; ii < table->m_Data.Count(); ++ii)
	{
		EXCEL_DATA & data = table->m_Data[ii];
		ASSERT_RETFALSE(fbuf.PushInt(ii));
		ASSERT_RETFALSE(sExcelTableSaveCookedDefaultStats(table, fbuf, data.m_Stats));
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// convert dynamic string fields to int offsets from string table
//----------------------------------------------------------------------------
static BOOL sExcelTableSaveCookedDynamicStrings(
	EXCEL_TABLE * table,
	WRITE_BUF_DYNAMIC & fbuf)
{
	ASSERT_RETFALSE(fbuf.PushInt((DWORD)EXCEL_TABLE_MAGIC));
	ASSERT_RETFALSE(table->m_DStrings.WriteToBuffer(fbuf));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableSaveManifest(
	EXCEL_TABLE * table,
	WRITE_BUF_DYNAMIC & fbuf)
{
	REF(table);

	EXCEL_LOAD_MANIFEST & load_manifest = Excel().m_LoadManifest;
	// save the load manifest used to generate the cooked data
	ASSERT_RETFALSE(fbuf.PushInt((DWORD)load_manifest.m_VersionApp));
	ASSERT_RETFALSE(fbuf.PushInt((DWORD)load_manifest.m_VersionPackage));
	ASSERT_RETFALSE(fbuf.PushInt((WORD)load_manifest.m_VersionMajorMin));
	ASSERT_RETFALSE(fbuf.PushInt((WORD)load_manifest.m_VersionMajorMax));
	ASSERT_RETFALSE(fbuf.PushInt((WORD)load_manifest.m_VersionMinorMin));
	ASSERT_RETFALSE(fbuf.PushInt((WORD)load_manifest.m_VersionMinorMax));
	// save the read manifest that should be used to read this data
	EXCEL_READ_MANIFEST & read_manifest = Excel().m_ReadManifest;
	ASSERT(load_manifest.m_VersionApp == read_manifest.m_VersionApp);
	ASSERT(load_manifest.m_VersionPackage == read_manifest.m_VersionPackage);
	ASSERT(read_manifest.m_VersionMajor >= load_manifest.m_VersionMajorMin && read_manifest.m_VersionMajor <= load_manifest.m_VersionMajorMax);
	ASSERT(read_manifest.m_VersionMinor >= load_manifest.m_VersionMinorMin && read_manifest.m_VersionMinor <= load_manifest.m_VersionMinorMax);
	ASSERT_RETFALSE(fbuf.PushInt((WORD)read_manifest.m_VersionMajor));
	ASSERT_RETFALSE(fbuf.PushInt((WORD)read_manifest.m_VersionMinor));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableSaveCookedHeader(
	EXCEL_TABLE * table,
	WRITE_BUF_DYNAMIC & fbuf)
{
	// write header
	FILE_HEADER hdr;
	hdr.dwMagicNumber = EXCEL_TABLE_MAGIC;
	hdr.nVersion = table->m_Struct->m_CRC32;
	ASSERT_RETFALSE(fbuf.PushInt(hdr.dwMagicNumber));
	ASSERT_RETFALSE(fbuf.PushInt(hdr.nVersion));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::SaveCooked(
	void)
{
	WRITE_BUF_DYNAMIC fbuf;

	ASSERT_RETFALSE(m_Struct);

	ASSERT_RETFALSE(sExcelTableSaveCookedHeader(this, fbuf));

	ASSERT_RETFALSE(sExcelTableSaveManifest(this, fbuf));

	ASSERT_RETFALSE(sExcelTableSaveCookedDynamicStrings(this, fbuf));

	ASSERT_RETFALSE(sExcelTableSaveCookedData(this, fbuf));

	ASSERT_RETFALSE(sExcelTableSaveCookedStrDicts(this, fbuf));

	ASSERT_RETFALSE(sExcelTableSaveCookedIndexes(this, fbuf));

	ASSERT_RETFALSE(sExcelTableSaveCookedScriptSymbols(this, fbuf));

	ASSERT_RETFALSE(sExcelTableSaveCookedCodeBuffer(this, fbuf));

	ASSERT_RETFALSE(sExcelTableSaveCookedIsATable(this, fbuf));

	// save to .cooked file
	ASSERT_RETFALSE(fbuf.Save(m_FilenameCooked, NULL, &m_GenTime));

	// add to pak
	if (AppCommonUsePakFile())
	{
		// history debugging
		FileDebugCreateAndCompareHistory(m_FilenameCooked);
		
		DECLARE_LOAD_SPEC(specCooked, m_FilenameCooked);
		specCooked.buffer = (void *)fbuf.GetBuf();
		specCooked.size = fbuf.GetPos();
		specCooked.gentime = m_GenTime;
		PakFileAddFile(specCooked);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableLoadCookedIsATable(
	EXCEL_TABLE * table,
	BYTE_BUF & fbuf)
{
	DWORD dw;
	ASSERT_RETFALSE(fbuf.PopInt(&dw));
	ASSERT_RETFALSE(dw == EXCEL_TABLE_MAGIC);

	ASSERT_RETFALSE(fbuf.PopInt(&table->m_IsARowSize));
	ASSERT_RETFALSE(fbuf.PopInt(&table->m_IsARowCount));
	if (table->m_IsARowCount)
	{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		table->m_IsATable = (DWORD *)MALLOC(g_StaticAllocator, table->m_IsARowCount * table->m_IsARowSize * sizeof(DWORD));
#else
		table->m_IsATable = (DWORD *)MALLOC(NULL, table->m_IsARowCount * table->m_IsARowSize * sizeof(DWORD));
#endif

		ASSERT_RETFALSE(fbuf.PopBuf(table->m_IsATable, table->m_IsARowCount * table->m_IsARowSize * sizeof(DWORD)));
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableLoadCookedCodeBuffer(
	EXCEL_TABLE * table,
	BYTE_BUF & fbuf)
{
	DWORD dw;
	ASSERT_RETFALSE(fbuf.PopInt(&dw));
	ASSERT_RETFALSE(dw == EXCEL_TABLE_MAGIC);

	unsigned int bufsize;
	ASSERT_RETFALSE(fbuf.PopInt(&bufsize));
	if (!bufsize)
	{
		return TRUE;
	}

	TABLE_DATA_WRITE_LOCK(table);

	ASSERT_RETFALSE(table->m_CodeBuffer);
	table->m_CodeBuffer->AllocateSpace(bufsize);
	ASSERT_RETFALSE(fbuf.PopBuf(table->m_CodeBuffer->GetCurBuf(), bufsize));
	table->m_CodeBuffer->Advance(bufsize);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableLoadCookedScriptSymbols(
	EXCEL_TABLE * table,
	BYTE_BUF & fbuf)
{
	if (!table->m_Struct->m_hasScriptField)
	{
		return TRUE;
	}

	DWORD dw;
	ASSERT_RETFALSE(fbuf.PopInt(&dw));
	ASSERT_RETFALSE(dw == EXCEL_TABLE_MAGIC);

	EXCEL_SCRIPT_READ_FROM_PAK * fpScriptReadFromPak = Excel().m_fpScriptReadFromPak;
	ASSERT_RETFALSE(fpScriptReadFromPak);

	ASSERT_RETFALSE(fpScriptReadFromPak(fbuf));

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableLoadCookedIndexes(
	EXCEL_TABLE * table,
	BYTE_BUF & fbuf)
{
	TABLE_INDEX_WRITE_LOCK(table);

	for (int ii = 0; ii < EXCEL_STRUCT::EXCEL_MAX_INDEXES; ii++)
	{
		EXCEL_INDEX & index = table->m_Indexes[ii];

		index.m_IndexNodes.Destroy(NULL);

		DWORD dw;
		ASSERT_RETFALSE(fbuf.PopInt(&dw));
		ASSERT_RETFALSE(dw == EXCEL_TABLE_MAGIC);

		unsigned int count;
		ASSERT_RETFALSE(fbuf.PopInt(&count));

		for (unsigned int jj = 0; jj < count; jj++)
		{
			unsigned int row;
			ASSERT_RETFALSE(fbuf.PopInt(&row));

			ASSERT_RETFALSE(row < table->m_Data.Count());
			BYTE * rowdata = table->m_Data[row].m_RowData;
			ASSERT_RETFALSE(rowdata);

			EXCEL_INDEX_NODE node(row, rowdata);
			index.m_IndexNodes.InsertEnd(MEMORY_FUNC_FILELINE(NULL, node));
		}		
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableLoadCookedStrDicts(
	EXCEL_TABLE * table,
	BYTE_BUF & fbuf)
{
	TABLE_INDEX_WRITE_LOCK(table);

	for (unsigned int ii = 0; ii < table->m_Struct->m_Fields.Count(); ii++)
	{
		EXCEL_FIELD & field = (EXCEL_FIELD &)table->m_Struct->m_Fields[ii];

		if (field.m_Type == EXCEL_FIELDTYPE_STRINT)
		{
			EXCEL_LOCK(CWRAutoLockWriter, &field->m_FieldData.csStrIntDict);

			ASSERT_RETFALSE(field.m_FieldData.strintdict == NULL);
			field.m_FieldData.strintdict = (STRINT_DICT *)MALLOCZ(NULL, sizeof(STRINT_DICT));
			field.m_FieldData.strintdict->Init();
			field.m_FieldData.strintdict->ReadFromBuffer(fbuf);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableLoadCookedDefaultStats(
	EXCEL_TABLE * table,
	BYTE_BUF & fbuf,
	unsigned int row)
{
	if (!table->m_Struct->m_hasStatsList)
	{
		return TRUE;
	}

	EXCEL_STATS_READ_STATS * fpReadStats = Excel().m_fpStatsReadStats;
	ASSERT_RETFALSE(fpReadStats);

	unsigned int buflen;
	ASSERT_RETFALSE(fbuf.PopInt(&buflen));
	if (buflen == 0)
	{
		return TRUE;
	}

	STATS * stats = table->GetOrCreateStatsList(row);

	BIT_BUF buf(fbuf.GetPtr(), buflen);
	ASSERT_RETFALSE(fpReadStats(NULL, stats, buf, STATS_WRITE_METHOD_PAK));

	fbuf.SetCursor(fbuf.GetCursor() + buflen);

	return TRUE;
}


//----------------------------------------------------------------------------
// convert int offsets from string table to dynamic fields
//----------------------------------------------------------------------------
static BOOL sExcelTableLoadCookedDataDynamicStringsFixup(
	EXCEL_TABLE * table,
	BYTE * rowdata)
{
	unsigned int strbuflen;
	const char * strbuf = table->m_DStrings.GetReadBuffer(strbuflen);

	ASSERT_RETFALSE(table->m_Key);
	ASSERT_RETFALSE(MemoryCopy(table->m_Key, table->m_Struct->m_Size, rowdata, table->m_Struct->m_Size));
	BYTE * src = table->m_Key;

	for (unsigned int ii = 0; ii < table->m_Struct->m_Fields.Count(); ++ii)
	{
		const EXCEL_FIELD & field = table->m_Struct->m_Fields[ii];
		if (field.m_Type != EXCEL_FIELDTYPE_DYNAMIC_STRING)
		{
			continue;
		}
		BYTE * src_fielddata = src + field.m_FieldData.offset64;
		BYTE * dest_fielddata = rowdata + field.m_nOffset;
		unsigned int link = *(int *)src_fielddata;
		if (link == EXCEL_LINK_INVALID)
		{
			*(const char **)dest_fielddata = EXCEL_BLANK;
			continue;
		}
		ASSERT_RETFALSE(link < strbuflen);
		*(const char **)dest_fielddata = (const char *)(strbuf + link);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableLoadCookedData(
	EXCEL_TABLE * table,
	BYTE_BUF & fbuf)
{
	DWORD dw;
	ASSERT_RETFALSE(fbuf.PopInt(&dw));
	ASSERT_RETFALSE(dw == EXCEL_TABLE_MAGIC);

	unsigned int count;
	ASSERT_RETFALSE(fbuf.PopInt(&count));
	if (count == 0)
	{
		return TRUE;
	}

	unsigned int rowsize = table->m_Struct->m_Size + sizeof(EXCEL_DATA_VERSION);
	
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	BYTE * buffer = (BYTE *)MALLOC(g_StaticAllocator, rowsize * count);		// memory leak if fail after this point!
#else	
	BYTE * buffer = (BYTE *)MALLOC(NULL, rowsize * count);		// memory leak if fail after this point!
#endif
	
	table->m_DataToFree.InsertEnd(MEMORY_FUNC_FILELINE(NULL, buffer));
	ASSERT_RETFALSE(fbuf.PopBuf(buffer, rowsize * count));

	ASSERT_RETFALSE(fbuf.PopInt(&dw));
	ASSERT_RETFALSE(dw == EXCEL_TABLE_MAGIC);

	BYTE * rowdata = buffer;

	TABLE_DATA_WRITE_LOCK(table);

	for (unsigned int ii = 0; ii < count; ++ii, rowdata += rowsize)
	{
		BYTE * data = ExcelGetDataFromRowData(rowdata);
		ASSERT_RETFALSE(sExcelTableLoadCookedDataDynamicStringsFixup(table, data));

		unsigned int index;
		ASSERT_RETFALSE(fbuf.PopInt(&index));

		table->PushRow(index);

		EXCEL_DATA row(rowdata, FALSE);
		table->m_Data.OverwriteIndex(MEMORY_FUNC_FILELINE(NULL, index, row));

		ASSERT_RETFALSE(sExcelTableLoadCookedDefaultStats(table, fbuf, index));
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableLoadCookedDynamicStrings(
	EXCEL_TABLE * table,
	BYTE_BUF & fbuf)
{
	DWORD dw;
	ASSERT_RETFALSE(fbuf.PopInt(&dw));
	ASSERT_RETFALSE(dw == EXCEL_TABLE_MAGIC);

	TABLE_DATA_WRITE_LOCK(table);
	ASSERT_RETFALSE(table->m_DStrings.ReadFromBuffer(fbuf));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableLoadCookedManifest(
	EXCEL_TABLE * table,
	BYTE_BUF & fbuf)
{
	REF(table);

#if ISVERSION(RETAIL_VERSION)
	EXCEL_LOAD_MANIFEST & load_manifest = Excel().m_LoadManifest;
	EXCEL_READ_MANIFEST & read_manifest = Excel().m_ReadManifest;
#else
	EXCEL_LOAD_MANIFEST load_manifest;
	EXCEL_READ_MANIFEST read_manifest;
#endif
	// read the load manifest
	ASSERT_RETFALSE(fbuf.PopInt(&load_manifest.m_VersionApp));
	ASSERT_RETFALSE(fbuf.PopInt(&load_manifest.m_VersionPackage));
	ASSERT_RETFALSE(fbuf.PopInt(&load_manifest.m_VersionMajorMin));
	ASSERT_RETFALSE(fbuf.PopInt(&load_manifest.m_VersionMajorMax));
	ASSERT_RETFALSE(fbuf.PopInt(&load_manifest.m_VersionMinorMin));
	ASSERT_RETFALSE(fbuf.PopInt(&load_manifest.m_VersionMinorMax));

	read_manifest.m_VersionApp = load_manifest.m_VersionApp;
	read_manifest.m_VersionPackage = load_manifest.m_VersionPackage;
	ASSERT_RETFALSE(fbuf.PopInt(&read_manifest.m_VersionMajor));
	ASSERT_RETFALSE(fbuf.PopInt(&read_manifest.m_VersionMinor));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelTableLoadCookedHeader(
	EXCEL_TABLE * table,
	BYTE_BUF & fbuf)
{
	FILE_HEADER hdr={0};
	ASSERT_RETFALSE(fbuf.PopInt(&hdr.dwMagicNumber));
	ASSERT_RETFALSE(hdr.dwMagicNumber == EXCEL_TABLE_MAGIC);
	ASSERT_RETFALSE(fbuf.PopInt(&hdr.nVersion));
	ASSERT_RETFALSE((unsigned int)hdr.nVersion == table->m_Struct->m_CRC32);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadFromBuffer(
	BYTE * buffer,
	unsigned int buflen)
{
	ASSERT_RETFALSE(buffer);
	ASSERT_RETFALSE(buflen > 0);

	BYTE_BUF fbuf(buffer, buflen);

	ASSERT_RETFALSE(sExcelTableLoadCookedHeader(this, fbuf));

	ASSERT_RETFALSE(sExcelTableLoadCookedManifest(this, fbuf));

	ASSERT_RETFALSE(sExcelTableLoadCookedDynamicStrings(this, fbuf));

	ASSERT_RETFALSE(sExcelTableLoadCookedData(this, fbuf));

	ASSERT_RETFALSE(sExcelTableLoadCookedStrDicts(this, fbuf));

	ASSERT_RETFALSE(sExcelTableLoadCookedIndexes(this, fbuf));

	ASSERT_RETFALSE(sExcelTableLoadCookedScriptSymbols(this, fbuf));

	ASSERT_RETFALSE(sExcelTableLoadCookedCodeBuffer(this, fbuf));

	ASSERT_RETFALSE(sExcelTableLoadCookedIsATable(this, fbuf));

	m_bRowsAdded = TRUE;

	PostIndexLoad();

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadFromCookedDirect(
	void)
{
	ExcelLog("EXCEL_TABLE::LoadFromCookedDirect()   TABLE: %s", m_Name); 

	DECLARE_LOAD_SPEC(spec, m_FilenameCooked);
	spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK | PAKFILE_LOAD_ADD_TO_PAK;
	BYTE * buffer = (BYTE *)PakFileLoadNow(spec);
	ASSERT_RETFALSE(buffer);

	AUTOFREE autofree(NULL, buffer);
	ASSERT_RETFALSE(LoadFromBuffer(buffer, spec.bytesread));
	m_GenTime = spec.gentime;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::LoadFromPak(
	void)
{
	ExcelLog("EXCEL_TABLE::LoadFromPak()   TABLE: %s", m_Name); 

	DECLARE_LOAD_SPEC(spec, m_FilenameCooked);
	spec.flags |= PAKFILE_LOAD_FORCE_FROM_PAK;
	spec.pool = g_ScratchAllocator;
	BYTE * buffer = (BYTE *)PakFileLoadNow(spec);
	ASSERT_RETFALSE(buffer);

	AUTOFREE autofree(g_ScratchAllocator, buffer);
	ASSERT_RETFALSE(LoadFromBuffer(buffer, spec.bytesread));
	m_GenTime = spec.gentime;
	return TRUE;
}


//----------------------------------------------------------------------------
// a isa b?
//----------------------------------------------------------------------------
static inline BOOL sExcelTypeIsAEvaluate(
	const EXCEL_TABLE * table,
	unsigned int a,
	unsigned int b)
{
	if (a == b)
	{
		return TRUE;
	}

	unsigned int count = table->m_Data.Count();
	unsigned int field_offset = table->m_Struct->m_IsAField->m_nOffset;
	unsigned int isa_count = table->m_Struct->m_IsAField->m_nCount;

	ASSERT_RETFALSE(a < count);
	ASSERT_RETFALSE(b < count);

	const BYTE * rowdataA = ExcelGetDataFromRowData(table->m_Data[a]);
	ASSERT(rowdataA);
	const unsigned int * pnTypeEquivalentA = (unsigned int *)(rowdataA + field_offset);

	// initialize stack
	enum
	{
		MAX_UNITTYPES_STACK_SIZE = 1024,
	};
	static unsigned int TypeStack[MAX_UNITTYPES_STACK_SIZE];
	unsigned int nStackTop = 0;

	for (unsigned int ii = 0; ii < isa_count; ii++)
	{
		if (pnTypeEquivalentA[ii] >= count)
		{
			break;
		}
		TypeStack[nStackTop++] = pnTypeEquivalentA[ii];
	}

	while (nStackTop > 0)
	{
		unsigned int type = TypeStack[--nStackTop];

		if (type == b)
		{
			return TRUE;
		}

		ASSERT_RETFALSE(type < table->m_Data.Count());
		const BYTE * rowdataB = ExcelGetDataFromRowData(table->m_Data[type]);
		ASSERT(rowdataB);
		const unsigned int * pnTypeEquivalentB = (unsigned int *)(rowdataB + field_offset);

		for (unsigned int ii = 0; ii < isa_count; ++ii)
		{
			if (pnTypeEquivalentB[ii] >= count)
			{
				break;
			}
			TypeStack[nStackTop++] = pnTypeEquivalentB[ii];
			ASSERT_RETFALSE(nStackTop < MAX_UNITTYPES_STACK_SIZE);
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::CreateIsATable(
	void)
{
	ASSERT_RETFALSE(m_Struct);
	ASSERT_RETFALSE(m_Struct->m_IsAField);
	unsigned int count = m_Data.Count();

	m_IsARowCount = count;
	m_IsARowSize = DWORD_FLAG_SIZE(m_IsARowCount);
	
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	m_IsATable = (DWORD *)MALLOCZ(g_StaticAllocator, m_IsARowCount * m_IsARowSize * sizeof(DWORD));
#else	
	m_IsATable = (DWORD *)MALLOCZ(NULL, m_IsARowCount * m_IsARowSize * sizeof(DWORD));
#endif
	

	DWORD * isa = m_IsATable;
	for (unsigned int ii = 0; ii < m_IsARowCount; ++ii)
	{
		BYTE * rowdata = ExcelGetDataFromRowData(m_Data[ii]);
		ASSERT_RETFALSE(rowdata);

		for (unsigned int jj = 0; jj < m_IsARowCount; ++jj)
		{
			BOOL bIsA = sExcelTypeIsAEvaluate(this, ii, jj);
			SETBIT(isa, jj, bIsA);
		}
		isa += m_IsARowSize;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::IsA(
	const EXCEL_CONTEXT & context, 
	unsigned int a,
	unsigned int b) const
{
	REF(context);

	TABLE_DATA_READ_LOCK(this);

	ASSERT_RETFALSE(a < m_IsARowCount);
	ASSERT_RETFALSE(b < m_IsARowCount);
	ASSERT_RETFALSE(m_IsATable);
	ASSERT_RETFALSE(m_IsARowSize > 0);

	const DWORD * isa = m_IsATable + a * m_IsARowSize;
	return TESTBIT(isa, b);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::NeedsUpdate(
	void)
{
	switch (m_UpdateState)
	{
	case EXCEL_UPDATE_STATE_NEEDS_UPDATE:
	case EXCEL_UPDATE_STATE_TEXT_READ:
	case EXCEL_UPDATE_STATE_INDEX_BUILT:
	case EXCEL_UPDATE_STATE_UPTODATE_INDEX_BUILT:
	case EXCEL_UPDATE_STATE_UPTODATE_LOADED:
		return TRUE;
	}

	if (!AppLoadsTable())
	{
		m_UpdateState = EXCEL_UPDATE_STATE_UPTODATE;
		return FALSE;
	}

	if (!AppCommonGetDirectLoad())
	{
		m_UpdateState = EXCEL_UPDATE_STATE_UPTODATE;
		return FALSE;
	}

	ASSERT_RETFALSE(m_Struct);

	// if the string table system has been updated, this table must be updated as well
	if (m_Struct->m_usesStringTable)	
	{
		if (Excel().StringTablesGetUpdateState() == EXCEL_UPDATE_STATE_UPDATED)
		{
#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEBUG_VERSION)
			TraceError("ERROR: Excel data system determined the data for table \"%s\" is invalid because the strings table was marked as having been updated.",this->m_Name);
#endif
			m_UpdateState = EXCEL_UPDATE_STATE_NEEDS_UPDATE;
			return TRUE;
		}
	}
		
	if (AppCommonGetRegenExcel())
	{
		m_UpdateState = EXCEL_UPDATE_STATE_NEEDS_UPDATE;
		return TRUE;
	}

	m_UpdateState = EXCEL_UPDATE_STATE_EXAMINED;

	for (unsigned int ii = 0; ii < m_Files.Count(); ++ii)
	{
		EXCEL_TABLE_FILE & file = m_Files[ii];
		if (!sExcelLoadsFile(file))
		{
			continue;
		}
		UINT64 gentime = 0;
		BOOL bNeedsUpdate = FileNeedsUpdate(m_FilenameCooked, file.m_Filename, EXCEL_TABLE_MAGIC, m_Struct->m_CRC32, 0, &gentime, &m_bInPak);
		if (gentime > m_GenTime)
		{
			m_GenTime = gentime;
		}
		if (bNeedsUpdate || ( AppCommonGetDirectLoad() && !m_bInPak ))
		{
			m_UpdateState = EXCEL_UPDATE_STATE_NEEDS_UPDATE;
			return TRUE;
		}
	}

	unsigned int ResultState = EXCEL_UPDATE_STATE_UPTODATE;

	// check if any of my link ancestors need update
	for (unsigned int ii = 0; ii < m_Struct->m_Ancestors.Count(); ++ii)
	{
		unsigned int idTable = m_Struct->m_Ancestors[ii];
		if (idTable == m_Index)
		{
			continue;
		}
		EXCEL_TABLE * table = Excel().GetTable(idTable);
		if (!table)
		{
			continue;
		}
		if (table->m_UpdateState == EXCEL_UPDATE_STATE_UPDATED || (table->m_UpdateState == EXCEL_UPDATE_STATE_UNKNOWN && table->NeedsUpdate()))
		{
#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEBUG_VERSION)
			TraceError( "ERROR: Excel data system determined the data for table \"%s\" is invalid because it's ancestor table \"%s\" has been updated.", this->m_Name, table->m_Name);
#endif
			m_UpdateState = EXCEL_UPDATE_STATE_NEEDS_UPDATE;
			return TRUE;
		}
		if (table->m_UpdateState == EXCEL_UPDATE_STATE_EXAMINED)
		{
			ResultState = EXCEL_UPDATE_STATE_UNKNOWN;
		}
	}

	m_UpdateState = ResultState;
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_POSTCREATE_POSTPROCESS(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(table->m_Struct);
	ASSERT_RETFALSE(table->m_PostCreateData);

	unsigned int rowsize = table->m_Struct->m_Size;
	ASSERT_RETFALSE(rowsize > 0);
	const BYTE * srcdata = (const BYTE *)table->m_PostCreateData;
	for (unsigned int ii = 0; ii < table->m_PostCreateSize; ++ii, srcdata += rowsize)
	{
		ASSERT_RETFALSE(table->AddRow(ii));
		BYTE * rowdata = (BYTE *)ExcelGetDataPrivate(table, ii);
		ASSERT_RETFALSE(rowdata);
		ASSERT_RETFALSE(MemoryCopy(rowdata, rowsize, srcdata, rowsize));
		ExcelAddRowToIndexPrivate(table, ii);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::Load(
	void)
{
	ASSERT_RETFALSE(m_Struct);

	BOOL result;

	if (!m_bNoLoad && (m_UpdateState < EXCEL_UPDATE_STATE_UPDATED || m_UpdateState == EXCEL_UPDATE_STATE_UPTODATE))
	{

		// if this table uses the string tables, load them now if not already loaded
		if (m_Struct->m_usesStringTable)	
		{
			if (Excel().StringTablesLoadAll() == FALSE)
			{
				return FALSE;
			}
		}
	
		BOOL bLoadDirect = NeedsUpdate();		
		if (bLoadDirect)
		{
			result = LoadDirect();
		}
		else
		{
			result = LoadFromPak();
		}
	}
	else
	{
		result = TRUE;
	}

	if (result)
	{
		if (m_PostCreateData && m_PostCreateSize > 0)
		{
			EXCEL_POSTCREATE_POSTPROCESS(this);
		}
		if (m_Struct->m_fpPostProcessTable)
		{
			m_Struct->m_fpPostProcessTable(this);
		}

		if (m_UpdateState < EXCEL_UPDATE_STATE_UPDATED)
		{
			m_UpdateState = EXCEL_UPDATE_STATE_UPDATED;
		}
		else
		{
			m_UpdateState = EXCEL_UPDATE_STATE_UPTODATE_LOADED;
		}
	}

	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL_TABLE::Init(
	unsigned int idTable,
	const char * name,
	const EXCEL_STRUCT * estruct,
	APP_GAME appGame,
	EXCEL_TABLE_SCOPE scope,
	unsigned int data1)
{
	ASSERT_RETURN(estruct);
	ASSERT_RETURN(estruct->m_Size > 0);

	m_UpdateState = EXCEL_UPDATE_STATE_UNKNOWN;

	EXCEL_LOCK_INIT(m_CS_Data);
	m_Data.Init();
	m_DataToFree.Init();
	m_DStrings.Init();
	m_DataEx = NULL;
	m_nDataExRowSize = 0;
	m_nDataExCount = 0;
	m_IsATable = NULL;
	m_IsARowSize = 0;
	m_IsARowCount = 0;
	m_Index = idTable;
	m_Name = name;
	m_Struct = estruct;
	m_bSelfIndex = (m_Struct->m_bSelfIndex || (m_Struct->m_Ancestors.FindExact(m_Index) != NULL));
	m_bInPak = FALSE;
	m_GenTime = 0;

#if ISVERSION(DEVELOPMENT)
	m_bTrace = FALSE;
#endif

	m_Key = (BYTE *)MALLOCZ(g_StaticAllocator, m_Struct->m_Size + sizeof(EXCEL_DATA_VERSION));
	
	m_CodeBuffer = &m_CodeBufferBase;
	m_CodeBuffer->Init(NULL);

	EXCEL_LOCK_INIT(m_CS_Index);
	for (int ii = 0; ii < EXCEL_STRUCT::EXCEL_MAX_INDEXES; ++ii)
	{
		const EXCEL_INDEX_DESC * indexDesc = NULL;
		if (m_Struct->m_Indexes[ii].m_Type != EXCEL_INDEXTYPE_INVALID)
		{
			indexDesc = &m_Struct->m_Indexes[ii];
		}

		m_Indexes[ii].Init(indexDesc);
	}

	AddFile(name, appGame, scope, data1);
	ASSERT(arrsize(m_Filename) == arrsize(m_FilenameCooked));
	sExcelConstructFilename(m_Filename, m_FilenameCooked, arrsize(m_Filename), name, scope);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE * EXCEL_TABLE::GetData(
	unsigned int row
#if EXCEL_FILE_LINE
	,
	 const char * file,
	 unsigned int line
#endif 
	 ) const

{
	TABLE_DATA_READ_LOCK(this);

#if EXCEL_FILE_LINE
	ASSERTV_RETNULL(row < m_Data.Count(), "Invalid row (%d) requested from table '%s' File: %s Line %u", row, m_Name, file, line);
#else
	ASSERTV_RETNULL(row < m_Data.Count(), "Invalid row (%d) requested from table '%s'", row, m_Name);
#endif

	return ExcelGetDataFromRowData(m_Data[row]);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * EXCEL_TABLE::GetData(
	const EXCEL_CONTEXT & context,
	unsigned int row
#if EXCEL_FILE_LINE
	,
	 const char * file,
	 unsigned int line
#endif 
	 ) const
{
	TABLE_DATA_READ_LOCK(this);

#if EXCEL_FILE_LINE
	ASSERTV_RETNULL(row < m_Data.Count(), "Invalid row (%d) requested from table '%s' File: %s Line %u", row, m_Name, file, line);
#else
	ASSERTV_RETNULL(row < m_Data.Count(), "Invalid row (%d) requested from table '%s'", row, m_Name);
#endif

	return ExcelGetDataFromRowData(context, m_Data[row]);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const void * EXCEL_TABLE::GetDataEx(
	const EXCEL_CONTEXT & context,
	unsigned int row) const
{
	REF(context);

	TABLE_DATA_READ_LOCK(this);

	ASSERT_RETNULL(row < m_nDataExCount);
	ASSERT_RETNULL(m_DataEx);
	return m_DataEx + row * m_nDataExRowSize;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE * EXCEL_TABLE::GetScriptCode(
	const EXCEL_CONTEXT & context,
	PCODE code_offset,
	int * maxlen)
{
	REF(context);

	if (code_offset == NULL)
	{
		if (maxlen)
		{
			*maxlen = 0;
		}
		return NULL;
	}

	TABLE_DATA_READ_LOCK(this);

	ASSERT_RETNULL(m_CodeBuffer);
	ASSERT_RETNULL((unsigned int)code_offset < m_CodeBuffer->GetPos());

	if (maxlen)
	{
		*maxlen = m_CodeBuffer->GetPos() - (unsigned int)code_offset;
	}

	return m_CodeBuffer->GetBuf((unsigned int)code_offset);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE * EXCEL_TABLE::GetScript(
	const EXCEL_CONTEXT & context,
	unsigned int field_offset,
	unsigned int row,
	unsigned int * maxlen)
{
	REF(context);

	ASSERT_RETNULL(m_Struct);
	ASSERT_RETNULL(field_offset + sizeof(PCODE) <= m_Struct->m_Size);

	TABLE_DATA_READ_LOCK(this);

	ASSERT_RETZERO(row < m_Data.Count());

	BYTE * rowdata = ExcelGetDataFromRowData(context, m_Data[row]);
	ASSERT_RETZERO(rowdata);

	PCODE code_offset = *(DWORD *)(rowdata + field_offset);
	if (code_offset == NULL_CODE)
	{
		return NULL;
	}

	ASSERT_RETNULL(m_CodeBuffer);
	ASSERT_RETNULL((unsigned int)code_offset < m_CodeBuffer->GetPos());

	if (maxlen)
	{
		*maxlen = m_CodeBuffer->GetPos() - (unsigned int)code_offset;
	}

	return m_CodeBuffer->GetBuf((unsigned int)code_offset);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE * EXCEL_TABLE::GetScriptNoLock(
	const EXCEL_CONTEXT & context,
	unsigned int field_offset,
	unsigned int row,
	unsigned int * maxlen) const
{
	REF(context);

	ASSERT_RETNULL(m_Struct);
	ASSERT_RETNULL(field_offset + sizeof(PCODE) <= m_Struct->m_Size);

	ASSERT_RETZERO(row < m_Data.Count());

	BYTE * rowdata = ExcelGetDataFromRowData(context, m_Data[row]);
	ASSERT_RETZERO(rowdata);

	PCODE code_offset = *(DWORD *)(rowdata + field_offset);
	if (code_offset == NULL_CODE)
	{
		return NULL;
	}

	ASSERT_RETNULL(m_CodeBuffer);
	ASSERT_RETNULL((unsigned int)code_offset < m_CodeBuffer->GetPos());

	if (maxlen)
	{
		*maxlen = m_CodeBuffer->GetPos() - (unsigned int)code_offset;
	}

	return m_CodeBuffer->GetBuf((unsigned int)code_offset);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL_TABLE::HasScript(
	const EXCEL_CONTEXT & context,
	unsigned int field_offset,
	unsigned int row)
{
	REF(context);

	BYTE * script = GetScript(context, field_offset, row);
	return (script != NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int EXCEL_TABLE::EvalScript(
	const EXCEL_CONTEXT & context,
	struct UNIT * subject,
	struct UNIT * object,
	struct STATS * statslist,
	unsigned int field_offset,
	unsigned int row)
{
	EXCEL_SCRIPT_EXECUTE * fpScriptExecute = Excel().m_fpScriptExecute;
	ASSERT_RETZERO(fpScriptExecute);

	unsigned int maxlen;
	BYTE * script = GetScript(context, field_offset, row, &maxlen);
	if (!script)
	{
		return 0;
	}

	char strError[1024];
	strError[0] = 0;
#if	ISVERSION(DEVELOPMENT)
	PStrPrintf(strError, arrsize(strError), "Table: %s  Field Offset: %u  Row: %u", m_Name, field_offset, row);
#endif
	return fpScriptExecute(context.game, subject, object, statslist, script, maxlen, strError);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int EXCEL_TABLE::EvalScriptNoLock(
	const EXCEL_CONTEXT & context,
	struct UNIT * subject,
	struct UNIT * object,
	struct STATS * statslist,
	unsigned int field_offset,
	unsigned int row) const
{
	EXCEL_SCRIPT_EXECUTE * fpScriptExecute = Excel().m_fpScriptExecute;
	ASSERT_RETZERO(fpScriptExecute);

	unsigned int maxlen;
	BYTE * script = GetScriptNoLock(context, field_offset, row, &maxlen);
	if (!script)
	{
		return 0;
	}

	char strError[1024];
	strError[0] = 0;
#if	ISVERSION(DEVELOPMENT)
	PStrPrintf(strError, arrsize(strError), "Table: %s  Field Offset: %u  Row: %u", m_Name, field_offset, row);
#endif
	return fpScriptExecute(context.game, subject, object, statslist, script, maxlen, strError);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct STATS * EXCEL_TABLE::GetStatsList(
	const EXCEL_CONTEXT & context,
	unsigned int row) const
{
	REF(context);

	ASSERT_RETNULL(m_Struct && m_Struct->m_hasStatsList);

	TABLE_DATA_READ_LOCK(this);

	ASSERT_RETNULL(m_Data.Count() > row);

	return m_Data[row].m_Stats;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int EXCEL_TABLE::GetLineByCode(
	const EXCEL_CONTEXT & context,
	DWORD cc)
{
	REF(context);

	ASSERT_RETVAL(m_Struct, EXCEL_LINK_INVALID);
	ASSERTV_RETVAL(m_Struct->m_CodeIndex != EXCEL_LINK_INVALID, EXCEL_LINK_INVALID, "EXCEL_TABLE::GetLineByCode() -- TABLE:%s  Attempt to index by code on a table with no code index!", m_Name);
    ASSERT_RETVAL(m_Struct->m_CodeIndex < ARRAYSIZE(m_Indexes),EXCEL_LINK_INVALID);

	TABLE_INDEX_READ_LOCK(this);

	const EXCEL_INDEX_NODE * node = NULL;
	switch (m_Indexes[m_Struct->m_CodeIndex].m_IndexDesc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_FOURCC:
	case EXCEL_FIELDTYPE_THREECC:
		node = m_Indexes[m_Struct->m_CodeIndex].FindByCC<DWORD>(this, cc);
		break;
	case EXCEL_FIELDTYPE_TWOCC:
		node = m_Indexes[m_Struct->m_CodeIndex].FindByCC<WORD>(this, (WORD)cc);
		break;
	case EXCEL_FIELDTYPE_ONECC:
		node = m_Indexes[m_Struct->m_CodeIndex].FindByCC<BYTE>(this, (BYTE)cc);
		break;
	default:
		ASSERT_RETVAL(0, EXCEL_LINK_INVALID);
	}
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	return node->m_RowIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * EXCEL_TABLE::GetDataByCode(
	const EXCEL_CONTEXT & context,
	DWORD cc)
{
	REF(context);

	ASSERT_RETNULL(m_Struct);
	ASSERTV_RETNULL(m_Struct->m_CodeIndex != EXCEL_LINK_INVALID, "EXCEL_TABLE::GetDataByCode() -- TABLE:%s  Attempt to index by code on a table with no code index!", m_Name);
    ASSERT_RETNULL(m_Struct->m_CodeIndex < ARRAYSIZE(m_Indexes));

	TABLE_INDEX_READ_LOCK(this);

	const EXCEL_INDEX_NODE * node = NULL;
	switch (m_Indexes[m_Struct->m_CodeIndex].m_IndexDesc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_FOURCC:
	case EXCEL_FIELDTYPE_THREECC:
		node = m_Indexes[m_Struct->m_CodeIndex].FindByCC<DWORD>(this, cc);
		break;
	case EXCEL_FIELDTYPE_TWOCC:
		node = m_Indexes[m_Struct->m_CodeIndex].FindByCC<WORD>(this, (WORD)cc);
		break;
	case EXCEL_FIELDTYPE_ONECC:
		node = m_Indexes[m_Struct->m_CodeIndex].FindByCC<BYTE>(this, (BYTE)cc);
		break;
	default:
		ASSERT_RETNULL(0);
	}
	if (!node)
	{
		return NULL;
	}
	return ExcelGetDataFromRowData(context, *node);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD EXCEL_TABLE::GetCodeByLine(
	const EXCEL_CONTEXT & context,
	unsigned int row) const
{
	REF(context);

	ASSERT_RETZERO(m_Struct);
	ASSERTV_RETVAL(m_Struct->m_CodeIndex != EXCEL_LINK_INVALID, EXCEL_LINK_INVALID, "EXCEL_TABLE::GetCodeByLine() -- TABLE:%s  Attempt to index by code on a table with no code index!", m_Name);
	ASSERT_RETZERO(m_Struct->m_CodeIndex < ARRAYSIZE(m_Indexes));

	TABLE_DATA_READ_LOCK(this);

	ASSERT_RETZERO(row < m_Data.Count());

	BYTE * rowdata = ExcelGetDataFromRowData(context, m_Data[row]);
	ASSERT_RETZERO(rowdata);

	EXCEL_FIELD * field = m_Indexes[m_Struct->m_CodeIndex].m_IndexDesc->m_IndexFields[0];
	ASSERT_RETZERO(field);

	DWORD code;
	switch (field->m_Type)
	{
	case EXCEL_FIELDTYPE_FOURCC:
	case EXCEL_FIELDTYPE_THREECC:
		code = *(DWORD *)(rowdata + field->m_nOffset);
		break;
	case EXCEL_FIELDTYPE_TWOCC:
		code = *(WORD *)(rowdata + field->m_nOffset);
		break;
	case EXCEL_FIELDTYPE_ONECC:
		code = *(BYTE *)(rowdata + field->m_nOffset);
		break;
	default:
		ASSERT_RETZERO(0);
	}
	return code;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK EXCEL_TABLE::GetLineByKey(
	unsigned int idIndex,
	const void * key,
	unsigned int len) const
{
	ASSERT_RETVAL(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(key, EXCEL_LINK_INVALID);

	ASSERT_RETVAL(m_Struct, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(len == m_Struct->m_Size, EXCEL_LINK_INVALID);

	TABLE_INDEX_READ_LOCK(this);

	const EXCEL_INDEX_NODE * node = m_Indexes[idIndex].Find((const BYTE *)key);
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	return node->m_RowIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK EXCEL_TABLE::GetLineByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idIndex,
	const void * key,
	unsigned int len) const
{
	REF(context);
	ASSERT_RETVAL(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(key, EXCEL_LINK_INVALID);

	ASSERT_RETVAL(m_Struct, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(len == m_Struct->m_Size, EXCEL_LINK_INVALID);

	TABLE_INDEX_READ_LOCK(this);

	const EXCEL_INDEX_NODE * node = m_Indexes[idIndex].Find((const BYTE *)key);
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	return node->m_RowIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK EXCEL_TABLE::GetClosestLineByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idIndex,
	const void * key,
	unsigned int len,
	BOOL & bExact) const
{
	REF(context);
	ASSERT_RETVAL(key, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES, EXCEL_LINK_INVALID);

	ASSERT_RETVAL(m_Struct, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(len == m_Struct->m_Size, EXCEL_LINK_INVALID);

	TABLE_INDEX_READ_LOCK(this);

	const EXCEL_INDEX_NODE * node = m_Indexes[idIndex].FindClosest((const BYTE *)key, bExact);
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	return node->m_RowIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK EXCEL_TABLE::GetNextLineByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idIndex,
	const void * key,
	unsigned int len) const
{
	REF(context);
	ASSERT_RETVAL(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES, EXCEL_LINK_INVALID);

	ASSERT_RETVAL(m_Struct, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(len == m_Struct->m_Size, EXCEL_LINK_INVALID);

	TABLE_INDEX_READ_LOCK(this);

	BOOL bExact;
	const EXCEL_INDEX_NODE * node = m_Indexes[idIndex].FindClosest((const BYTE *)key, bExact);
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	if (!bExact)
	{
		return node->m_RowIndex;
	}
	if (node >= m_Indexes[idIndex].m_IndexNodes.GetPointer() + m_Indexes[idIndex].m_IndexNodes.Count())
	{
		return EXCEL_LINK_INVALID;
	}
	++node;
	return node->m_RowIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK EXCEL_TABLE::GetPrevLineByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idIndex,
	const void * key,
	unsigned int len) const
{
	REF(context);
	ASSERT_RETVAL(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES, EXCEL_LINK_INVALID);

	ASSERT_RETVAL(m_Struct, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(len == m_Struct->m_Size, EXCEL_LINK_INVALID);

	TABLE_INDEX_READ_LOCK(this);

	BOOL bExact;
	const EXCEL_INDEX_NODE * node = m_Indexes[idIndex].FindClosest((const BYTE *)key, bExact);
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	if (node <= m_Indexes[idIndex].m_IndexNodes.GetPointer())
	{
		return EXCEL_LINK_INVALID;
	}
	--node;	
	ASSERT_RETVAL(node, EXCEL_LINK_INVALID);
	return node->m_RowIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * EXCEL_TABLE::GetDataByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idIndex,
	const void * key,
	unsigned int len) const
{
	REF(context);
	ASSERT_RETNULL(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES);

	ASSERT_RETNULL(m_Struct);
	ASSERT_RETNULL(len == m_Struct->m_Size);

	TABLE_INDEX_READ_LOCK(this);

	const EXCEL_INDEX_NODE * node = m_Indexes[idIndex].Find((const BYTE *)key);
	if (!node)
	{
		return NULL;
	}
	return ExcelGetDataFromRowData(context, *node);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T> 
EXCEL_LINK EXCEL_TABLE::GetLineByStrKeyNoLock(
	T key) const
{
	ASSERT_RETVAL(m_Struct, EXCEL_LINK_INVALID);
	ASSERTV_RETVAL(m_Struct->m_StringIndex != EXCEL_LINK_INVALID, EXCEL_LINK_INVALID, "EXCEL_TABLE::GetLineByStrKeyNoLock()  TABLE DOESN'T HAVE A STRING INDEX.  TABLE:%s", m_Name);
	ASSERT_RETVAL(m_Struct->m_StringIndex < ARRAYSIZE(m_Indexes), EXCEL_LINK_INVALID);

	const EXCEL_INDEX_NODE * node = NULL;
	switch (m_Indexes[m_Struct->m_StringIndex].m_IndexDesc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
		node = m_Indexes[m_Struct->m_StringIndex].FindByStrKey(key);
		break;
	default:
		ASSERT_RETVAL(0, EXCEL_LINK_INVALID);
	}
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	return node->m_RowIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T> 
EXCEL_LINK EXCEL_TABLE::GetLineByStrKey(
	T key) const
{
	ASSERT_RETVAL(m_Struct, EXCEL_LINK_INVALID);
	ASSERTV_RETVAL(m_Struct->m_StringIndex != EXCEL_LINK_INVALID, EXCEL_LINK_INVALID, "EXCEL_TABLE::GetLineByStrKey()  TABLE DOESN'T HAVE A STRING INDEX.  TABLE:%s", m_Name);
	ASSERT_RETVAL(m_Struct->m_StringIndex < ARRAYSIZE(m_Indexes), EXCEL_LINK_INVALID);

	TABLE_INDEX_READ_LOCK(this);

	const EXCEL_INDEX_NODE * node = NULL;
	switch (m_Indexes[m_Struct->m_StringIndex].m_IndexDesc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
		node = m_Indexes[m_Struct->m_StringIndex].FindByStrKey(key);
		break;
	default:
		ASSERT_RETVAL(0, EXCEL_LINK_INVALID);
	}
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	return node->m_RowIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static unsigned int sExcelGetStrIntByKey(
	EXCEL_FIELD * field,
	const char * key)
{
	ASSERT_RETVAL(field, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(key, EXCEL_LINK_INVALID);

	EXCEL_LOCK(CWRAutoLockReader, &field->m_FieldData.csStrIntDict);

	if (!field->m_FieldData.strintdict)
	{
		return EXCEL_LINK_INVALID;
	}
	return field->m_FieldData.strintdict->GetValByStr(key);			
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static unsigned int sExcelGetStrLinkByKey(
	const EXCEL_TABLE * table,
	const EXCEL_INDEX & index,
	const char * key)
{
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(key, EXCEL_LINK_INVALID);

	TABLE_INDEX_READ_LOCK(table);

	const EXCEL_INDEX_NODE * node = index.FindByStrKey(key);
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	return node->m_RowIndex;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static unsigned int sExcelGetStrLinkByFourCC(
	const EXCEL_TABLE * table,
	const EXCEL_INDEX & index,
	const char * key)
{
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(key, EXCEL_LINK_INVALID);

	TABLE_INDEX_READ_LOCK(table);

	DWORD cc = STRTOFOURCC(key);
	const EXCEL_INDEX_NODE * node = index.FindByCC(table, cc);
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	return node->m_RowIndex;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T> 
EXCEL_LINK EXCEL_TABLE::GetLineByStrKey(
	unsigned int idIndex,
	T key) const
{
	ASSERT_RETVAL(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES, EXCEL_LINK_INVALID);

	const EXCEL_INDEX_DESC * desc = m_Indexes[idIndex].m_IndexDesc;
	ASSERT_RETVAL(desc, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(desc->m_IndexFields[0], EXCEL_LINK_INVALID);
	ASSERT_RETVAL(desc->m_IndexFields[1] == NULL, EXCEL_LINK_INVALID);
	switch (desc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
		return sExcelGetStrLinkByKey(this, m_Indexes[idIndex], key);
	case EXCEL_FIELDTYPE_STRINT:
		return sExcelGetStrIntByKey(desc->m_IndexFields[0], key);
	case EXCEL_FIELDTYPE_FOURCC:
		return sExcelGetStrLinkByFourCC(this, m_Indexes[idIndex], key);
	default:
		ASSERT_RETVAL(0, EXCEL_LINK_INVALID);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T> 
EXCEL_LINK EXCEL_TABLE::GetLineByStrKey(
	const EXCEL_CONTEXT & context,
	T key) const
{
	REF(context);

	ASSERT_RETVAL(m_Struct, EXCEL_LINK_INVALID);
	ASSERTV_RETVAL(m_Struct->m_StringIndex != EXCEL_LINK_INVALID, EXCEL_LINK_INVALID, "EXCEL_TABLE::GetLineByStrKey()  TABLE DOESN'T HAVE A STRING INDEX.  TABLE:%s", m_Name);
	ASSERT_RETVAL(m_Struct->m_StringIndex < ARRAYSIZE(m_Indexes), EXCEL_LINK_INVALID);

	switch (m_Indexes[m_Struct->m_StringIndex].m_IndexDesc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
		return sExcelGetStrLinkByKey(this, m_Indexes[m_Struct->m_StringIndex], key);
	case EXCEL_FIELDTYPE_STRINT:
		return sExcelGetStrIntByKey(m_Indexes[m_Struct->m_StringIndex].m_IndexDesc->m_IndexFields[0], key);
	default:
		ASSERT_RETVAL(0, EXCEL_LINK_INVALID);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T> 
EXCEL_LINK EXCEL_TABLE::GetLineByStrKey(
	const EXCEL_CONTEXT & context,
	unsigned int idIndex,
	T key) const
{
	REF(context);

	ASSERT_RETVAL(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES, EXCEL_LINK_INVALID);

	const EXCEL_INDEX_DESC * desc = m_Indexes[idIndex].m_IndexDesc;
	ASSERT_RETVAL(desc, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(desc->m_IndexFields[0], EXCEL_LINK_INVALID);
	ASSERT_RETVAL(desc->m_IndexFields[1] == NULL, EXCEL_LINK_INVALID);
	switch (desc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
		return sExcelGetStrLinkByKey(this, m_Indexes[idIndex], key);
	case EXCEL_FIELDTYPE_STRINT:
		return sExcelGetStrIntByKey(desc->m_IndexFields[0], key);
	default:
		ASSERT_RETVAL(0, EXCEL_LINK_INVALID);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T> 
EXCEL_LINK EXCEL_TABLE::GetNextLineByStrKey(
	const EXCEL_CONTEXT & context,
	unsigned int idIndex,
	T key) const
{
	REF(context);

	ASSERT_RETVAL(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES, EXCEL_LINK_INVALID);

	const EXCEL_INDEX_DESC * desc = m_Indexes[idIndex].m_IndexDesc;
	ASSERT_RETVAL(desc, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(desc->m_IndexFields[0], EXCEL_LINK_INVALID);
	ASSERT_RETVAL(desc->m_IndexFields[1] == NULL, EXCEL_LINK_INVALID);
	switch (desc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
	case EXCEL_FIELDTYPE_STRINT:
		break;
	default:
		ASSERT_RETVAL(0, EXCEL_LINK_INVALID);
	}

	BOOL bExact;
	const EXCEL_INDEX_NODE * node = m_Indexes[idIndex].FindClosestByStrKey(key, bExact);
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	if (!bExact)
	{
		return node->m_RowIndex;
	}
	if (node >= m_Indexes[idIndex].m_IndexNodes.GetPointer() + m_Indexes[idIndex].m_IndexNodes.Count() - 1)
	{
		return EXCEL_LINK_INVALID;
	}
	++node;
	return node->m_RowIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T> 
EXCEL_LINK EXCEL_TABLE::GetPrevLineByStrKey(
	const EXCEL_CONTEXT & context,
	unsigned int idIndex,
	T key) const
{
	REF(context);

	ASSERT_RETVAL(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES, EXCEL_LINK_INVALID);

	const EXCEL_INDEX_DESC * desc = m_Indexes[idIndex].m_IndexDesc;
	ASSERT_RETVAL(desc, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(desc->m_IndexFields[0], EXCEL_LINK_INVALID);
	ASSERT_RETVAL(desc->m_IndexFields[1] == NULL, EXCEL_LINK_INVALID);
	switch (desc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
	case EXCEL_FIELDTYPE_STRINT:
		break;
	default:
		ASSERT_RETVAL(0, EXCEL_LINK_INVALID);
	}

	BOOL bExact;
	const EXCEL_INDEX_NODE * node = m_Indexes[idIndex].FindClosestByStrKey(key, bExact);
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	if (node <= m_Indexes[idIndex].m_IndexNodes.GetPointer())
	{
		return EXCEL_LINK_INVALID;
	}
	--node;	
	ASSERT_RETVAL(node, EXCEL_LINK_INVALID);
	return node->m_RowIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T> 
void * EXCEL_TABLE::GetDataByStrKey(
	const EXCEL_CONTEXT & context,
	T key) const
{
	REF(context);

	ASSERT_RETNULL(m_Struct);
	ASSERTV_RETNULL(m_Struct->m_StringIndex != EXCEL_LINK_INVALID, "EXCEL_TABLE::GetDataByStrKey()  TABLE DOESN'T HAVE A STRING INDEX.  TABLE:%s", m_Name);
	ASSERT_RETNULL(m_Struct->m_StringIndex < ARRAYSIZE(m_Indexes));

	TABLE_INDEX_READ_LOCK(this);

	const EXCEL_INDEX_NODE * node = NULL;
	switch (m_Indexes[m_Struct->m_StringIndex].m_IndexDesc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
	case EXCEL_FIELDTYPE_STRINT:
		node = m_Indexes[m_Struct->m_StringIndex].FindByStrKey(key);
		break;
	default:
		ASSERT_RETNULL(0);
	}
	if (!node)
	{
		return NULL;
	}
	return ExcelGetDataFromRowData(context, *node);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T> 
T EXCEL_TABLE::GetStringKeyByLine(
	const EXCEL_CONTEXT & context,
	unsigned int idIndex,
	unsigned int row) const
{
	static T strNull = (T)("");
	REF(context);

	ASSERT_RETVAL(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES, strNull);

	const EXCEL_INDEX_DESC * desc = m_Indexes[idIndex].m_IndexDesc;
	ASSERT_RETVAL(desc, strNull);
	ASSERT_RETVAL(desc->m_IndexFields[0], strNull);
	ASSERT_RETVAL(desc->m_IndexFields[1] == NULL, strNull);
	switch (desc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_STRINT:
		{
			TABLE_DATA_READ_LOCK(this);

			ASSERT_RETVAL(row < m_Data.Count(), strNull);
			BYTE * rowdata = ExcelGetDataFromRowData(context, m_Data[row]);
			ASSERT_RETVAL(rowdata, strNull);

			return (T)(rowdata + desc->m_IndexFields[0]->m_nOffset);
		}
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
		{
			TABLE_DATA_READ_LOCK(this);

			ASSERT_RETVAL(row < m_Data.Count(), strNull);
			BYTE * rowdata = ExcelGetDataFromRowData(context, m_Data[row]);
			ASSERT_RETVAL(rowdata, strNull);

			return (T)*(T *)(rowdata + desc->m_IndexFields[0]->m_nOffset);
		}
	default:
		ASSERT_RETVAL(0, strNull);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T> 
T EXCEL_TABLE::GetStringKeyByLine(
	const EXCEL_CONTEXT & context,
	unsigned int row) const
{
	static T strNull = (T)("");
	if (row == EXCEL_LINK_INVALID)
	{
		return strNull;
	}

	ASSERT_RETVAL(m_Struct, strNull);
	ASSERTV_RETVAL(m_Struct->m_StringIndex != EXCEL_LINK_INVALID, strNull, "EXCEL_TABLE::GetStringKeyByLine()  TABLE DOESN'T HAVE A STRING INDEX.  TABLE:%s", m_Name);
	ASSERT_RETVAL(m_Struct->m_StringIndex < ARRAYSIZE(m_Indexes), strNull);

	switch (m_Indexes[m_Struct->m_StringIndex].m_IndexDesc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
	case EXCEL_FIELDTYPE_STRINT:
		{
			TABLE_DATA_READ_LOCK(this);

			ASSERT_RETVAL(row < m_Data.Count(), strNull);
			BYTE * rowdata = ExcelGetDataFromRowData(context, m_Data[row]);
			if(!rowdata)
				return strNull;

			return (T)(rowdata + m_Indexes[m_Struct->m_StringIndex].m_IndexDesc->m_IndexFields[0]->m_nOffset);
		}
	case EXCEL_FIELDTYPE_DYNAMIC_STRING:
		{
			TABLE_DATA_READ_LOCK(this);

			ASSERT_RETVAL(row < m_Data.Count(), strNull);
			BYTE * rowdata = ExcelGetDataFromRowData(context, m_Data[row]);
			if(!rowdata)
				return strNull;

			return (T)*(T *)(rowdata + m_Indexes[m_Struct->m_StringIndex].m_IndexDesc->m_IndexFields[0]->m_nOffset);
		}
	default:
		ASSERT_RETVAL(0, strNull);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * EXCEL_TABLE::GetNameStringByLine(
	const EXCEL_CONTEXT & context,
	unsigned int row,
	GRAMMAR_NUMBER eNumber) const
{
	static WCHAR * strNull = L"";
	if (row == EXCEL_LINK_INVALID)
	{
		return strNull;
	}

	ASSERT_RETVAL(m_Struct, sExcelStringTableLookupVerbose());
	ASSERT_RETVAL(m_Struct->m_NameField != NULL, sExcelStringTableLookupVerbose());

	switch (m_Struct->m_NameField->m_Type)
	{
	case EXCEL_FIELDTYPE_STR_TABLE:
		break;
	default:
		ASSERT_RETVAL(0, sExcelStringTableLookupVerbose());
	}

	DWORD dwAttributes = StringTableCommonGetGrammarNumberAttribute(INVALID_INDEX, eNumber);		
	
	TABLE_DATA_READ_LOCK(this);

	ASSERT_RETVAL(row < m_Data.Count(), sExcelStringTableLookupVerbose());

	BYTE * rowdata = ExcelGetDataFromRowData(context, m_Data[row]);
	if (!rowdata)
	{
		return NULL;
	}

	int stridx = *(int *)(rowdata + m_Struct->m_NameField->m_nOffset);
	if (stridx < 0)
	{
		return sExcelStringTableLookupVerbose();
	}

	int nStringTable = LanguageGetCurrent();	
	const WCHAR * str = StringTableCommonGetStringByIndexVerbose(nStringTable, stridx, dwAttributes, 0, NULL, NULL);
	ASSERT_RETVAL(str, sExcelStringTableLookupVerbose());
		
	return str;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
EXCEL_LINK EXCEL_TABLE::GetLineByCC(
	unsigned int idIndex,
	T cc)
{
	ASSERT_RETVAL(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES, EXCEL_LINK_INVALID);

	const EXCEL_INDEX_DESC * desc = m_Indexes[idIndex].m_IndexDesc;
	ASSERT_RETVAL(desc, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(desc->m_IndexFields[0], EXCEL_LINK_INVALID);
	ASSERT_RETVAL(desc->m_IndexFields[1] == NULL, EXCEL_LINK_INVALID);
	switch (desc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_FOURCC:
	case EXCEL_FIELDTYPE_THREECC:
	case EXCEL_FIELDTYPE_TWOCC:
	case EXCEL_FIELDTYPE_ONECC:
		break;
	default:
		ASSERT_RETVAL(0, EXCEL_LINK_INVALID);
	}

	TABLE_INDEX_READ_LOCK(this);

	const EXCEL_INDEX_NODE * node = m_Indexes[idIndex].FindByCC<T>(this, cc);
	if (!node)
	{
		return EXCEL_LINK_INVALID;
	}
	return node->m_RowIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int EXCEL_TABLE::Lookup(
	const EXCEL_CONTEXT & context,
	unsigned int idField,
	unsigned int row,
	unsigned int arrayindex)
{
	REF(context);
	ASSERT_RETZERO(m_Struct);
	ASSERT_RETZERO(idField < m_Struct->m_Fields.Count());
	const EXCEL_FIELD * field = &m_Struct->m_Fields[idField];
	ASSERT_RETZERO(field);

#if EXCEL_FILE_LINE
	const BYTE * rowdata = (BYTE *)GetData(context, row, __FILE__, __LINE__);
#else
	const BYTE * rowdata = (BYTE *)GetData(context, row);
#endif
	ASSERT_RETZERO(rowdata);

	const BYTE * fielddata = rowdata + field->m_nOffset;
	return field->Lookup(context, this, row, fielddata, arrayindex);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if EXCEL_TRACE
void EXCEL_TABLE::TraceLink(
	unsigned int idIndex, 
	EXCEL_LINK link) const
{
	ASSERT_RETURN(idIndex < EXCEL_STRUCT::EXCEL_MAX_INDEXES);

	const EXCEL_INDEX_DESC * desc = m_Indexes[idIndex].m_IndexDesc;
	ASSERT_RETURN(desc);
	ASSERT_RETURN(desc->m_IndexFields[0]);

	if (desc->m_IndexFields[1])
	{
		ExcelTrace("%d", (unsigned int)link);
		return;
	}
	switch (desc->m_IndexFields[0]->m_Type)
	{
	case EXCEL_FIELDTYPE_STRING:
		break;
	default:
		ExcelTrace("%d", (unsigned int)link);
		return;
	}

	TABLE_DATA_READ_LOCK(this);

	ASSERT_RETURN(link < m_Data.Count());

	BYTE * rowdata = ExcelGetDataFromRowData(m_Data[link]);
	ASSERT_RETURN(rowdata);
	const char * str = (const char *)(rowdata + desc->m_IndexFields[0]->m_nOffset);
	ExcelTrace("%s", str);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if EXCEL_TRACE
void EXCEL_TABLE::TraceTable(
	void)
{
	if (!m_bTrace)
	{
		return;
	}
	ASSERT_RETURN(m_Struct);

	const EXCEL_FIELD * fields[EXCEL_STRUCT::EXCEL_MAX_FIELDS];
	structclear(fields);

	ExcelTrace("\nEXCEL_TABLE::TraceTable()   TABLE: %s\n", m_Name);

	// build array of fields in defined order
	unsigned int fieldCount = 0;
	for (unsigned int ii = 0; ii < m_Struct->m_Fields.Count(); ++ii)
	{
		for (unsigned int jj = 0; jj < m_Struct->m_Fields.Count(); ++jj)
		{
			if (m_Struct->m_Fields[jj].m_nDefinedOrder == ii)
			{
				fields[ii] = &m_Struct->m_Fields[jj];
				fieldCount++;
			}
		}
	}

	// trace the header
	ExcelTrace("#");
	for (unsigned int ii = 0; ii < fieldCount; ++ii)
	{
		ExcelTrace(", ");
		ExcelTrace("%s", fields[ii]->m_ColumnName);
	}
	ExcelTrace("\n");

	// trace the data
	for (unsigned int ii = 0; ii < m_Data.Count(); ++ii)
	{
	#if EXCEL_FILE_LINE
		const BYTE * rowdata = (BYTE *)GetData(ii, __FILE__, __LINE__);
	#else
		const BYTE * rowdata = (BYTE *)GetData(ii);
	#endif

		ExcelTrace("%d", ii);
		for (unsigned int jj = 0; jj < fieldCount; ++jj)
		{
			ExcelTrace(", ");
			
			fields[jj]->TraceData(this, ii, rowdata);
		}
		ExcelTrace("\n");
	}
}
#endif


//----------------------------------------------------------------------------
// EXCEL:: FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL::EXCEL(
	void)
{
	EXCEL_LOCK_INIT(m_CS);

	m_fpScriptParseFunc = NULL;
	m_fpStatsListInit = NULL;
	m_fpStatsListFree = NULL;
	m_fpScriptExecute = NULL;
	m_fpStatsGetStat = NULL;
	m_fpStatsSetStat = NULL;

	m_StringTablesUpdateState = EXCEL_UPDATE_STATE_UNKNOWN;
	m_idUnitTypesTable = EXCEL_LINK_INVALID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL::Init(
	void)
{
	m_VersionAppDict = StrDictionaryInit(g_VersionAppTokens);
	m_VersionPackageFlagsDict = StrDictionaryInit(g_VersionPackageFlagTokens);
	m_VersionPackageDict = StrDictionaryInit(g_VersionPackageTokens);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EXCEL::Free(
	void)
{
	ExcelTrace("EXCEL::Free()\n"); 
	
	// free all tables
	for (unsigned int ii = 0; ii < m_Tables.Count(); ++ii)
	{
		if (m_Tables[ii])
		{
			m_Tables[ii]->Free();
			FREE(NULL, m_Tables[ii]);
		}
	}
	m_Tables.Destroy(NULL);

	// free all structs
	for (unsigned int ii = 0; ii < m_Structs.Count(); ++ii)
	{
		ASSERT_CONTINUE(m_Structs[ii]);
		m_Structs[ii]->Free();
		FREE(NULL, m_Structs[ii]);
	}
	m_Structs.Destroy(NULL);

	if (m_VersionAppDict)
	{
		StrDictionaryFree(m_VersionAppDict);
		m_VersionAppDict = NULL;
	}

	if (m_VersionPackageDict)
	{
		StrDictionaryFree(m_VersionPackageDict);
		m_VersionPackageDict = NULL;
	}

	if (m_VersionPackageFlagsDict)
	{
		StrDictionaryFree(m_VersionPackageFlagsDict);
		m_VersionPackageFlagsDict = NULL;
	}

	EXCEL_LOCK_FREE(m_CS);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_STRUCT * EXCEL::GetStructByName(
	const char * structname)
{
	ASSERT_RETNULL(structname);

	EXCEL_STRUCT key(structname);
	EXCEL_STRUCT ** ptr = m_Structs.FindExact(&key);
	if (!ptr)
	{	
		return NULL;
	}
	return *ptr;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_STRUCT * EXCEL::AddStruct(
	const EXCEL_STRUCT & estruct)
{
	EXCEL_STRUCT * es = (EXCEL_STRUCT *)MALLOCZ(NULL, sizeof(EXCEL_STRUCT));
	*es = estruct;

	m_Structs.Insert(NULL, es);

	return es;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_TABLE * EXCEL::RegisterTable(
	unsigned int idTable,
	const char * name,
	const char * structName,
	APP_GAME appGame,
	EXCEL_TABLE_SCOPE scope,
	unsigned int data1)
{
	ASSERT_RETNULL(idTable < EXCEL_MAX_TABLE_COUNT);
	ASSERT_RETNULL(name);
	ASSERT_RETNULL(structName);
	ASSERT_RETNULL(idTable >= m_Tables.Count() || m_Tables[idTable] == NULL)

	const EXCEL_STRUCT * estruct = Excel().GetStructByName(structName);
	ASSERT_RETNULL(estruct);

	BOOL isUnitTypesTable = (PStrICmp(name, UNITTYPES_TABLE) == 0);

	EXCEL_TABLE * etable = (EXCEL_TABLE *)MALLOCZ(NULL, sizeof(EXCEL_TABLE));
	etable->Init(idTable, name, estruct, appGame, scope, data1);

	m_Tables.OverwriteIndex(MEMORY_FUNC_FILELINE(NULL, idTable, etable));

	if (isUnitTypesTable)
	{
		m_idUnitTypesTable = idTable;
	}

	return etable;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int EXCEL::StringTablesGetUpdateState(
	void)
{
	return m_StringTablesUpdateState;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL::StringTablesLoadAll(void)
{
	// load tables only if the update state is unknown
	if (m_StringTablesUpdateState == EXCEL_UPDATE_STATE_UNKNOWN)
	{
		EXCEL_STRING_TABLES_LOAD * fpLoadFunction = Excel().m_fpStringTablesLoad;
		ASSERTV_RETFALSE( fpLoadFunction, "Expected load function for string tables" );
		
		EXCEL_UPDATE_STATE eResult = fpLoadFunction();
		if (eResult != EXCEL_UPDATE_STATE_ERROR)
		{
			m_StringTablesUpdateState = eResult;
		}
		
		return eResult != EXCEL_UPDATE_STATE_ERROR;

	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_TABLE * EXCEL::GetTable(
	unsigned int idTable) const
{
	ASSERT_RETNULL(idTable < EXCEL::EXCEL_MAX_TABLE_COUNT);

	ASSERT_RETNULL(idTable < m_Tables.Count());

	return m_Tables[idTable];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int EXCEL::GetTableCount(
	void) const
{
	return m_Tables.Count();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK EXCEL::GetTableIdByName(
	const char * name) const
{
	ASSERT_RETVAL(name, EXCEL_LINK_INVALID);

	unsigned int count = m_Tables.Count();

	for (unsigned int ii = 0; ii < count; ++ii)
	{
		EXCEL_TABLE * table = GetTable(ii);
		if (table && PStrICmp(name, table->m_Name) == 0)
		{
			return table->m_Index;
		}
	}

	return EXCEL_LINK_INVALID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL::InitManifest(
	const EXCEL_LOAD_MANIFEST & manifest)
{
	m_LoadManifest = manifest;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL::LoadDatatable(
	unsigned eTable)
{
	ASSERT_RETFALSE(eTable < m_Tables.Count());

	{
		EXCEL_TABLE * table = GetTable(eTable);
		if (table && table->AppLoadsTable())
		{
			table->Load();
		}
	}

	{
		EXCEL_TABLE * table = GetTable(eTable);
		if (table && table->AppLoadsTable())
		{
			if (table->m_Struct && table->m_Struct->m_fpPostProcessAll)
			{
				table->m_Struct->m_fpPostProcessAll(table);
			}
#if EXCEL_TRACE
			table->TraceTable();
#endif
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL::LoadAll(
	void)
{
	unsigned int count = m_Tables.Count();

	for (unsigned int ii = 0; ii < count; ++ii)
	{
		EXCEL_TABLE * table = GetTable(ii);
		if (table && table->AppLoadsTable())
		{
			table->Load();
		}
	}
	for (unsigned int ii = 0; ii < count; ++ii)
	
	{
		EXCEL_TABLE * table = GetTable(ii);
		if (table && table->AppLoadsTable())
		{
			if (table->m_Struct && table->m_Struct->m_fpPostProcessAll)
			{
				table->m_Struct->m_fpPostProcessAll(table);
			}
#if EXCEL_TRACE
			table->TraceTable();
#endif
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL::LoadTablePostProcessAll()
{
	unsigned int count = m_Tables.Count();
	for (unsigned int ii = 0; ii < count; ++ii)
	{
		EXCEL_TABLE * table = GetTable(ii);
		if (table != NULL && table->AppLoadsTable()) {
			if (table->m_Struct != NULL &&
				table->m_Struct->m_fpPostProcessAll) {
				table->m_Struct->m_fpPostProcessAll(table);
			}
#if EXCEL_TRACE
			table->TraceTable();
#endif
		}
	}
	return TRUE;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int EXCEL::GetFieldByName(
	unsigned int idTable,
	const char * fieldname) const
{
	ASSERT_RETINVALID(fieldname);
	ASSERT_RETINVALID(fieldname[0]);

	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETINVALID(table);

	EXCEL_FIELD key(fieldname);

	const EXCEL_FIELD * field = table->m_Struct->m_Fields.FindExact(key);
	ASSERT_RETINVALID(field);

	return (unsigned int)(field - table->m_Struct->m_Fields.GetPointer());
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int EXCEL::Lookup(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int idField,
	unsigned int row,
	unsigned int arrayindex)
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETINVALID(table);

	return table->Lookup(context, idField, row, arrayindex);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK EXCEL::GetLineByKey(
	unsigned int idTable,
	unsigned int idIndex,
	const void * key,
	unsigned int len) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);

	return table->GetLineByKey(idIndex, key, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK EXCEL::GetLineByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int idIndex,
	const void * key,
	unsigned int len,
	BOOL bExact) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);

	if (bExact)
	{
		return table->GetLineByKey(context, idIndex, key, len);
	}
	else
	{
		return table->GetClosestLineByKey(context, idIndex, key, len, bExact);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK EXCEL::GetNextLineByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int idIndex,	
	const void * key,
	unsigned int len) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);

	return table->GetNextLineByKey(context, idIndex, key, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK EXCEL::GetPrevLineByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int idIndex,
	const void * key,
	unsigned int len) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);

	return table->GetPrevLineByKey(context, idIndex, key, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * EXCEL::GetDataByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int idIndex,
	const void * key,
	unsigned int len) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetDataByKey(context, idIndex, key, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
EXCEL_LINK EXCEL::GetLineByStrKey(
	unsigned int idTable,
	T key) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetLineByStrKey<T>(key);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
EXCEL_LINK EXCEL::GetLineByStrKey(
	unsigned int idTable,
	unsigned int idIndex,
	T key) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetLineByStrKey<T>(idIndex, key);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
EXCEL_LINK EXCEL::GetLineByStrKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	T key) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetLineByStrKey<T>(context, key);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
EXCEL_LINK EXCEL::GetLineByStrKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int idIndex,
	T key) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetLineByStrKey<T>(context, idIndex, key);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
EXCEL_LINK EXCEL::GetNextLineByStrKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable, 
	unsigned int idIndex,
	T strkey) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetNextLineByStrKey<T>(context, idIndex, strkey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
EXCEL_LINK EXCEL::GetPrevLineByStrKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable, 
	unsigned int idIndex,
	T strkey) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetPrevLineByStrKey<T>(context, idIndex, strkey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
void * EXCEL::GetDataByStrKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	T key) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetDataByStrKey<T>(context, key);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T> 
T EXCEL::GetStringKeyByLine(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int idIndex,
	unsigned int row) const
{
	static T strNull = (T)("");
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETVAL(table, strNull);

	return table->GetStringKeyByLine<T>(context, idIndex, row);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T> 
T EXCEL::GetStringKeyByLine(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row) const
{
	static T strNull = (T)("");
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETVAL(table, strNull);

	return table->GetStringKeyByLine<T>(context, row);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * EXCEL::GetNameStringByLine(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row,
	GRAMMAR_NUMBER eNumber) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETVAL(table, sExcelStringTableLookupVerbose());

	return table->GetNameStringByLine(context, row, eNumber);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
EXCEL_LINK EXCEL::GetLineByCC(
	unsigned int idTable,
	unsigned int idIndex,
	T cc)
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	{
		TABLE_INDEX_READ_LOCK(table);
		return table->GetLineByCC<T>(idIndex, cc);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * EXCEL::GetData(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row
#if EXCEL_FILE_LINE
	,
	 const char * file,
	 unsigned int line
#endif 
	 )
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

#if EXCEL_FILE_LINE
	return table->GetData(context, row, file, line);
#else
	return table->GetData(context, row);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const void * EXCEL::GetDataEx(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row)
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetDataEx(context, row);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EXCEL::HasScript(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int field_offset,
	unsigned int row)
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->HasScript(context, field_offset, row);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE * EXCEL::GetScriptCode(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	PCODE code_offset,
	int * maxlen)
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetScriptCode(context, code_offset, maxlen);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int EXCEL::EvalScript(
	const EXCEL_CONTEXT & context,
	struct UNIT * subject,
	struct UNIT * object,
	struct STATS * statslist,
	unsigned int idTable,
	unsigned int field_offset,
	unsigned int row)
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETZERO(table);

	return table->EvalScript(context, subject, object, statslist, field_offset, row);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct STATS * EXCEL::GetStatsList(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetStatsList(context, row);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int EXCEL::GetLineByCode(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	DWORD cc)
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetLineByCode(context, cc);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * EXCEL::GetDataByCode(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	DWORD cc)
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetDataByCode(context, cc);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD EXCEL::GetCodeByLine(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row) const
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	return table->GetCodeByLine(context, row);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int EXCEL::GetCount(
	const EXCEL_CONTEXT & context,
	unsigned int idTable)
{
	REF(context);

	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETNULL(table);

	{
		TABLE_DATA_READ_LOCK(table);
		return table->m_Data.Count();
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if EXCEL_TRACE
void EXCEL::TraceLink(
	unsigned int idTable, 
	unsigned int idIndex, 
	EXCEL_LINK link)
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETURN(table);

	table->TraceLink(idIndex, link);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if EXCEL_TRACE
void EXCEL::TraceLinkTable(
	unsigned int idTable)
{
	EXCEL_TABLE * table = GetTable(idTable);
	ASSERT_RETURN(table);
	ASSERT_RETURN(table->m_Name);

	ExcelTrace("[%s]", table->m_Name);
}
#endif


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelCommonInit(
	void)
{
	Excel().Init();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelFree(
	void)
{
	Excel().Free();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterImports(
	EXCEL_SCRIPTPARSE * fpScriptParse,
	EXCEL_STATSLIST_INIT * fpStatsListInit,
	EXCEL_STATSLIST_FREE * fpStatsListFree,
	EXCEL_SCRIPT_EXECUTE * fpScriptExecute,
	EXCEL_STATS_GET_STAT * fpStatsGetStat,
	EXCEL_STATS_SET_STAT * fpStatsSetStat,
	EXCEL_STATS_GET_STAT_FLOAT * fpStatsGetStatFloat,
	EXCEL_STATS_SET_STAT_FLOAT * fpStatsSetStatFloat,
	EXCEL_STATS_WRITE_STATS * fpStatsWriteStats,
	EXCEL_STATS_READ_STATS * fpStatsReadStats,
	EXCEL_STATS_PARSE_TOKEN * fpStatsParseToken,
	EXCEL_SCRIPT_ADD_MARKER * fpScriptAddMarker,
	EXCEL_SCRIPT_WRITE_TO_PAK * fpScriptWriteToPak,
	EXCEL_SCRIPT_READ_FROM_PAK * fpScriptReadFromPak,
	EXCEL_SCRIPT_GET_IMPORT_CRC * fpScriptGetImportCRC,
	EXCEL_STRING_TABLES_LOAD * fpStringTablesLoad)
{
	ASSERT_RETURN(fpScriptParse);
	Excel().m_fpScriptParseFunc = fpScriptParse;
	Excel().m_fpStatsListInit = fpStatsListInit;
	Excel().m_fpStatsListFree = fpStatsListFree;
	Excel().m_fpScriptExecute = fpScriptExecute;
	Excel().m_fpStatsGetStat = fpStatsGetStat;
	Excel().m_fpStatsSetStat = fpStatsSetStat;
	Excel().m_fpStatsGetStatFloat = fpStatsGetStatFloat;
	Excel().m_fpStatsSetStatFloat = fpStatsSetStatFloat;
	Excel().m_fpStatsWriteStats = fpStatsWriteStats;
	Excel().m_fpStatsReadStats = fpStatsReadStats;
	Excel().m_fpStatsParseToken = fpStatsParseToken;
	Excel().m_fpScriptAddMarker = fpScriptAddMarker;
	Excel().m_fpScriptWriteToPak = fpScriptWriteToPak;
	Excel().m_fpScriptReadFromPak = fpScriptReadFromPak;
	Excel().m_fpScriptGetImportCRC = fpScriptGetImportCRC;
	Excel().m_fpStringTablesLoad = fpStringTablesLoad;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_TABLE * ExcelRegisterTable(
	unsigned int idTable,
	const char * structName,
	APP_GAME appGame,
	EXCEL_TABLE_SCOPE scope,
	const char * name,
	unsigned int data1)
{
	return Excel().RegisterTable(idTable, name, structName, appGame, scope, data1);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelTableAddFile(
	EXCEL_TABLE * table,
	const char * filename,
	APP_GAME appGame,
	EXCEL_TABLE_SCOPE scope,
	unsigned int data1)
{
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(filename);

	return table->AddFile(filename, appGame, scope, data1);
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelSetPostCreate(
	EXCEL_TABLE * table,
	const void * data, 
	unsigned int size)
{
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(table->m_Struct);
	ASSERT_RETFALSE(data);
	table->m_PostCreateData = data;
	table->m_PostCreateSize = size;
	table->m_bNoLoad = TRUE;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetTableNoLoad(
	EXCEL_TABLE * table)
{
	ASSERT_RETURN(table);
	table->m_bNoLoad = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetTableName(
	EXCEL_TABLE * table,
	const char * name,
	EXCEL_TABLE_SCOPE scope)
{
	ASSERT_RETURN(table);
	ASSERT_RETURN(name);
	table->m_Name = name;
	sExcelConstructFilename(table->m_Filename, table->m_FilenameCooked, arrsize(table->m_Filename), name, scope);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void ExcelSetTableTrace(
	EXCEL_TABLE * table)
{
	ASSERT_RETURN(table);
	table->m_bTrace = TRUE;;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_STRUCT * ExcelRegisterStructBegin(
	const char * name,
	unsigned int size)
{
	ASSERT_RETNULL(name);

	const EXCEL_STRUCT * estruct = Excel().GetStructByName(name);
	if (estruct)
	{
		return NULL;		// already registered a struct by this name
	}

	EXCEL_STRUCT excelStruct;
	excelStruct.m_Name = name;
	excelStruct.m_Size = size;

	return Excel().AddStruct(excelStruct);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterStructEnd(
	EXCEL_STRUCT * estruct)
{
	ASSERT_RETURN(estruct);
	estruct->RegisterEnd();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T>
void ExcelRegisterFieldBasicType(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	T defaultValue)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(count > 0);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = count;
	field.m_nElemSize = sizeof(T);
	field.SetDefaultValue((const void *)&defaultValue);

	excelStruct->AddField(field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T>
void ExcelRegisterFieldCC(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	const char * defaultValue)
{
	ASSERT_RETURN(excelStruct);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = 1;
	field.m_nElemSize = sizeof(T);
	field.SetDefaultValue((const void *)defaultValue);

	excelStruct->AddField(field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T>
void ExcelRegisterFieldLink(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(count > 0);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = count;
	field.m_nElemSize = sizeof(T);
	field.SetDefaultValue((const void *)defaultValue);
	field.SetLinkData(idTable, idIndex);
	field.m_dwFlags |= EXCEL_FIELD_ISLINK;

	excelStruct->AddField(field);
	excelStruct->m_Ancestors.Insert(NULL, idTable);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T>
void ExcelRegisterFieldDict(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	const STR_DICT * dictionary,
	const char * defaultValue,
	unsigned int max)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(dictionary);
	ASSERT_RETURN(count > 0);

	STR_DICTIONARY * strdict = StrDictionaryInit(dictionary, max);
	ASSERT_RETURN(strdict);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = count;
	field.m_nElemSize = sizeof(T);
	if (!defaultValue || defaultValue[0] == 0)
	{
		field.m_dwFlags = EXCEL_FIELD_PARSE_NULL_TOKEN;
	}
	BOOL found;
	StrDictionaryFind(strdict, "none", &found);
	if (found)
	{
		field.m_dwFlags |= EXCEL_FIELD_NONE_IS_NOT_ZERO;
	}
	field.SetDefaultValue((const void *)defaultValue);
	field.SetDictData(strdict, max);

	excelStruct->AddField(field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T>
void ExcelRegisterFieldString(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft,
	const T * name,
	unsigned int offset,
	unsigned int len,
	DWORD flags,
	const T * defaultValue)
{
	ASSERT_RETURN(excelStruct);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = len;
	field.m_nElemSize = sizeof(T);
	field.SetDefaultValue((const void *)defaultValue);
	field.SetDataFlag(flags);

	excelStruct->AddField(field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldFlag(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft,
	const char * name,
	unsigned int offset,
	unsigned int bit,
	BOOL defaultValue)
{
	ASSERT_RETURN(excelStruct);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = 1;
	field.m_nElemSize = bit / 8 + 1;
	field.SetDefaultValue((const void *)&defaultValue);
	field.SetFlagData(bit);

	excelStruct->AddField(field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldChar(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	char defaultValue)
{
	ExcelRegisterFieldBasicType<char>(excelStruct, eft, name, offset, 1, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldByte(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	BYTE defaultValue)
{
	ExcelRegisterFieldBasicType<BYTE>(excelStruct, eft, name, offset, 1, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldShort(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	short defaultValue)
{
	ExcelRegisterFieldBasicType<short>(excelStruct, eft, name, offset, 1, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldWord(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	WORD defaultValue)
{
	ExcelRegisterFieldBasicType<WORD>(excelStruct, eft, name, offset, 1, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldInt(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	int defaultValue)
{
	ExcelRegisterFieldBasicType<int>(excelStruct, eft, name, offset, 1, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldDword(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	DWORD defaultValue)
{
	ExcelRegisterFieldBasicType<DWORD>(excelStruct, eft, name, offset, 1, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldBool(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	BOOL defaultValue)
{
	ExcelRegisterFieldBasicType<BOOL>(excelStruct, eft, name, offset, 1, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldFloat(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	float defaultValue)
{
	ExcelRegisterFieldBasicType<float>(excelStruct, eft, name, offset, 1, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldFloatPercent(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	float defaultValue)
{
	ASSERT_RETURN(excelStruct);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = 1;
	field.m_nElemSize = sizeof(float);
	field.SetDefaultValue((const void *)&defaultValue);
	field.SetFloatMinMaxData(0.0f, 100.0f);

	excelStruct->AddField(field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldBytePercent(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	float defaultValue)
{
	ASSERT(defaultValue >= 0.0f && defaultValue <= 100.0f);
	int nPercent = (BYTE)(UCHAR_MAX * defaultValue / 100.0f  + 0.5);
	BYTE bytePercent = (BYTE)PIN(nPercent, 0, UCHAR_MAX);
	ExcelRegisterFieldBasicType<BYTE>(excelStruct, eft, name, offset, 1, bytePercent);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldFourCC(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	const char * defaultValue)
{
	ExcelRegisterFieldCC<DWORD>(excelStruct, eft, name, offset, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldThreeCC(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	const char * defaultValue)
{
	ExcelRegisterFieldCC<DWORD>(excelStruct, eft, name, offset, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldTwoCC(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	const char * defaultValue)
{
	ExcelRegisterFieldCC<WORD>(excelStruct, eft, name, offset, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldOneCC(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	const char * defaultValue)
{
	ExcelRegisterFieldCC<BYTE>(excelStruct, eft, name, offset, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldCharString(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft,
	const char * name,
	unsigned int offset,
	unsigned int len,
	DWORD flags,
	const char * defaultValue)
{
	ExcelRegisterFieldString<char>(excelStruct, eft, name, offset, len, flags, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldDynamicString(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft,
	const char * name,
	unsigned int offset,
	DWORD flags,
	const char * defaultValue)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_DYNAMIC_STRING);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = 1;
	field.m_nElemSize = sizeof(const char *);
	field.SetDefaultValue((const void *)defaultValue);
	field.SetDataFlag(flags, offset);

	excelStruct->AddField(field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldDynamicStringArrayElement(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft,
	const char * name,
	unsigned int offset,
	unsigned int offset64,
	DWORD flags,
	const char * defaultValue)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_DYNAMIC_STRING);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = 1;
	field.m_nElemSize = sizeof(const char *);
	field.SetDefaultValue((const void *)defaultValue);
	field.SetDataFlag(flags, offset64);

	excelStruct->AddField(field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldStringInt(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft,
	const char * name,
	unsigned int offset)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_STRINT);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = 1;
	field.m_nElemSize = sizeof(int);
	EXCEL_LINK defaultValue = EXCEL_LINK_INVALID;
	field.SetDefaultValue((const void *)&defaultValue);

	excelStruct->AddField(field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkChar(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_CHAR);
	ExcelRegisterFieldLink<char>(excelStruct, eft, name, offset, 1, idTable, idIndex, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkShort(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_SHORT);
	ExcelRegisterFieldLink<short>(excelStruct, eft, name, offset, 1, idTable, idIndex, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkInt(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_INT);
	ExcelRegisterFieldLink<int>(excelStruct, eft, name, offset, 1, idTable, idIndex, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkFourCC(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_FOURCC);
	ExcelRegisterFieldLink<DWORD>(excelStruct, eft, name, offset, 1, idTable, idIndex, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkThreeCC(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_THREECC);
	ExcelRegisterFieldLink<DWORD>(excelStruct, eft, name, offset, 1, idTable, idIndex, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkTwoCC(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_TWOCC);
	ExcelRegisterFieldLink<WORD>(excelStruct, eft, name, offset, 1, idTable, idIndex, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkOneCC(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_ONECC);
	ExcelRegisterFieldLink<BYTE>(excelStruct, eft, name, offset, 1, idTable, idIndex, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkTable(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	int defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_TABLE);
	ExcelRegisterFieldBasicType<int>(excelStruct, eft, name, offset, 1, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkStat(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int stat,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_STAT);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = 0;
	field.m_nCount = 1;
	field.m_nElemSize = 0;
	field.SetDefaultValue((const void *)defaultValue);
	field.SetLinkData(idTable, idIndex, stat);
	field.m_dwFlags |= EXCEL_FIELD_ISLINK;

	excelStruct->AddField(field);
	excelStruct->m_Ancestors.Insert(NULL, idTable);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldDictChar(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	STR_DICT * dict,
	unsigned int max,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_DICT_CHAR);
	ExcelRegisterFieldDict<char>(excelStruct, eft, name, offset, 1, dict, defaultValue, max);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldDictShort(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	STR_DICT * dict,
	unsigned int max,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_DICT_SHORT);
	ExcelRegisterFieldDict<short>(excelStruct, eft, name, offset, 1, dict, defaultValue, max);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldDictInt(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	const STR_DICT * dict,
	unsigned int max,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_DICT_INT);
	ExcelRegisterFieldDict<int>(excelStruct, eft, name, offset, 1, dict, defaultValue, max);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldDictIntArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	const STR_DICT * dict,
	unsigned int max,	
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_DICT_INT_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldDict<int>(excelStruct, eft, name, offset, count, dict, defaultValue, max);	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldCharArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	char defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_CHAR_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldBasicType<char>(excelStruct, eft, name, offset, count, defaultValue);
}

						
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldByteArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	BYTE defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_BYTE_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldBasicType<BYTE>(excelStruct, eft, name, offset, count, defaultValue);
}

						
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldShortArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	short defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_SHORT_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldBasicType<short>(excelStruct, eft, name, offset, count, defaultValue);
}
						

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldWordArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	WORD defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_WORD_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldBasicType<WORD>(excelStruct, eft, name, offset, count, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldIntArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	int defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_INT_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldBasicType<int>(excelStruct, eft, name, offset, count, defaultValue);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldDwordArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	DWORD defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_DWORD_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldBasicType<DWORD>(excelStruct, eft, name, offset, count, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldFloatArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	float defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_FLOAT_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldBasicType<float>(excelStruct, eft, name, offset, count, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkByteArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_BYTE_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldLink<BYTE>(excelStruct, eft, name, offset, count, idTable, idIndex, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkWordArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_WORD_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldLink<WORD>(excelStruct, eft, name, offset, count, idTable, idIndex, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkIntArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_INT_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldLink<int>(excelStruct, eft, name, offset, count, idTable, idIndex, defaultValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkFlagArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	unsigned int idTable,
	unsigned int idIndex)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_FLAG_ARRAY);
	ASSERT_RETURN(count >= 8);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = count;
	field.m_nElemSize = (count + 7)/8;
	field.SetLinkData(idTable, idIndex);
	field.m_dwFlags |= EXCEL_FIELD_ISLINK;

	excelStruct->AddField(field);
	excelStruct->m_Ancestors.Insert(NULL, idTable);
}

									 
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkFourCCArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_FOURCC_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldLink<DWORD>(excelStruct, eft, name, offset, count, idTable, idIndex, defaultValue);
}
						

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkThreeCCArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_THREECC_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldLink<DWORD>(excelStruct, eft, name, offset, count, idTable, idIndex, defaultValue);
}
						

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkTwoCCArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_TWOCC_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldLink<WORD>(excelStruct, eft, name, offset, count, idTable, idIndex, defaultValue);
}
						

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldLinkOneCCArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	unsigned int idTable,
	unsigned int idIndex,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_LINK_ONECC_ARRAY);
	ASSERT_RETURN(count > 0);
	ExcelRegisterFieldLink<BYTE>(excelStruct, eft, name, offset, count, idTable, idIndex, defaultValue);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetFieldAppFlags(
	DWORD &dwFlags,
	APP_GAME eAppGame)
{	
	if (eAppGame == APP_GAME_HELLGATE)
	{
		dwFlags |= EXCEL_FIELD_HELLGATE_ONLY;
	}
	else if (eAppGame == APP_GAME_TUGBOAT)
	{
		dwFlags |= EXCEL_FIELD_MYTHOS_ONLY;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldStrTableLink(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	enum APP_GAME appGame,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_STR_TABLE);
	ASSERT_RETURN(excelStruct);
	
	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = 1;
	field.m_nElemSize = sizeof(int);
	field.SetDefaultValue((const void *)defaultValue);
	sSetFieldAppFlags( field.m_dwFlags, appGame );

	excelStruct->AddField(field);
	excelStruct->m_usesStringTable = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldStrTableLinkArray(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int count,
	enum APP_GAME appGame,
	const char * defaultValue)
{
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_STR_TABLE_ARRAY);
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(count > 0);
	
	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = count;
	field.m_nElemSize = sizeof(int);
	field.SetDefaultValue((const void *)defaultValue);
	sSetFieldAppFlags( field.m_dwFlags, appGame );

	excelStruct->AddField(field);
	excelStruct->m_usesStringTable = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldFileid(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft,
	const char * name,
	unsigned int offset,
	const char * path,
	unsigned int pakid)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_FILEID || eft == EXCEL_FIELDTYPE_COOKED_FILEID);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = 1;
	field.m_nElemSize = sizeof(FILEID);
	field.SetFileIdData(path, (PAK_ENUM)pakid);

	excelStruct->AddField(field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldCode(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset)
{
	ASSERT_RETURN(excelStruct);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = 1;
	field.m_nElemSize = sizeof(PCODE);

	excelStruct->AddField(field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldDefCode(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_DEFCODE);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = 0;
	field.m_nCount = 1;
	field.m_nElemSize = 0;

	excelStruct->AddField(field);
	excelStruct->m_hasStatsList = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldStat(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	int wStat,
	int defaultValue,
	DWORD dwParam)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_DEFSTAT);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = 0;
	field.m_nCount = 1;
	field.m_nElemSize = 0;
	field.SetDefaultValue((const void *)&defaultValue);
	field.SetStatData(wStat, dwParam);

	excelStruct->AddField(field);
	excelStruct->m_hasStatsList = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldStatFloat(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	int wStat,
	float defaultValue,
	DWORD dwParam)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_DEFSTAT_FLOAT);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = 0;
	field.m_nCount = 1;
	field.m_nElemSize = 0;
	field.SetDefaultValue((const void *)&defaultValue);
	field.SetStatData(wStat, dwParam);

	excelStruct->AddField(field);
	excelStruct->m_hasStatsList = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldUnitTypes(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int size,
	const char * defaultValue)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_UNITTYPES);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nCount = size;
	field.m_nElemSize = sizeof(int);
	field.SetDefaultValue((const void *)defaultValue);

	excelStruct->AddField(field);
}
								 

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldParse(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset,
	unsigned int size,
	EXCEL_PARSEFUNC * fpParseFunc,
	int param)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_PARSE);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nElemSize = size;
	field.SetParseData(fpParseFunc, param);

	excelStruct->AddField(field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterFieldInheritRow(
	EXCEL_STRUCT * excelStruct,
	EXCEL_FIELDTYPE eft, 
	const char * name,
	unsigned int offset)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(eft == EXCEL_FIELDTYPE_INHERIT_ROW);
	ASSERT_RETURN(excelStruct->m_usesInheritRow == FALSE);

	EXCEL_FIELD field;
	field.m_ColumnName = name;
	field.m_Type = eft;
	field.m_nOffset = offset;
	field.m_nElemSize = sizeof(int);
	field.m_dwFlags = EXCEL_FIELD_INHERIT_ROW;

	excelStruct->AddField(field);
	excelStruct->m_usesInheritRow = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetFlags(
	EXCEL_STRUCT * excelStruct,
	DWORD dwFlags)
{
	ASSERT_RETURN(excelStruct);
	unsigned int fieldCount = excelStruct->m_Fields.Count();
	ASSERT_RETURN(fieldCount > 0);
	unsigned int lastDefinedField = fieldCount - 1;

	EXCEL_FIELD * field = NULL;
	for (unsigned int ii = 0; ii < fieldCount; ++ii)
	{
		if (excelStruct->m_Fields[ii].m_nDefinedOrder == lastDefinedField)
		{
			field = &excelStruct->m_Fields[ii];
			break;
		}
	}
	ASSERT_RETURN(field);
	field->m_dwFlags |= dwFlags;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetIndex(
	EXCEL_STRUCT * excelStruct,
	unsigned int index,
	DWORD flags,
	const char * field1,
	const char * field2,
	const char * field3)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(index < EXCEL_STRUCT::EXCEL_MAX_INDEXES);
	excelStruct->AddIndex(index, flags, field1, field2, field3);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelAddAncestor(
	EXCEL_STRUCT * excelStruct, 
	unsigned int idTable)
{
	ASSERT_RETURN(excelStruct);
	excelStruct->m_Ancestors.Insert(NULL, idTable);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetSelfIndex(
	EXCEL_STRUCT * excelStruct)
{
	ASSERT_RETURN(excelStruct);
	excelStruct->m_bSelfIndex = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetAllowDynamicStats(
	EXCEL_STRUCT * excelStruct)
{
	ASSERT_RETURN(excelStruct);
	excelStruct->m_hasStatsList = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetNameField(
	EXCEL_STRUCT * excelStruct,
	const char * name)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(name);
	excelStruct->SetNameField(name);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetIsAField(
	EXCEL_STRUCT * excelStruct,
	const char * name)
{
	ASSERT_RETURN(excelStruct);
	ASSERT_RETURN(name);
	excelStruct->SetIsAField(name);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetPostIndex(
	EXCEL_STRUCT * excelStruct,
	EXCEL_POSTPROCESS * fpPostProcess)
{
	ASSERT_RETURN(excelStruct);
	excelStruct->m_fpPostIndex = fpPostProcess;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetPostProcessRow(
	EXCEL_STRUCT * excelStruct,
	EXCEL_POSTPROCESS_ROW * fpPostProcess)
{
	ASSERT_RETURN(excelStruct);
	excelStruct->m_fpPostProcessRow = fpPostProcess;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetPostProcessTable(
	EXCEL_STRUCT * excelStruct,
	EXCEL_POSTPROCESS * fpPostProcess)
{
	ASSERT_RETURN(excelStruct);
	excelStruct->m_fpPostProcessTable = fpPostProcess;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetPostProcessAll(
	EXCEL_STRUCT * excelStruct,
	EXCEL_POSTPROCESS * fpPostProcess)
{
	ASSERT_RETURN(excelStruct);
	excelStruct->m_fpPostProcessAll = fpPostProcess;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetTableFree(
	EXCEL_STRUCT * excelStruct,
	EXCEL_POSTPROCESS * fpPostProcess)
{
	ASSERT_RETURN(excelStruct);
	excelStruct->m_fpTableFree = fpPostProcess;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetRowFree(
	EXCEL_STRUCT * excelStruct,
	EXCEL_ROW_FREE * fpRowFree)
{
	ASSERT_RETURN(excelStruct);
	excelStruct->m_fpRowFree = fpRowFree;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelSetMaxRows(
	EXCEL_STRUCT * excelStruct,
	unsigned int max)
{
	ASSERT_RETURN(excelStruct);
	excelStruct->m_nMaxRows = max;
}

					 
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ExcelScriptTest(
	void)
{
#if ISVERSION(DEVELOPMENT)
	EXCEL_SCRIPTPARSE * fpScriptParse = Excel().m_fpScriptParseFunc;
	ASSERT_RETURN(fpScriptParse);

	const char * scriptTest[] =
	{
		"feedcolorcode(stamina_feed)",
	};

	for (unsigned int ii = 0; ii < arrsize(scriptTest); ++ii)
	{
		char szErrorDesc[1024];
		PStrPrintf(szErrorDesc, 1024, "ExcelScriptTest: %s", scriptTest[ii]);

		BYTE buf[4096];
		BYTE * end = fpScriptParse(NULL, scriptTest[ii], buf, arrsize(buf), EXCEL_LOG, szErrorDesc);
		ASSERT_CONTINUE(end > buf);
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelInitManifest(
	const EXCEL_LOAD_MANIFEST & manifest)
{
	ASSERT_RETFALSE(Excel().InitManifest(manifest));

	ExcelInitReadManifest();

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelLoadData(
	void)
{
	ExcelTrace("ExcelLoadData()\n"); 

	ASSERT_RETFALSE(Excel().LoadAll());

	ExcelScriptTest();

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterStart(
	void)
{
#if EXCEL_MT
	Excel().m_CS.Enter();
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRegisterEnd(
	void)
{
#if EXCEL_MT
	Excel().m_CS.Leave();
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_TABLE * ExcelGetTableNotThreadSafe(		// not thread safe
	unsigned int idTable)
{
	EXCEL_TABLE * table = Excel().GetTable(idTable);
	ASSERT(table);
	return table;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int ExcelGetFieldByName(
	unsigned int idTable,
	const char * field)
{
	return Excel().GetFieldByName(idTable, field);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetLineByFourCC(
	unsigned int idTable,
	unsigned int idIndex,
	DWORD fourCC)
{
	return Excel().GetLineByCC<DWORD>(idTable, idIndex, fourCC);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetLineByThreeCC(
	unsigned int idTable,
	unsigned int idIndex,
	DWORD threeCC)
{
	return Excel().GetLineByCC<DWORD>(idTable, idIndex, threeCC);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetLineByTwoCC(
	unsigned int idTable,
	unsigned int idIndex,
	WORD twoCC)
{
	return Excel().GetLineByCC<WORD>(idTable, idIndex, twoCC);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetLineByOneCC(
	unsigned int idTable,
	unsigned int idIndex,
	BYTE oneCC)
{
	return Excel().GetLineByCC<BYTE>(idTable, idIndex, oneCC);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const void * _ExcelGetData(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row
#if EXCEL_FILE_LINE
	,
	 const char * file,
	 unsigned int line
#endif 
	)
{
	if (row == EXCEL_LINK_INVALID)
	{
		return NULL;
	}
#if EXCEL_FILE_LINE
	return (const void *)Excel().GetData(context, idTable, row, file, line);
#else
	return (const void *)Excel().GetData(context, idTable, row);
#endif 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * _ExcelGetDataNotThreadSafe(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row
#if EXCEL_FILE_LINE
	,
	 const char * file,
	 unsigned int line
#endif 
	)
{
	if (row == EXCEL_LINK_INVALID)
	{
		return NULL;
	}
#if EXCEL_FILE_LINE
	return (void *)Excel().GetData(context, idTable, row, file, line);
#else
	return (void *)Excel().GetData(context, idTable, row);
#endif 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const void * ExcelGetDataEx(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row)
{
	if (row == EXCEL_LINK_INVALID)
	{
		return NULL;
	}
	return Excel().GetDataEx(context, idTable, row);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ExcelGetCode(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row)
{
	return Excel().GetCodeByLine(context, idTable, row);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelHasScript(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int field_offset,
	unsigned int row)
{
	return Excel().HasScript(context, idTable, field_offset, row);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ExcelEvalScript(
	const EXCEL_CONTEXT & context,
	struct UNIT * subject,
	struct UNIT * object,
	struct STATS * statslist,
	unsigned int idTable,
	unsigned int field_offset,
	unsigned int row)
{
	return Excel().EvalScript(context, subject, object, statslist, idTable, field_offset, row);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct STATS * ExcelGetStats(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row)
{
	return Excel().GetStatsList(context, idTable, row);
}


//----------------------------------------------------------------------------
// returns the line given a key, exact match only
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetLineByStrKey(
	unsigned int idTable,
	const char * strkey)	
{
	ASSERT_RETVAL(strkey, EXCEL_LINK_INVALID);
	return Excel().GetLineByStrKey<const char *>(idTable, strkey);
}


//----------------------------------------------------------------------------
// returns the line given a key, exact match only
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetLineByStrKey(
	unsigned int idTable,
	unsigned int idIndex,
	const char * strkey)	
{
	return Excel().GetLineByStrKey<const char *>(idTable, idIndex, strkey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetLineByStringIndex(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	const char * strkey)
{
	ASSERT_RETVAL(strkey, EXCEL_LINK_INVALID);
	return Excel().GetLineByStrKey<const char *>(context, idTable, strkey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetLineByStringIndex(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int idIndex,
	const char * strkey)
{
	ASSERT_RETVAL(strkey, EXCEL_LINK_INVALID);
	return Excel().GetLineByStrKey<const char *>(context, idTable, idIndex, strkey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const void * ExcelGetDataByStringIndex(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	const char * strkey)
{
	ASSERT_RETNULL(strkey);
	return (const void *)Excel().GetDataByStrKey<const char *>(context, idTable, strkey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * ExcelGetDataByStringIndexNotThreadSafe(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	const char * strkey)
{
	ASSERT_RETNULL(strkey);
	return Excel().GetDataByStrKey<const char *>(context, idTable, strkey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * ExcelGetStringIndexByLine(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int idIndex,
	unsigned int row)
{
	return Excel().GetStringKeyByLine<const char *>(context, idTable, idIndex, row);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * ExcelGetStringIndexByLine(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row)
{
	return Excel().GetStringKeyByLine<const char *>(context, idTable, row);
}


//----------------------------------------------------------------------------
// returns the line given a key, exact match only
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetLineByKey(
	unsigned int idTable,
	unsigned int idIndex,
	const void * key,
	unsigned int len)	
{
	return Excel().GetLineByKey(idTable, idIndex, key, len);
}


//----------------------------------------------------------------------------
// returns the line given a key
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetLineByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	const void * key,
	unsigned int len,
	unsigned int idIndex,
	BOOL bExact)					// = TRUE
{
	return Excel().GetLineByKey(context, idTable, idIndex, key, len, bExact);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetNextLineByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	const void * key,
	unsigned int len,
	unsigned int idIndex)
{
	return Excel().GetNextLineByKey(context, idTable, idIndex, key, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetPrevLineByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	const void * key,
	unsigned int len,
	unsigned int idIndex)
{
	return Excel().GetPrevLineByKey(context, idTable, idIndex, key, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetNextLineByStrKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	const char * strkey,
	unsigned int idIndex)
{
	return Excel().GetNextLineByStrKey<const char *>(context, idTable, idIndex, strkey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetPrevLineByStrKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	const char * strkey,
	unsigned int idIndex)
{
	return Excel().GetPrevLineByStrKey<const char *>(context, idTable, idIndex, strkey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const void * ExcelGetDataByKey(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	const void * key,
	unsigned int len,
	unsigned int idIndex)
{
	ASSERT_RETNULL(key);
	return (const void*)Excel().GetDataByKey(context, idTable, idIndex, key, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const void * ExcelGetDataByKeyNotThreadSafe(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	const void * key,
	unsigned int len,
	unsigned int idIndex)
{
	ASSERT_RETNULL(key);
	return Excel().GetDataByKey(context, idTable, idIndex, key, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelTestIsA(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int a,
	unsigned int b)
{
	EXCEL_TABLE * table = Excel().GetTable(idTable);
	ASSERT_RETFALSE(table);
	return table->IsA(context, a, b);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE * ExcelGetScriptCode(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	PCODE code_offset,
	int * maxlen)
{
	return Excel().GetScriptCode(context, idTable, code_offset, maxlen);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelHasRowNameString(
	const EXCEL_CONTEXT & context,
	unsigned int idTable)
{
	REF(context);

	EXCEL_TABLE * table = Excel().GetTable(idTable);
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(table->m_Struct);
	return (table->m_Struct->m_NameField != NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * ExcelGetRowNameString(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	unsigned int row,
	GRAMMAR_NUMBER eNumber)
{
	return Excel().GetNameStringByLine(context, idTable, row, eNumber);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int ExcelGetCount(
	const EXCEL_CONTEXT & context,
	unsigned int idTable)
{
	return Excel().GetCount(context, idTable);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int ExcelGetLineByCode(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	DWORD cc)
{
	return Excel().GetLineByCode(context, idTable, cc);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const void * ExcelGetDataByCode(
	const EXCEL_CONTEXT & context,
	unsigned int idTable,
	DWORD cc)
{
	return (const void *)Excel().GetDataByCode(context, idTable, cc);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const EXCEL_TABLE_DEFINITION * ExcelTableGetDefinition(
	unsigned int idTable)
{
	return (const EXCEL_TABLE_DEFINITION *)ExcelGetData(EXCEL_CONTEXT(NULL), DATATABLE_EXCELTABLES, idTable);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ExcelTableGetCode(
	unsigned int idTable)
{
	const EXCEL_TABLE_DEFINITION * excel_table_definition = ExcelTableGetDefinition(idTable);
	ASSERT_RETNULL(excel_table_definition);
	return excel_table_definition->wCode;
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int ExcelTableGetByCode(
	DWORD dwCode)
{
	return ExcelGetLineByCode(EXCEL_CONTEXT(), DATATABLE_EXCELTABLES, dwCode);
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int ExcelGetTableCount(
	void)
{
	return Excel().GetTableCount();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ExcelGetVersionPackageByString(
	const TCHAR * pszString)
{
	char szConvertedString[ 128 ];
	PStrCvt( szConvertedString, pszString, 128 );
	return CAST_PTR_TO_INT(StrDictionaryFind( Excel().m_VersionPackageDict, szConvertedString, NULL ));
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ExcelGetVersionPackage()
{
	return Excel().m_LoadManifest.m_VersionPackage;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int ExcelGetCodeIndex(
	unsigned int idTable)
{
	EXCEL_TABLE * table = Excel().GetTable(idTable);
	if (table)
	{
		ASSERT_RETVAL(table->m_Struct, EXCEL_LINK_INVALID);
		return table->m_Struct->m_CodeIndex;
	}
	return EXCEL_LINK_INVALID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int ExcelGetCodeIndexSize(
	unsigned int idTable)
{
	EXCEL_TABLE * table = Excel().GetTable(idTable);
	ASSERT_RETZERO(table);
	ASSERT_RETZERO(table->m_Struct);
	if (table->m_Struct->m_CodeIndex >= EXCEL_STRUCT::EXCEL_MAX_INDEXES)
	{
		return 0;
	}

	return table->m_Indexes[table->m_Struct->m_CodeIndex].m_IndexDesc->m_IndexFields[0]->m_nElemSize;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_LINK ExcelGetLineByStringIndexPrivate(
	EXCEL_TABLE * table,
	const char * strkey)
{
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);
	ASSERT_RETVAL(strkey, EXCEL_LINK_INVALID);
	return table->GetLineByStrKeyNoLock<const char *>(strkey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * ExcelGetTableName(
	unsigned int idTable)
{
	EXCEL_TABLE * table = Excel().GetTable(idTable);
	if (table)
	{
		return table->m_Name;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * ExcelTableGetFilePath( 
	unsigned int idTable)
{
	EXCEL_TABLE * table = Excel().GetTable(idTable);
	if (table)
	{
		return table->m_Filename;
	}
	return NULL;
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int ExcelAppendTable(
	EXCEL_TABLE * dest_table,
	EXCEL_TABLE * src_table,
	unsigned int startrow)
{
	unsigned int endrow = startrow;

	ASSERT_RETVAL(dest_table, endrow);
	ASSERT_RETVAL(src_table, endrow);
	ASSERT_RETVAL(dest_table->m_Struct, endrow);
	ASSERT_RETVAL(src_table->m_Struct, endrow);
	ASSERT_RETVAL(src_table->m_Struct == dest_table->m_Struct, endrow);

	TABLE_DATA_WRITE_LOCK(src_table);

	ASSERT_RETVAL(src_table->m_DataEx == NULL, endrow);
	ASSERT_RETVAL(src_table->m_IsATable == NULL, endrow);
	ASSERT_RETVAL(src_table->m_CodeBuffer, endrow);
	ASSERT_RETVAL(dest_table->m_CodeBuffer, endrow);

	// append code sections		- note, if we allow table reload w/ multiple threads, this will become not-thread safe
	if (src_table->m_CodeBuffer->GetPos() != 0)
	{
		unsigned int src_size = src_table->m_CodeBuffer->GetPos() - 1;	// skip the 0 byte

		dest_table->m_CodeBuffer->AllocateSpace(src_size);
		if (dest_table->m_CodeBuffer->GetPos() == 0)
		{
			dest_table->m_CodeBuffer->Advance(1);
		}
		ASSERT_RETVAL(dest_table->m_CodeBuffer->GetPos() > 0, endrow);
		unsigned int src_offset = dest_table->m_CodeBuffer->GetPos() - 1;

		ASSERT_RETVAL(dest_table->m_CodeBuffer->GetRemaining() >= src_size, endrow);
		ASSERT_RETVAL(MemoryCopy(dest_table->m_CodeBuffer->GetCurBuf(), dest_table->m_CodeBuffer->GetRemaining(), src_table->m_CodeBuffer->GetBuf(1), src_size),endrow);
		dest_table->m_CodeBuffer->Advance(src_size);

		// fixup any code fields
		if (src_offset > 0)
		{
			for (unsigned int ii = 0; ii < src_table->m_Struct->m_Fields.Count(); ++ii)
			{
				const EXCEL_FIELD * field = &src_table->m_Struct->m_Fields[ii];
				if (field->m_Type == EXCEL_FIELDTYPE_CODE)
				{
					for (unsigned int jj = 0; jj < src_table->m_Data.Count(); ++jj)
					{
						EXCEL_DATA * node = &src_table->m_Data[jj];
						while (node)
						{
							BYTE * rowdata = ExcelGetDataFromRowData(*node);
							ASSERT_BREAK(rowdata);
							BYTE * fielddata = rowdata + field->m_nOffset;
							if (*(PCODE *)fielddata != NULL_CODE)
							{
								*(PCODE *)fielddata = *(PCODE *)fielddata + src_offset;
							}
							node = node->m_Prev;
						}
					}
				}
			}
		}

		// alias the source code buffer
		src_table->m_CodeBufferBase.Free();
		src_table->m_CodeBuffer = dest_table->m_CodeBuffer;
	}

	endrow = startrow + src_table->m_Data.Count();

	// copy row data & add to index
	for (unsigned int ii = 0, row = startrow; ii < src_table->m_Data.Count(); ++ii, ++row)
	{
		BYTE * rowdata = ExcelGetDataFromRowData(src_table->m_Data[ii]);
		ASSERT(rowdata);

		ASSERT(dest_table->AddRow(row));
		dest_table->m_Data[row].m_RowData = rowdata;
		dest_table->m_Data[row].m_Stats = src_table->m_Data[ii].m_Stats;
		dest_table->m_Data[row].m_dwFlags = 0;

		dest_table->AddRowToIndexes(row, rowdata);
	}

	dest_table->PostIndexLoad();

	return endrow;
}
	
//----------------------------------------------------------------------------
struct COOK_STRING_CALLBACK_DATA
{
	WRITE_BUF_DYNAMIC *					pWriteBuffer;
	BOOL								bCook;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCookString(
	const ADDED_STRING &tAdded,
	void *pCallbackData)
{
	COOK_STRING_CALLBACK_DATA *pCookStringCallbackData = (COOK_STRING_CALLBACK_DATA *)pCallbackData;
	
	if (pCookStringCallbackData->bCook)
	{
		WRITE_BUF_DYNAMIC *pWriteBuffer = pCookStringCallbackData->pWriteBuffer;
		ASSERTX_RETURN( pWriteBuffer, "Expected write buffer to cook string" );

		// push string into write buffer
		StringTableCommonWriteStringToPak( tAdded, pWriteBuffer );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelLoadStringTableDirectAndCook(
	int nLanguage,
	const char * filename,
	const char * filenameCooked,
	int nStringFileIndex,
	BOOL bCook)
{		
	// open cooked file for output
	DWORD dwCount = 0;	
	WRITE_BUF_DYNAMIC fbuf;
	DWORD fileposCount = 0;
	if (bCook)
	{
		FILE_HEADER hdr;
		hdr.dwMagicNumber = STRING_TABLE_MAGIC;
		hdr.nVersion = STRING_TABLE_VERSION;	
		ASSERT_RETFALSE(fbuf.PushInt(hdr.dwMagicNumber));
		ASSERT_RETFALSE(fbuf.PushInt(hdr.nVersion));
		fileposCount = (DWORD)fbuf.GetPos();
		ASSERT_RETFALSE(fbuf.PushInt(dwCount));
	}

	// setup callback data
	COOK_STRING_CALLBACK_DATA tCallbackData;
	tCallbackData.bCook = bCook;
	tCallbackData.pWriteBuffer = &fbuf;

	// convert path to wide chars
	WCHAR uszFilename[ MAX_PATH ];
	PStrCvt( uszFilename, filename, MAX_PATH );
	
	// load all strings in this file
	if (!StringTableCommonLoadTabDelimitedFile(nLanguage, uszFilename, nStringFileIndex, sCookString, &tCallbackData, &dwCount))
	{
		ASSERTV_RETFALSE( 0, "Error loading '%s'", filename );
	}			
		
	if (bCook)
	{
		unsigned int fileposEnd = fbuf.GetPos();
		fbuf.SetPos(fileposCount);
		ASSERT_RETFALSE(fbuf.PushInt(dwCount));
		ASSERT_RETFALSE(fbuf.SetPos(fileposEnd));

		UINT64 gentime = 0;
		ASSERT_RETFALSE(fbuf.Save(filenameCooked, NULL, &gentime));

		// copy file to history
		FileDebugCreateAndCompareHistory(filenameCooked);		
		DECLARE_LOAD_SPEC(spec, filenameCooked);
		spec.buffer = (void *)fbuf.GetBuf();
		spec.size = fbuf.GetPos();
		spec.gentime = gentime;
		spec.pakEnum = PAK_LOCALIZED;
		PakFileAddFile(spec);
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcelLoadStringTableFromPak(
	int nLanguage,
	const char * filename,
	int nStringFileIndex,
	BYTE * buf,
	unsigned int bufsize)
{
	REF(filename);

	if (AppCommonUsePakFile() == FALSE)
	{
		return FALSE;
	}
	
	BYTE_BUF bbuf(buf, bufsize);

	FILE_HEADER hdr;
	ASSERT_RETFALSE(bbuf.PopBuf(&hdr, sizeof(hdr)));
	if (!(hdr.dwMagicNumber == STRING_TABLE_MAGIC && hdr.nVersion == STRING_TABLE_VERSION))
	{
		return FALSE;
	}

	unsigned int count = 0;
	ASSERT_RETFALSE(bbuf.PopBuf(&count, sizeof(count)));
						
	// load all strings
	for (unsigned int ii = 0; ii < count; ++ii)
	{
		if (!StringTableCommonReadStringFromPak(nLanguage, nStringFileIndex, bbuf))
		{
			return FALSE;
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStringTableNeedsUpdate( 
	const char * filename,
	const char * filenameCooked)
{
	return FileNeedsUpdate(filenameCooked, filename, STRING_TABLE_MAGIC, STRING_TABLE_VERSION, FILE_UPDATE_UPDATE_IF_NOT_IN_PAK);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelCommonLoadStringTable(
	const char * file,
	int nStringFileIndex,
	BOOL bIsCommon,
	BOOL bForceDirect,
	int nLanguage,
	BOOL bAddToPak)
{
	char filename[MAX_PATH];
	PStrPrintf(filename, arrsize(filename), "%s%s", StringTableCommonGetStringFileGetExcelPath( bIsCommon ), file);
	PStrFixFilename(filename, arrsize(filename));

	char filenameCooked[MAX_PATH];
	PStrPrintf(filenameCooked, MAX_PATH, "%s.%s", filename, COOKED_FILE_EXTENSION);
	PStrFixFilename(filenameCooked, arrsize(filenameCooked));
	
	// see if the string table needs update
	BOOL bNeedUpdate = sStringTableNeedsUpdate(filename, filenameCooked);
	
	if (bForceDirect == TRUE || 
		bNeedUpdate || 
		AppCommonUsePakFile() == FALSE)
	{
loaddirect:

		// add to pak if app allows it
		BOOL bAddCookedToPak = AppCommonUsePakFile();
		if (bAddToPak == FALSE)
		{
			bAddCookedToPak = FALSE;  // nope, we have explicit instructions to not add to pak
		}
		
		if (!sExcelLoadStringTableDirectAndCook(nLanguage, filename, filenameCooked, nStringFileIndex, bAddCookedToPak))
		{
			return FALSE;
		}
	}
	else
	{
		// load from pak file
		DECLARE_LOAD_SPEC(spec, filenameCooked);
		spec.flags |= PAKFILE_LOAD_FORCE_FROM_PAK;
		spec.pakEnum = PAK_LOCALIZED;
		BYTE * buf = (BYTE *)PakFileLoadNow(spec);
		if (!buf)
		{
			return FALSE;
		}
		if (spec.frompak == FALSE)
		{
			if (buf)
			{
				FREE(NULL, buf);
			}
			goto loaddirect;
		}
		if (!sExcelLoadStringTableFromPak(nLanguage, filenameCooked, nStringFileIndex, buf, spec.bytesread))
		{
			FREE(NULL, buf);
			goto loaddirect;
		}
		FREE(NULL, buf);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelLoadDatatable(
	EXCELTABLE eTable)
{
	ASSERT_RETFALSE(Excel().LoadDatatable(eTable));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelStringTablesLoadAll(
	void)
{
	ASSERT_RETFALSE(Excel().StringTablesLoadAll());
	return TRUE;
}
