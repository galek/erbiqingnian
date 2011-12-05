//----------------------------------------------------------------------------
// dictionary.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#define DEFAULT_DICT_BUFFER_SIZE			16


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
typedef void FP_INT_DICT_DELETE(void * data);
typedef void FP_STR_DICT_DELETE(char * key, void * data);


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct STR_DICTIONARY;

struct STR_DICT
{
	const char *	str;
	int				value;
};


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
struct INT_DICTIONARY * IntDictionaryInit(
	int nBufferSize = DEFAULT_DICT_BUFFER_SIZE,
	FP_INT_DICT_DELETE * fpDelete = NULL);

void IntDictionaryFree(
	struct INT_DICTIONARY * dictionary);

void IntDictionarySort(
	struct INT_DICTIONARY * dictionary);

void IntDictionaryAdd(
	struct INT_DICTIONARY * dictionary,
	int key,
	void * data);

int IntDictionaryGetCount(
	INT_DICTIONARY * dictionary);

void * IntDictionaryGet(
	struct INT_DICTIONARY * dictionary,
	int index);

void * IntDictionaryFind(
	struct INT_DICTIONARY * dictionary,
	int key);

struct STR_DICTIONARY * StrDictionaryInit(
	int nBufferSize = DEFAULT_DICT_BUFFER_SIZE,
	BOOL bCopyKey = FALSE,
	FP_STR_DICT_DELETE* fpDelete = NULL);

struct STR_DICTIONARY * StrDictionaryInit(
	const STR_DICT * init_array,
	BOOL bCopyKey = FALSE,
	int offset = 0);

struct STR_DICTIONARY * StrDictionaryInit(
	const STR_DICT * init_array,
	unsigned int max_size,
	BOOL bCopyKey = FALSE,
	int offset = 0);

void StrDictionaryFree(
	struct STR_DICTIONARY * dictionary);

void StrDictionarySort(
	struct STR_DICTIONARY * dictionary);

void StrDictionaryAdd(
	struct STR_DICTIONARY * dictionary,
	const char * key,
	void * data);

int StrDictionaryGetCount(
	const STR_DICTIONARY * dictionary);

void * StrDictionaryGet(
	const STR_DICTIONARY * dictionary,
	int index);

const char * StrDictionaryGetStr(
	const STR_DICTIONARY * dictionary,
	int index);

void * StrDictionaryFind(
	STR_DICTIONARY * dictionary,
	const char * key,
	BOOL * found = NULL);

int StrDictionaryFindIndex(
	STR_DICTIONARY * dictionary,
	const char * key);

const char * StrDictionaryFindNameByValue(
	const STR_DICTIONARY * dictionary,
	void * data);

const char * StrDictGet(
	STR_DICT * dictionary,
	int index);

int StrDictFind(
	STR_DICT * dictionary,
	const char * key,
	BOOL bIgnoreCase = TRUE);

//#ifdef HAMMER
void StrDictionaryPopulateComboBox(
	struct STR_DICTIONARY * dictionary,
	class CComboBox * combobox);
//#endif


#endif // _DICTIONARY_H_