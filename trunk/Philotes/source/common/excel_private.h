//----------------------------------------------------------------------------
// excel_private.h
//----------------------------------------------------------------------------
#ifdef	_EXCEL_PRIVATE_H_
#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _EXCEL_PRIVATE_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _PAKFILE_H_
#include "pakfile.h"
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
#define EXCEL_TRACE								1
#else
#define EXCEL_TRACE								0
#endif

#if ISVERSION(DEVELOPMENT) && !ISVERSION(OPTIMIZED_VERSION)
#define	EXCEL_MT								0
#else
#define	EXCEL_MT								0
#endif

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#if EXCEL_TRACE
#define	ExcelTrace(fmt, ...)					trace(fmt, __VA_ARGS__)
#else
#define ExcelTrace(...)							NULLFUNC(__VA_ARGS__)
#endif

#define ExcelLog(fmt, ...)						LogMessage(EXCEL_LOG, fmt, __VA_ARGS__)


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
typedef int EXCEL_ROW_COMPARE(const struct EXCEL_INDEX_DESC & index, const BYTE * a, const BYTE * b);
typedef int EXCEL_KEY_COMPARE(const struct EXCEL_INDEX_DESC & index, const BYTE * fielddata, const BYTE * key);
typedef const BYTE * EXCEL_KEY_PREPROCESS(const struct EXCEL_INDEX_DESC & index, const BYTE * key);
typedef EXCEL_LINK EXCEL_TRANSLATE_LINK(struct EXCEL_TABLE * table, unsigned int row, const struct EXCEL_FIELD * field, const char * token);


//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum EXCEL_INDEXTYPE
{
	EXCEL_INDEXTYPE_INVALID,
	EXCEL_INDEXTYPE_DEFAULT,
	EXCEL_INDEXTYPE_STRING,
	EXCEL_INDEXTYPE_DYNAMIC_STRING,
	EXCEL_INDEXTYPE_STRINT,
	EXCEL_INDEXTYPE_FOURCC,
	EXCEL_INDEXTYPE_TWOCC,
	EXCEL_INDEXTYPE_ONECC,
	NUM_EXCEL_INDEXTYPE
};

enum EXCEL_TOK
{
	EXCEL_TOK_EOF,
	EXCEL_TOK_TOKEN,
	EXCEL_TOK_EOL
};

enum EXCEL_UPDATE_STATE
{	
	EXCEL_UPDATE_STATE_UNKNOWN,												// haven't processed this file yet
	EXCEL_UPDATE_STATE_EXAMINED,											// file has been looked at
	EXCEL_UPDATE_STATE_NEEDS_UPDATE,										// this file has been determined to require update
	EXCEL_UPDATE_STATE_ERROR,												// error while loading file
	EXCEL_UPDATE_STATE_TEXT_READ,											// text file has been read into memory
	EXCEL_UPDATE_STATE_INDEX_BUILT,											// index has been built
	EXCEL_UPDATE_STATE_UPDATED,												// file needed to be updated and has been fully updated
	EXCEL_UPDATE_STATE_UPTODATE,											// file was determined to not need update
	EXCEL_UPDATE_STATE_UPTODATE_INDEX_BUILT,								// cooked index was loaded
	EXCEL_UPDATE_STATE_UPTODATE_LOADED,										// cooked index was loaded
};

enum
{
	BYTEORDER_UTF8,
	BYTEORDER_UTF16_LITTLE,
	BYTEORDER_UTF16_BIG,
	BYTEORDER_UTF32_LITTLE,
	BYTEORDER_UTF32_BIG,
};

enum
{
	EXCEL_FIELD_ISINDEX	=						MAKE_MASK(0),
	EXCEL_FIELD_ISLINK	=						MAKE_MASK(1),
	EXCEL_FIELD_INHERIT_ROW	=					MAKE_MASK(2),
	EXCEL_FIELD_VERSION_ROW = 					MAKE_MASK(3),
	EXCEL_FIELD_PARSE_NULL_TOKEN =				MAKE_MASK(4),
	EXCEL_FIELD_IGNORE_VERSION =				MAKE_MASK(5),
	EXCEL_FIELD_HELLGATE_ONLY =					MAKE_MASK(6),
	EXCEL_FIELD_MYTHOS_ONLY =					MAKE_MASK(7),
	EXCEL_FIELD_NONE_IS_NOT_ZERO =				MAKE_MASK(8),
	EXCEL_FIELD_DICTIONARY_ALLOW_NEGATIVE =		MAKE_MASK(9),
	EXCEL_FIELD_DO_NOT_TRANSLATE =				MAKE_MASK(10),
};

enum
{
	EXCEL_HEADER_DYNAMIC =						0,							// field is dynamic (version, stats)
	EXCEL_HEADER_FREE =							1,							// field needs to be freed
};

enum
{
	EXCEL_DATA_FLAG_FREE =						MAKE_MASK(0),
};


//----------------------------------------------------------------------------
// INLINE
//----------------------------------------------------------------------------
inline int EXCEL_FIELD_COMPARE_BYNAME(
	const EXCEL_FIELD & a,
	const EXCEL_FIELD & b);

inline int EXCEL_STRUCT_COMPARE_BYNAME(
	EXCEL_STRUCT * const & a,
	EXCEL_STRUCT * const & b);


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct EXCEL_FIELD
{
	const char *								m_ColumnName;				// column heading in excel
	EXCEL_FIELDTYPE								m_Type;						// field type
	unsigned int								m_nOffset;					// offset of field in structure
	unsigned int								m_nCount;					// number of elements
	unsigned int								m_nElemSize;				// size of single element
	unsigned int								m_nDefinedOrder;			// ordinal for order in which the field was defined
	DWORD										m_dwFlags;					// field flags

	union
	{
		struct
		{
			SAFE_PTR(const char *,				str);
			unsigned int						strlen;
		};
		char									charval;
		BYTE									byte;
		short									shortval;
		WORD									word;
		int										intval;
		DWORD									dword;
		BOOL									boolean;
		float									floatval;
		DWORD									fourcc;
		WORD									twocc;
		BYTE									onecc;
		EXCEL_LINK								link;						// table link
	}											m_Default;
	union
	{
		struct 
		{
			DWORD								flags;						// string
			unsigned int						offset64;
		};
		unsigned int							bit;						// flags
		struct 
		{
			SAFE_PTR(const char *,				filepath);
			PAK_ENUM							pakid;
		};
		struct 
		{
			unsigned int						idTable;					// link
			unsigned int						idIndex;
			unsigned int						wLinkStat;					// link stat
		};
		struct
		{
			SAFE_PTR(struct STR_DICTIONARY *,	strdict);
			unsigned int						max;
		};
		struct																// float (not set if both == 0)
		{
			float								fmin;
			float								fmax;
		};
		struct
		{
			SAFE_PTR(EXCEL_PARSEFUNC *,			fpParseFunc);
			int									param;
		};
		struct																// stat
		{
			int									wStat;
			DWORD								dwParam;
		};
		struct
		{
//#if EXCEL_MT
			CReaderWriterLock_NS_WP				csStrIntDict;
//#endif
			SAFE_PTR(struct STRINT_DICT *,		strintdict);					// dictionary for strint
		};
	}											m_FieldData;

	EXCEL_FIELD(void) : m_ColumnName(NULL), m_Type(EXCEL_FIELDTYPE_INVALID), m_nOffset(0), m_nCount(0), m_dwFlags(FALSE) { structclear(m_Default); structclear(m_FieldData); }

	EXCEL_FIELD(const char * fieldname) : m_ColumnName(fieldname) {}		// use this only for constructing keys

	void Free(void);
	DWORD ComputeCRC(void) const;
	void InitDictField(char * str);
	void SetDefaultValue(const void * defaultValue);
	void SetDataFlag(DWORD flags, unsigned int offset64 = -1);
	void SetFlagData(unsigned int bit);
	void SetFloatMinMaxData(float min, float max);
	void SetFileIdData(const char * path, PAK_ENUM pakid);
	void SetLinkData(unsigned int in_table, unsigned int in_index, unsigned int in_stat = (unsigned int)EXCEL_LINK_INVALID);
	void SetDictData(STR_DICTIONARY * dictionary, unsigned int max);
	void SetParseData(EXCEL_PARSEFUNC * fp, int param);
	void SetStatData(int wStat, DWORD dwParam);
	BOOL IsLoadFirst(void) const;
	BOOL IsIndex(void) const { return (m_dwFlags & EXCEL_FIELD_ISINDEX) != 0; }

	template <typename T> void SetStringData(BYTE * fielddata, const T * src, unsigned int len) const;
	void SetDynamicStringData(EXCEL_TABLE * table, BYTE * fielddata, const char * src, unsigned int len) const;
	BOOL LoadDirectTranslateUnitTypes(struct EXCEL_TABLE * table, unsigned int row, BYTE * fielddata, const char * token) const;
	BOOL LoadDirectTranslateStringTableArray(struct EXCEL_TABLE * table, unsigned int row, BYTE * fielddata, const char * token) const;

	template <typename T> void SetDefaultDataArray(BYTE * rowdata, T defaultVal) const;
	void SetDefaultData(EXCEL_TABLE * table, unsigned int row, BYTE * rowdata) const;
	template <typename T> BOOL TestDefaultDataArray(const BYTE * rowdata, T defaultVal) const;
	BOOL IsBlank(struct EXCEL_TABLE * table, unsigned int row, const BYTE * fielddata) const;
	EXCEL_LINK LoadDirectTranslateLink(const struct EXCEL_TABLE * table, unsigned int row,const char * token) const;
	template <typename T> BOOL LoadDirectTranslateLinkArray(EXCEL_TABLE * table, unsigned int row, BYTE * fielddata, char * token, EXCEL_TRANSLATE_LINK fpTranslate, EXCEL_LINK max) const;
	template <typename T> BOOL LoadDirectTranslateBasicType(BYTE * fielddata, const char * token) const;
	template <typename T> BOOL LoadDirectTranslateBasicTypeArray(unsigned int row, BYTE * fielddata, char * token, T defaultVal) const;
	BOOL LoadDirectTranslate(struct EXCEL_TABLE * table, unsigned int row, BYTE * rowdata, char * token, unsigned int toklen) const;

	int Lookup(const EXCEL_CONTEXT & context, const struct EXCEL_TABLE * table, unsigned int row, const BYTE * fielddata, unsigned int arrayindex = 0) const;
	template <typename T> int LookupArray(const EXCEL_CONTEXT & context, const BYTE * fielddata, unsigned int arrayindex) const;

#if EXCEL_TRACE
	void TraceLink(const struct EXCEL_TABLE * table, EXCEL_LINK link) const;
	void TraceLinkTable(const struct EXCEL_TABLE * table, EXCEL_LINK link) const;
	void TraceDict(EXCEL_LINK link) const;
	template <typename T> void TraceDictArray(const struct EXCEL_TABLE * table, const BYTE * fielddata) const;
	template <typename T> void TraceBasicTypeArray(const BYTE * fielddata, const char * fmt) const;
	template <typename T> void TraceLinkArray(const struct EXCEL_TABLE * table, const BYTE * fielddata) const;
	void TraceData(const struct EXCEL_TABLE * table, unsigned int row, const BYTE * rowdata) const;
#endif
/*
	int						nLookupIndex;
	unsigned int			m_nMaxStrDictSize;
*/
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct EXCEL_INDEX_DESC
{
	static const unsigned int EXCEL_MAX_INDEX_FIELDS =			3;			// maximum number of index fields

	EXCEL_INDEXTYPE								m_Type;						// index type
	EXCEL_ROW_COMPARE *							m_fpRowCompare;				// row compare function
	EXCEL_KEY_PREPROCESS *						m_fpKeyPreprocess;			// prepare key for m_fpKeyCompare
	EXCEL_KEY_COMPARE *							m_fpKeyCompare;				// key compare function (2nd value is not full row)
	DWORD										m_dwFlags;
	EXCEL_FIELD *								m_IndexFields[EXCEL_MAX_INDEX_FIELDS];

	EXCEL_INDEX_DESC(void) : m_Type(EXCEL_INDEXTYPE_INVALID), m_fpRowCompare(NULL), m_fpKeyCompare(NULL), m_fpKeyPreprocess(NULL), m_dwFlags(0) { structclear(m_IndexFields); };
	void Free(void);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct EXCEL_STRUCT
{
	static const unsigned int EXCEL_MAX_FIELDS =				1024;		// maximum number of fields sanity check
	static const unsigned int EXCEL_FIELD_BUFSIZE =				16;			// bucket size for EXCEL_STRUCT::m_Fields
	static const unsigned int EXCEL_MAX_INDEXES =				4;			// maximum number of indexes on a table

	const char *								m_Name;						// we search on name
	unsigned int								m_Size;						// size of structure

	SORTED_ARRAYEX<
		EXCEL_FIELD, 
		EXCEL_FIELD_COMPARE_BYNAME, 
		EXCEL_FIELD_BUFSIZE>					m_Fields;
	const EXCEL_FIELD *							m_NameField;				// pointer to field which we designate as "name"
	const EXCEL_FIELD *							m_IsAField;					// construct isa table out of this field

	EXCEL_INDEX_DESC							m_Indexes[EXCEL_MAX_INDEXES];
	BOOL										m_isIndexed;				// has any index at all
	unsigned int								m_StringIndex;				// which index is the string index
	unsigned int								m_CodeIndex;				// which index is the code index
	unsigned int								m_nMaxRows;					// maximum number of rows in table

	BOOL										m_usesStringTable;			// does this structure use stringtables
	BOOL										m_hasStatsList;				// structure requires default stats list
	BOOL										m_hasScriptField;			// structure has a script field
	BOOL										m_usesInheritRow;			// inherit row

	SORTED_ARRAY<
		unsigned int, 
		EXCEL_FIELD_BUFSIZE>					m_Ancestors;				// array of idTables which i directly need
	SORTED_ARRAY<
		unsigned int, 
		EXCEL_FIELD_BUFSIZE>					m_Dependents;				// array of idTables which depend on me
	BOOL										m_bSelfIndex;				// set this flag if the table that uses this struct will reference itself 
																			// (used if multiple tables use the same struct)

	EXCEL_POSTPROCESS *							m_fpPostIndex;				// run after building index for table
	EXCEL_POSTPROCESS_ROW *						m_fpPostProcessRow;			// run after reading each row
	EXCEL_POSTPROCESS *							m_fpPostProcessTable;		// run after reading all data for table
	EXCEL_POSTPROCESS *							m_fpPostProcessAll;			// run after reading all tables
	EXCEL_POSTPROCESS *							m_fpTableFree;				// run when freeing table
	EXCEL_ROW_FREE *							m_fpRowFree;				// run when freeing a row

	DWORD										m_CRC32;

	EXCEL_STRUCT(void) : m_Name(NULL), m_Size(0), m_NameField(NULL), m_IsAField(NULL), m_isIndexed(FALSE), 
		m_StringIndex(EXCEL_LINK_INVALID), m_CodeIndex(EXCEL_LINK_INVALID), m_nMaxRows(UINT_MAX),
		m_usesStringTable(FALSE), m_hasStatsList(FALSE), m_hasScriptField(FALSE), m_usesInheritRow(FALSE), 
		m_fpPostProcessRow(NULL), m_fpPostProcessTable(NULL), m_fpPostProcessAll(NULL), m_fpPostIndex(NULL),
		m_fpTableFree(NULL), m_fpRowFree(NULL), m_CRC32(0)
	{ 
		m_Fields.Init(); 
		structclear(m_Indexes); 
		m_Ancestors.Init(); 
		m_Dependents.Init();
	}

	EXCEL_STRUCT(const char * key) : m_Name(key) {};						// use only for by name searches

	void Free(void);
	void AddField(EXCEL_FIELD & field);
	EXCEL_FIELD * GetFieldByName(const char * fieldname);
	const EXCEL_FIELD * GetFieldByNameConst(const char * fieldname, unsigned int strlen) const;
	void AddIndex(unsigned int idIndex, DWORD flags, const char * fieldname1, const char * fieldname2 = NULL, const char * fieldname3 = NULL);
	void SetNameField(const char * fieldname);
	void SetIsAField(const char * fieldname);
	void RegisterEnd(void);
	void ComputeCRC(void);
};


//----------------------------------------------------------------------------
// used to store a list of the fields in a text file in the order presented in the .txt
//----------------------------------------------------------------------------
struct EXCEL_HEADER
{
	const EXCEL_FIELD *							m_Field;
	DWORD										m_dwFieldFlags;

	EXCEL_HEADER() : m_Field(NULL), m_dwFieldFlags(0) {};
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct EXCEL_TEXTFIELD
{
	char *										m_Token;
	unsigned int								m_Toklen;
	const EXCEL_FIELD *							m_Field;
	DWORD										m_dwFieldFlags;

	EXCEL_TEXTFIELD(char * token, unsigned int toklen, const EXCEL_FIELD * field, DWORD flags) : m_Token(token), m_Toklen(toklen), m_Field(field), m_dwFieldFlags(flags) {};
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct EXCEL_TEXTFIELD_LINE
{
	static const unsigned int EXCEL_TEXTFIELD_BUFSIZE =				16;

	unsigned int								m_CurFile;
	SIMPLE_ARRAY<EXCEL_TEXTFIELD, 32>			m_Fields;

	EXCEL_TEXTFIELD_LINE(void) : m_CurFile(0) { m_Fields.Init(); };
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct EXCEL_INDEX_NODE
{
	BYTE *										m_RowDataEx;				// pointer to structure data for row
	unsigned int								m_RowIndex;					// row index

	EXCEL_INDEX_NODE(unsigned int row, BYTE * data) : m_RowDataEx(data), m_RowIndex(row) {}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct EXCEL_INDEX
{
	static const unsigned int EXCEL_INDEX_BUFSIZE =					64;		// buffer size for excel index

	const EXCEL_INDEX_DESC *					m_IndexDesc;				// pointer to index description in m_Struct

	SIMPLE_ARRAY<
		EXCEL_INDEX_NODE, 
		EXCEL_INDEX_BUFSIZE>					m_IndexNodes;

	void Init(const EXCEL_INDEX_DESC * desc);
	void Free(void);
	BOOL IsBlank(struct EXCEL_TABLE * table, unsigned int row, const BYTE * data) const;
	BOOL AddIndexNode(unsigned int row, BYTE * rowdata, BYTE * data, unsigned int * duperow = NULL);
	void Sort(void);
	const EXCEL_INDEX_NODE * Find(const BYTE * key) const;
	const EXCEL_INDEX_NODE * FindClosest(const BYTE * key, BOOL & bExact) const;
	unsigned int FindClosestIndex(const BYTE * key, BOOL & bExact) const;
	const EXCEL_INDEX_NODE * FindByStrKey(const char * key) const;			
	const EXCEL_INDEX_NODE * FindClosestByStrKey(const char * key, BOOL & bExact) const;			
	template <typename T>const EXCEL_INDEX_NODE * FindByCC(const EXCEL_TABLE * table, T cc) const;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#pragma pack(push, 16)
struct EXCEL_DATA_VERSION
{
	static const WORD EXCEL_MAX_VERSION =		USHRT_MAX;

	DWORD										m_VersionApp;
	DWORD										m_VersionPackage;
	WORD										m_VersionMajorMin;
	WORD										m_VersionMajorMax;
	WORD										m_VersionMinorMin;
	WORD										m_VersionMinorMax;

	EXCEL_DATA_VERSION(void) : m_VersionApp(EXCEL_APP_VERSION_DEFAULT), m_VersionPackage(EXCEL_PACKAGE_VERSION_DEFAULT), 
		m_VersionMajorMin(0), m_VersionMajorMax(EXCEL_MAX_VERSION), m_VersionMinorMin(0), m_VersionMinorMax(EXCEL_MAX_VERSION) {  }

	void Init(void) 
	{ 
		m_VersionApp = EXCEL_APP_VERSION_DEFAULT;
		m_VersionPackage = EXCEL_PACKAGE_VERSION_DEFAULT; 
		m_VersionMajorMin = 0; m_VersionMajorMax = EXCEL_MAX_VERSION;  
		m_VersionMinorMin = 0; m_VersionMinorMax = EXCEL_MAX_VERSION;
	}
};
#pragma pack(pop)


//----------------------------------------------------------------------------
// actual rowdata for excel tables
//----------------------------------------------------------------------------
struct EXCEL_DATA
{
	EXCEL_DATA *								m_Prev;
	BYTE *										m_RowData;					// points to version followed by the rowdata if EXCEL_DATA_FLAG_HASVERSION, or just rowdata otherwise
	struct STATS *								m_Stats;
	DWORD										m_dwFlags;					// TRUE if we free m_Data FALSE if it doesn't get freed

	EXCEL_DATA(BYTE * in_Data, BOOL in_bFree) : m_RowData(in_Data), m_Stats(NULL), m_dwFlags(in_bFree ? EXCEL_DATA_FLAG_FREE : 0), m_Prev(NULL) {}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T, unsigned int SIZE>
struct EXCEL_DSTRING_BUCKET
{
	T *											m_Strings;
	unsigned int								m_Size;
	unsigned int								m_Used;
	EXCEL_DSTRING_BUCKET<T, SIZE> *				m_Next;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T, unsigned int SIZE>
struct EXCEL_DSTRING_POOL
{
	EXCEL_DSTRING_BUCKET<T, SIZE> *				m_StringBucket;

	const T * AddString(const T * str);
	const T * AddString(const T * str, unsigned int len);
	T * GetSpace(unsigned int len);
	void Init(void) { m_StringBucket = NULL; };
	void Free(void);
	unsigned int GetStringOffset(const T * str);
	BOOL WriteToBuffer(WRITE_BUF_DYNAMIC & fbuf) const;
	T * GetReadBuffer(unsigned int & size) const;
	BOOL ReadFromBuffer(BYTE_BUF & fbuf);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct EXCEL_TABLE_FILE
{
	static const unsigned int EXCEL_HEADER_BUFSIZE =				32;		// bucket size for EXCEL_HEADER

	const char *								m_Name;
	APP_GAME									m_AppGame;					// game enum which uses this table
	EXCEL_TABLE_SCOPE							m_Scope;					// common shared or private to a game
	unsigned int								m_Data1;					// parameter
	char										m_Filename[MAX_PATH];
	char										m_FilenameCooked[MAX_PATH];
	BOOL										m_bLoad;

	// used only during parsing
	char *										m_TextBuffer;				// contents of text file for direct load (temporary)
	DWORD										m_TextBufferSize;
	char *										m_TextDataStart;			// after parsing header, this points to data start

	SIMPLE_ARRAY<
		EXCEL_HEADER, 
		EXCEL_HEADER_BUFSIZE>					m_TextHeaders;				// used during parsing, list of fields in the order they appear in the raw data
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct EXCEL_TABLE
{
	static const unsigned int EXCEL_DATA_BUFSIZE =					64;		// bucket size for EXCEL_DATA array
	static const unsigned int EXCEL_DATA_MAXROWS =					65536;	// sanity check on maximum number of rows
	static const unsigned int EXCEL_CODE_BUFSIZE =					16384;	// bucket size for code buffer
	static const unsigned int EXCEL_DSTRING_BUFSIZE =				16384;	// bucket size for string buffer

	// start table lock section   (this section should lock using m_CS for read/write)
//#if EXCEL_MT
	CReaderWriterLock_NS_WP						m_CS_Data;
//#endif

	SIMPLE_ARRAY<
		EXCEL_DATA, 
		EXCEL_DATA_BUFSIZE>						m_Data;						// table data
	EXCEL_DSTRING_POOL<
		char, 
		EXCEL_DSTRING_BUFSIZE>					m_DStrings;					// dynamic strings
	BUF_DYNAMIC<BYTE, EXCEL_CODE_BUFSIZE>		m_CodeBufferBase;			// script code
	BUF_DYNAMIC<BYTE, EXCEL_CODE_BUFSIZE> *		m_CodeBuffer;
	BYTE *										m_DataEx;
	unsigned int								m_nDataExRowSize;
	unsigned int								m_nDataExCount;
	DWORD *										m_IsATable;					
	unsigned int								m_IsARowSize;
	unsigned int								m_IsARowCount;

//#if EXCEL_MT
	CReaderWriterLock_NS_WP						m_CS_Index;
//#endif

	EXCEL_INDEX									m_Indexes[EXCEL_STRUCT::EXCEL_MAX_INDEXES];
	BYTE *										m_Key;						// buffer for key constructed for searches
	// end table lock section

	const char *								m_Name;						// table name
	char										m_Filename[MAX_PATH];		// basic table name
	char										m_FilenameCooked[MAX_PATH];	
	unsigned int								m_Index;					// table index

	const EXCEL_STRUCT *						m_Struct;					// pointer to excel struct

	BOOL										m_bNoLoad;					// set to TRUE if this table doesn't get loaded from file
	BOOL										m_bSelfIndex;				// links to self, so need to build index before parsing row data
	BOOL										m_bHasVersion;				// has a version column
	SIMPLE_ARRAY<
		const BYTE *, 
		1>										m_DataToFree;				// cooked data to free on destroy

	SIMPLE_ARRAY<
		EXCEL_TABLE_FILE,
		1>										m_Files;					// data files
	BOOL										m_bInPak;					// is the cooked file in the pakfile?
	UINT64										m_GenTime;					// cooked file write time

	// used only during parsing
	unsigned int								m_CurFile;					// current file

	unsigned int								m_UpdateState;				// state of table
	BOOL										m_bRowsAdded;				// set to true when data rows have been added

	const EXCEL_FIELD *							m_InheritField;				// inherit row field

	SIMPLE_ARRAY<
		EXCEL_TEXTFIELD_LINE,
		EXCEL_DATA_BUFSIZE>						m_TextFields;				// used by inherit row tables, created as a preprocess

#if ISVERSION(DEVELOPMENT)
	BOOL										m_bTrace;
#endif
	const void *								m_PostCreateData;
	unsigned int								m_PostCreateSize;

	void Init(unsigned int idTable, const char * name, const EXCEL_STRUCT * estruct, APP_GAME appGame, EXCEL_TABLE_SCOPE scope, unsigned int data1);
	void Free(void);
	void ConstructFilename(EXCEL_TABLE_FILE & file);
	BOOL AddFile(const char * name, APP_GAME appGame, EXCEL_TABLE_SCOPE scope, unsigned int data1);
	BOOL AppLoadsTable(void);
	BOOL NeedsUpdate(void);
	void PostIndexLoad(void);
	void PushRow(unsigned int index);
	BOOL AddRow(unsigned int index);
	BOOL AddRowIfUndefined(unsigned int index);
	BOOL AddRowToIndexes(unsigned int row, BYTE * rowdata);
	void FreeParseData(void);
	BOOL LoadDirectTranslateInheritRow(void);
	void LoadDirectReadOutLine(char * & cur);
	BOOL LoadDirectTranslateDataLine(unsigned int row);
	BOOL LoadDirectTranslateData(void);
	EXCEL_FIELD * LoadDirectTranslateHeaderCreateDynamicField(EXCEL_HEADER & header, EXCEL_FIELDTYPE type, const char * token, unsigned int toklen);
	BOOL LoadDirectTranslateHeaderVersion(EXCEL_HEADER & header, const char * token, unsigned int toklen);
	BOOL LoadDirectTranslateHeaderDynamicStats(EXCEL_HEADER & header, const char * token, unsigned int toklen);
	BOOL LoadDirectTranslateHeader(EXCEL_TABLE_FILE & file);
	BOOL LoadDirectIndexLine(unsigned int row);
	BOOL LoadCookedIndexes(void);
	BOOL LoadDirectIndexes(void);
	BOOL LoadIndexes(void);
	BYTE * GetFieldData(unsigned int row, const EXCEL_FIELD * field);
	BOOL GetInheritToken(unsigned int row, const EXCEL_FIELD * field, char * & token, unsigned int & toklen);
	void RemoveTrailingBlankLines(EXCEL_TABLE_FILE & file);
	BOOL LoadDirectBuildTextFields(EXCEL_TABLE_FILE & file);
	BOOL LoadDirectText(void);
	BOOL LoadDirect(void);
	BOOL SaveCooked(void);
	BOOL LoadFromBuffer(BYTE * buffer, unsigned int buflen);
	BOOL LoadFromCookedDirect(void);
	BOOL LoadFromPak(void);
	BOOL Load(void);
	BOOL CreateIsATable(void);

	BYTE * GetCodeBufferSpace(unsigned int & len);
	PCODE SetCodeBuffer(const BYTE * end);
	struct STATS * GetOrCreateStatsList(unsigned int row);
	
#if EXCEL_FILE_LINE
	BYTE * GetData(unsigned int row, const char * file, unsigned int line) const;
	void * GetData(const EXCEL_CONTEXT & context, unsigned int row, const char * file, unsigned int line) const;
#else
	BYTE * GetData(unsigned int row) const;
	void * GetData(const EXCEL_CONTEXT & context, unsigned int row) const;
#endif
	const void * GetDataEx(const EXCEL_CONTEXT & context, unsigned int row) const;
	EXCEL_LINK GetLineByKey(unsigned int idIndex, const void * key, unsigned int len)  const;
	EXCEL_LINK GetLineByKey(const EXCEL_CONTEXT & context, unsigned int idIndex, const void * key, unsigned int len) const;
	EXCEL_LINK GetClosestLineByKey(const EXCEL_CONTEXT & context, unsigned int idIndex, const void * key, unsigned int len, BOOL & bExact) const;
	EXCEL_LINK GetNextLineByKey(const EXCEL_CONTEXT & context, unsigned int idIndex, const void * key, unsigned int len) const;
	EXCEL_LINK GetPrevLineByKey(const EXCEL_CONTEXT & context, unsigned int idIndex, const void * key, unsigned int len) const;
	void * GetDataByKey(const EXCEL_CONTEXT & context, unsigned int idIndex, const void * key, unsigned int len) const;
	template <typename T> EXCEL_LINK GetLineByStrKeyNoLock(T strkey) const;
	template <typename T> EXCEL_LINK GetLineByStrKey(T strkey) const;
	template <typename T> EXCEL_LINK GetLineByStrKey(unsigned int idIndex, T strkey) const;
	template <typename T> EXCEL_LINK GetLineByStrKey(const EXCEL_CONTEXT & context, T strkey) const;
	template <typename T> EXCEL_LINK GetLineByStrKey(const EXCEL_CONTEXT & context, unsigned int idIndex, T strkey) const;
	template <typename T> EXCEL_LINK GetNextLineByStrKey(const EXCEL_CONTEXT & context, unsigned int idIndex, T strkey) const;
	template <typename T> EXCEL_LINK GetPrevLineByStrKey(const EXCEL_CONTEXT & context, unsigned int idIndex, T strkey) const;
	template <typename T> void * GetDataByStrKey(const EXCEL_CONTEXT & context, T strkey) const;
	template <typename T> T GetStringKeyByLine(const EXCEL_CONTEXT & context, unsigned int idIndex, unsigned int row) const;
	template <typename T> T GetStringKeyByLine(const EXCEL_CONTEXT & context, unsigned int row) const;
	const WCHAR * GetNameStringByLine(const EXCEL_CONTEXT & context, unsigned int row, GRAMMAR_NUMBER eNumber) const;
	template <typename T> EXCEL_LINK GetLineByCC(unsigned int idIndex, T cc);
	BYTE * GetScript(const EXCEL_CONTEXT & context, unsigned int field_offset, unsigned int row, unsigned int * maxlen = NULL);
	BYTE * GetScriptNoLock(const EXCEL_CONTEXT & context, unsigned int field_offset, unsigned int row, unsigned int * maxlen = NULL) const;
	BOOL HasScript(const EXCEL_CONTEXT & context, unsigned int field_offset, unsigned int row);
	int EvalScript(const EXCEL_CONTEXT & context, struct UNIT * subject, struct UNIT * object, struct STATS * statslist, unsigned int field_offset, unsigned int row);
	int EvalScriptNoLock(const EXCEL_CONTEXT & context, struct UNIT * subject, struct UNIT * object, struct STATS * statslist, unsigned int field_offset, unsigned int row) const;
	BYTE * GetScriptCode(const EXCEL_CONTEXT & context, PCODE code_offset, int * maxlen);
	unsigned int GetLineByCode(const EXCEL_CONTEXT & context, DWORD cc);
	void * GetDataByCode(const EXCEL_CONTEXT & context, DWORD cc);
	DWORD GetCodeByLine(const EXCEL_CONTEXT & context, unsigned int row) const;
	struct STATS * GetStatsList(unsigned int row) const;
	struct STATS * GetStatsList(const EXCEL_CONTEXT & context, unsigned int row) const;
	int Lookup(const EXCEL_CONTEXT & context, unsigned int idField, unsigned int row, unsigned int arrayindex = 0);
	BOOL IsA(const EXCEL_CONTEXT & context, unsigned int a, unsigned int b) const;

#if EXCEL_TRACE
	void TraceLink(unsigned int idIndex, EXCEL_LINK link) const;
	void TraceTable(void);
#endif
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct EXCEL
{
	static const unsigned int EXCEL_STRUCT_BUFSIZE =			16;			// bucket size for EXCEL::m_Structs
	static const unsigned int EXCEL_MAX_TABLE_COUNT =			512;		// sanity check on table index

//#if EXCEL_MT
	CCriticalSection							m_CS;						// global lock for reading tables
//#endif

	SORTED_ARRAYEX<
		EXCEL_STRUCT *, 
		EXCEL_STRUCT_COMPARE_BYNAME, 
		EXCEL_STRUCT_BUFSIZE>					m_Structs;					// array of excel struct pointers
	SIMPLE_ARRAY<
		EXCEL_TABLE *,
		EXCEL_STRUCT_BUFSIZE>					m_Tables;					// array of data table pointers

	unsigned int								m_idUnitTypesTable;			// used for UNITTYPES lookup

	EXCEL_SCRIPTPARSE *							m_fpScriptParseFunc;
	EXCEL_STATSLIST_INIT *						m_fpStatsListInit;
	EXCEL_STATSLIST_FREE *						m_fpStatsListFree;
	EXCEL_SCRIPT_EXECUTE *						m_fpScriptExecute;
	EXCEL_STATS_GET_STAT *						m_fpStatsGetStat;
	EXCEL_STATS_SET_STAT *						m_fpStatsSetStat;
	EXCEL_STATS_GET_STAT_FLOAT *				m_fpStatsGetStatFloat;
	EXCEL_STATS_SET_STAT_FLOAT *				m_fpStatsSetStatFloat;
	EXCEL_STATS_WRITE_STATS *					m_fpStatsWriteStats;
	EXCEL_STATS_READ_STATS *					m_fpStatsReadStats;
	EXCEL_STATS_PARSE_TOKEN *					m_fpStatsParseToken;
	EXCEL_SCRIPT_ADD_MARKER *					m_fpScriptAddMarker;
	EXCEL_SCRIPT_WRITE_TO_PAK *					m_fpScriptWriteToPak;
	EXCEL_SCRIPT_READ_FROM_PAK *				m_fpScriptReadFromPak;
	EXCEL_SCRIPT_GET_IMPORT_CRC *				m_fpScriptGetImportCRC;
	EXCEL_STRING_TABLES_LOAD *					m_fpStringTablesLoad;

	unsigned int								m_StringTablesUpdateState;

	struct STR_DICTIONARY *						m_VersionAppDict;			// VERSION_APP dictionary
	struct STR_DICTIONARY *						m_VersionPackageDict;		// VERSION_PACKAGE dictionary
	struct STR_DICTIONARY *						m_VersionPackageFlagsDict;	// VERSION_PACKAGE dictionary

	EXCEL_LOAD_MANIFEST							m_LoadManifest;
	EXCEL_READ_MANIFEST							m_ReadManifest;

	EXCEL(void);

	void Init(void);
	void Free(void);
	EXCEL_STRUCT * GetStructByName(const char * structname);
	EXCEL_STRUCT * AddStruct(const EXCEL_STRUCT & estruct);
	EXCEL_TABLE * RegisterTable(unsigned int idTable, const char * name, const char * structName, APP_GAME appGame, EXCEL_TABLE_SCOPE scope, unsigned int data1);
	BOOL StringTablesLoadAll(void);
	unsigned int StringTablesGetUpdateState(void);
	BOOL InitManifest(const EXCEL_LOAD_MANIFEST & manifest);
	BOOL LoadAll(void);
	BOOL LoadDatatable(unsigned nTable);
	BOOL LoadTablePostProcessAll();

	EXCEL_TABLE * GetTable(unsigned int idTable) const;
	unsigned int GetTableCount(void) const;
	EXCEL_LINK GetTableIdByName(const char * name) const;
	unsigned int GetFieldByName(unsigned int idTable, const char * fieldname) const;

#if EXCEL_FILE_LINE
	void * GetData(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int row, const char * file, unsigned int line);
#else
	void * GetData(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int row);
#endif
	const void * GetDataEx(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int row);
	EXCEL_LINK GetLineByKey(unsigned int idTable, unsigned int idIndex, const void * key, unsigned int len) const;
	EXCEL_LINK GetLineByKey(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int idIndex, const void * key, unsigned int len, BOOL bExact) const;
	EXCEL_LINK GetNextLineByKey(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int idIndex, const void * key, unsigned int len) const;
	EXCEL_LINK GetPrevLineByKey(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int idIndex, const void * key, unsigned int len) const;
	void * GetDataByKey(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int idIndex, const void * key, unsigned int len) const;
	template <typename T> EXCEL_LINK GetLineByStrKey(unsigned int idTable, T strkey) const;
	template <typename T> EXCEL_LINK GetLineByStrKey(unsigned int idTable, unsigned int idIndex, T strkey) const;
	template <typename T> EXCEL_LINK GetLineByStrKey(const EXCEL_CONTEXT & context, unsigned int idTable, T strkey) const;
	template <typename T> EXCEL_LINK GetLineByStrKey(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int idIndex, T strkey) const;
	template <typename T> EXCEL_LINK GetNextLineByStrKey(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int idIndex, T strkey) const;
	template <typename T> EXCEL_LINK GetPrevLineByStrKey(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int idIndex, T strkey) const;
	template <typename T> void * GetDataByStrKey(const EXCEL_CONTEXT & context, unsigned int idTable, T strkey) const;
	template <typename T> T GetStringKeyByLine(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int idIndex, unsigned int row) const;
	template <typename T> T GetStringKeyByLine(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int row) const;
	const WCHAR * GetNameStringByLine(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int row, GRAMMAR_NUMBER eNumber) const;
	template <typename T> EXCEL_LINK GetLineByCC(unsigned int idTable, unsigned int idIndex, T cc);
	BOOL HasScript(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int field_offset, unsigned int row);
	BYTE * GetScriptCode(const EXCEL_CONTEXT & context, unsigned int idTable, PCODE code_offset, int * maxlen);
	int EvalScript(const EXCEL_CONTEXT & context, struct UNIT * subject, struct UNIT * object, struct STATS * statslist, unsigned int idTable, unsigned int field_offset, unsigned int row);
	unsigned int GetLineByCode(const EXCEL_CONTEXT & context, unsigned int idTable, DWORD cc);
	void * GetDataByCode(const EXCEL_CONTEXT & context, unsigned int idTable, DWORD cc);
	DWORD GetCodeByLine(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int row) const;
	struct STATS * GetStatsList(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int row) const;
	unsigned int GetCount(const EXCEL_CONTEXT & context, unsigned int idTable);
	
	int Lookup(const EXCEL_CONTEXT & context, unsigned int idTable, unsigned int idField, unsigned int row, unsigned int arrayindex = 0);

#if EXCEL_TRACE
	void TraceLink(unsigned int idTable, unsigned int idIndex, EXCEL_LINK link);
	void TraceLinkTable(unsigned int idTable);
#endif
};


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
// not thread safe
EXCEL_TABLE * ExcelGetTableNotThreadSafe(		
	unsigned int idTable);

// not thread safe
unsigned int ExcelAppendTable(					
	EXCEL_TABLE * dest_table,
	EXCEL_TABLE * src_table,
	unsigned int startrow);

EXCEL_LINK ExcelGetLineByStringIndexPrivate(
	EXCEL_TABLE * table,
	const char * strkey);

BOOL ExcelAddRowToIndexPrivate(
	EXCEL_TABLE * table,
	unsigned int row);


//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int EXCEL_FIELD_COMPARE_BYNAME(
	const EXCEL_FIELD & a,
	const EXCEL_FIELD & b)
{
	ASSERT_RETZERO(a.m_ColumnName && b.m_ColumnName);
	return PStrICmp(a.m_ColumnName, b.m_ColumnName);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int EXCEL_STRUCT_COMPARE_BYNAME(
	EXCEL_STRUCT * const & a,
	EXCEL_STRUCT * const & b)
{
	ASSERT_RETZERO(a && b);
	ASSERT_RETZERO(a->m_Name && b->m_Name);
	return PStrICmp(a->m_Name, b->m_Name);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline unsigned int ExcelGetCountPrivate(
	EXCEL_TABLE * table)
{
	ASSERT_RETZERO(table);
	return table->m_Data.Count();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline unsigned int ExcelGetCountPrivate(
	EXCEL_TABLE * table,
	unsigned int idTable)
{
	ASSERT_RETZERO(table);
	if (table->m_Index == idTable)
	{
		return ExcelGetCountPrivate(table);
	}
	else
	{
		return ExcelGetCount(EXCEL_CONTEXT(), idTable);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline unsigned int ExcelGetRowDataSize(
	const EXCEL_TABLE * table)
{
	ASSERT_RETZERO(table);
	ASSERT_RETZERO(table->m_Struct);
	unsigned int rowsize = sizeof(EXCEL_DATA_VERSION) + table->m_Struct->m_Size;
	return rowsize;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline EXCEL_DATA_VERSION * ExcelGetVersionFromRowData(
	const BYTE * byte)
{
	ASSERT_RETNULL(byte);
	return (EXCEL_DATA_VERSION *)(byte);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline EXCEL_DATA_VERSION * ExcelGetVersionFromRowData(
	const EXCEL_DATA & data)
{
	return ExcelGetVersionFromRowData(data.m_RowData);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL ExcelCheckReadVersion(
	const EXCEL_DATA_VERSION * version,
	const EXCEL_READ_MANIFEST & manifest)
{
	ASSERT_RETFALSE(version);
	ASSERT(version->m_VersionApp != 0);

	if ((manifest.m_VersionApp & version->m_VersionApp) == 0)
	{
		return FALSE;
	}
	if ((manifest.m_VersionPackage & version->m_VersionPackage) == 0)
	{
		return FALSE;
	}
	if (manifest.m_VersionMajor < version->m_VersionMajorMin)
	{
		return FALSE;
	}
	if (manifest.m_VersionMajor > version->m_VersionMajorMax)
	{
		return FALSE;
	}
	if (manifest.m_VersionMinor < version->m_VersionMinorMin)
	{
		return FALSE;
	}
	if (manifest.m_VersionMinor > version->m_VersionMinorMax)
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BYTE * ExcelGetDataFromRowData(
	const BYTE * rowdata)
{
	ASSERT_RETNULL(rowdata);
	return (BYTE *)(rowdata + sizeof(EXCEL_DATA_VERSION));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BYTE * ExcelGetDataFromRowData(
	const EXCEL_CONTEXT & context,
	const BYTE * rowdata)
{
	ASSERT_RETNULL(rowdata);
	if (!ExcelCheckReadVersion(ExcelGetVersionFromRowData(rowdata), context.manifest))
	{
		return NULL;
	}
	return (BYTE *)ExcelGetDataFromRowData(rowdata);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BYTE * ExcelGetDataFromRowData(
	const EXCEL_DATA & data)
{
	return ExcelGetDataFromRowData(data.m_RowData);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BYTE * ExcelGetDataFromRowData(
	const EXCEL_CONTEXT & context,
	const EXCEL_DATA & data)
{
	return ExcelGetDataFromRowData(context, data.m_RowData);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BYTE * ExcelGetDataFromRowData(
	const EXCEL_INDEX_NODE & idxnode)
{
	return ExcelGetDataFromRowData(idxnode.m_RowDataEx);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BYTE * ExcelGetDataFromRowData(
	const EXCEL_CONTEXT & context,
	const EXCEL_INDEX_NODE & idxnode)
{
	return ExcelGetDataFromRowData(context, idxnode.m_RowDataEx);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void * ExcelGetDataPrivate(
	EXCEL_TABLE * table,
	unsigned int row)
{
	ASSERT_RETNULL(table);
	ASSERT_RETNULL(table->m_Data.Count() > row);
	return ExcelGetDataFromRowData(EXCEL_CONTEXT(), table->m_Data[row]);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void * ExcelGetDataPrivate(
	EXCEL_TABLE * table,
	unsigned int idTable,
	unsigned int row)
{
	ASSERT_RETNULL(table);

	if (table->m_Index == idTable)
	{
		return ExcelGetDataPrivate(table, row);
	}
	else
	{
		return (void *)ExcelGetData(EXCEL_CONTEXT(), idTable, row);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline EXCEL_LINK ExcelGetLineByStringIndexPrivate(
	EXCEL_TABLE * table,
	unsigned int idTable,
	const char * strkey)
{
	ASSERT_RETVAL(table, EXCEL_LINK_INVALID);
	if (table->m_Index == idTable)
	{
		return ExcelGetLineByStringIndexPrivate(table, strkey);
	}
	else
	{
		return ExcelGetLineByStringIndex(EXCEL_CONTEXT(), idTable, strkey);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline unsigned int ExcelTableGetCurParseParam(
	const EXCEL_TABLE * table)
{
	ASSERT_RETZERO(table);
	ASSERT_RETZERO(table->m_CurFile < table->m_Files.Count());
	return table->m_Files[table->m_CurFile].m_Data1;
}


#endif

