// ---------------------------------------------------------------------------
// FILE: definition_priv.h
// ---------------------------------------------------------------------------
#ifndef _DEFINITION_PRIV_H_
#define _DEFINITION_PRIV_H_


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// TYPEDEF
//----------------------------------------------------------------------------
typedef BOOL (PFN_DEFINITION_POSTLOAD_PRIMARY)(void *, BOOL bForceSync);
typedef void (PFN_DEFINITION_POSTLOAD)(void *);

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum
{
	ELEMENT_TYPE_NONE = -1,
	ELEMENT_TYPE_MEMBER = 0,
	ELEMENT_TYPE_MEMBER_FLAG,
	ELEMENT_TYPE_MEMBER_FLAG_BIT,
	ELEMENT_TYPE_MEMBER_LINK,
	ELEMENT_TYPE_MEMBER_INDEX_LINK,
	ELEMENT_TYPE_MEMBER_LINK_ARRAY,
	ELEMENT_TYPE_MEMBER_ARRAY,
	ELEMENT_TYPE_MEMBER_VARRAY,
	ELEMENT_TYPE_REFERENCE,
	ELEMENT_TYPE_REFERENCE_ARRAY,
	ELEMENT_TYPE_REFERENCE_VARRAY,
};

enum
{
	DATA_TYPE_NONE = -1,
	DATA_TYPE_INT = 0,
	DATA_TYPE_FLOAT,
	DATA_TYPE_STRING,
	DATA_TYPE_DEFINITION,
	DATA_TYPE_PATH_FLOAT,
	DATA_TYPE_PATH_FLOAT_PAIR,
	DATA_TYPE_PATH_FLOAT_TRIPLE,
	DATA_TYPE_INT_NOSAVE,
	DATA_TYPE_FLOAT_NOSAVE,
	DATA_TYPE_INT_LINK,
	DATA_TYPE_INT_DEFAULT,
	DATA_TYPE_FLAG,
	DATA_TYPE_FLAG_BIT,
	DATA_TYPE_POINTER_NOSAVE,
	DATA_TYPE_INT_INDEX_LINK,
	DATA_TYPE_INT_64,
	DATA_TYPE_BYTE,
};


//----------------------------------------------------------------------------
// Macros used to create a definition - this way we can change the format and members
// without having to change a bunch of other code.
// struct and default are already used... so we start misspelling...
//----------------------------------------------------------------------------
#define MAKE_HASH(var)												(CRC(0,(const BYTE *)(var),PStrLen(var)))

#define DEFINITION_DECLARE(name)									extern CDefinitionLoader name;
#define DEFINITION_START(strukt,name,magic,bheader,bdirect,pfnPostLoad,hashsize)	CDefinitionLoader name = { #strukt, MAKE_HASH(#strukt), sizeof(strukt), magic, 0, bheader, bdirect, pfnPostLoad, hashsize, NULL, NULL, 0, {

#define DEFINITION_MEMBER(strukt,type,var,defolt)					{ #var, MAKE_HASH(#var), ELEMENT_TYPE_MEMBER, type, NULL, NULL, (int)OFFSET(strukt,var), 1, 0, -1, -1, #defolt, 0 },
#define DEFINITION_MEMBER_FLAG(strukt,var,shifted,defolt)			{ #shifted, MAKE_HASH(#shifted), ELEMENT_TYPE_MEMBER_FLAG, DATA_TYPE_FLAG, NULL, NULL, (int)OFFSET(strukt,var), 1, 0, shifted, -1, #defolt, 0 },
#define DEFINITION_MEMBER_FLAG_BIT(strukt,var,bit,count)			{ #bit, MAKE_HASH(#bit), ELEMENT_TYPE_MEMBER_FLAG_BIT, DATA_TYPE_FLAG_BIT, NULL, NULL, (int)OFFSET(strukt,var), 1, 0, bit, count, "0", 0 },
#define DEFINITION_MEMBER_LINK(strukt,var,table)					{ #var, MAKE_HASH(#var), ELEMENT_TYPE_MEMBER_LINK, DATA_TYPE_INT_LINK, NULL, NULL, (int)OFFSET(strukt,var), 1, 0, table, -1, "", (DWORD)(-1) },
#define DEFINITION_MEMBER_INDEX_LINK(strukt,var,table, index)		{ #var, MAKE_HASH(#var), ELEMENT_TYPE_MEMBER_INDEX_LINK, DATA_TYPE_INT_INDEX_LINK, NULL, NULL, (int)OFFSET(strukt,var), 1, 0, table, index, "", (DWORD)(-1) },
#define DEFINITION_MEMBER_LINK_ARRAY(strukt,var,table,length)		{ #var, MAKE_HASH(#var), ELEMENT_TYPE_MEMBER_LINK_ARRAY, DATA_TYPE_INT_LINK, NULL, NULL, (int)OFFSET(strukt,var), length, 0, table, -1, "", (DWORD)(-1) },
#define DEFINITION_MEMBER_LINK_VARRAY(strukt,var,table,lenvar)		{ #var, MAKE_HASH(#var), ELEMENT_TYPE_MEMBER_LINK_ARRAY, DATA_TYPE_INT_LINK, NULL, NULL, (int)OFFSET(strukt,var), -1, OFFSET(strukt,lenvar), table, -1, "", (DWORD)(-1) },
#define DEFINITION_MEMBER_ARRAY(strukt,type,var,defolt,length)		{ #var, MAKE_HASH(#var), ELEMENT_TYPE_MEMBER_ARRAY, type, NULL, NULL, (int)OFFSET(strukt,var), length, 0, -1, -1, #defolt, 0 },

// MEMBER_VARRAY WARNING:  Don't reference the value specified for "lenvar" anywhere else in the DEFINITION declaration!
//   The "lenvar" variable maps to a structure varray count member.  In the XML file, the varray count is
//   saved in the XML as "<arrayname>Count".
#define DEFINITION_MEMBER_VARRAY(strukt,type,var,defolt,lenvar)		{ #var, MAKE_HASH(#var), ELEMENT_TYPE_MEMBER_VARRAY, type, NULL, NULL, (int)OFFSET(strukt,var), -1, OFFSET(strukt,lenvar), -1, -1, #defolt, 0 },
#define DEFINITION_REFERENCE(strukt,type,var)						{ #var, MAKE_HASH(#var), ELEMENT_TYPE_REFERENCE, DATA_TYPE_DEFINITION, &type, NULL, (int)OFFSET(strukt,var), 1, 0, -1, -1, "", 0 },
#define DEFINITION_REFERENCE_ARRAY(strukt,type,var,length)			{ #var, MAKE_HASH(#var), ELEMENT_TYPE_REFERENCE_ARRAY, DATA_TYPE_DEFINITION, &type, NULL, (int)OFFSET(strukt,var), length, 0, -1, -1, "", 0 },

// REFERENCE_VARRAY WARNING:  Don't reference the value specified for "lenvar" anywhere else in the DEFINITION declaration!
//   The "lenvar" variable maps to a structure varray count member.  In the XML file, the varray count is
//   saved in the XML as "<arrayname>Count".
#define DEFINITION_REFERENCE_VARRAY(strukt,type,var,lenvar)			{ #var, MAKE_HASH(#var), ELEMENT_TYPE_REFERENCE_VARRAY, DATA_TYPE_DEFINITION, &type, NULL, (int)OFFSET(strukt,var), -1, (int)OFFSET(strukt,lenvar), -1, -1, "", 0 },

#define DEFINITION_END												{ NULL, 0, ELEMENT_TYPE_NONE, DATA_TYPE_NONE, NULL, NULL, 0, 0, 0, -1, 0, 0 },  } }; 

#define MAX_ELEMENTS_PER_DEFINITION		200
#define MAX_SAVED_DEFINITIONS			30

#define WRITEFILE_BUFSIZE				64

//-------------------------------------------------------------------------------------------------
class CDefinitionContainer *& DefinitionContainerGetRef( DEFINITION_GROUP_TYPE eType );

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
struct FILE_REQUEST_DATA
{
	DEFINITION_GROUP_TYPE eGroup; 
	const char * pszFullFileName;
	const char * pszDefinitionName; 
	const char * pszPathName;
	int nDefinitionId;
	BOOL bForceLoadDirect;
	BOOL bNeverPutInPak;
	BOOL bZeroOutData;
	BOOL bWarnWhenMissing;
	BOOL bAlwaysWarnWhenMissing;
	BOOL bNodirectIgnoreMissing;
	BOOL bAsyncLoad;
	BOOL bForceIgnoreWarnIfMissing;
	int nFileLoadPriority;
	enum PAK_ENUM ePakfile;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
enum
{
	DEFINITION_TRANSFER_READ,
	DEFINITION_TRANSFER_WRITE,
};

class CDefinitionLoader;

struct DEFINITION_TRANSFER_CONTEXT
{
	int nReadOrWrite;
	int nNumSavedDefinitons;
	DWORD nSavedDefinition[MAX_SAVED_DEFINITIONS];
	CDefinitionLoader * pSavedDefinitionLoader[MAX_SAVED_DEFINITIONS];
#if ISVERSION(DEVELOPMENT)
	char pszDefinitionName[MAX_SAVED_DEFINITIONS][256];
#endif
	union
	{
		WRITE_BUF_DYNAMIC * writeBuf;
		BYTE_BUF * readBuf;
	};
	void * pTempBuf;
	int nTempBufSize;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> class CInOrderHash;

struct DefinitionAsynchLoadCallback
{
	int								nId;
	DefinitionAsynchLoadCallback	*pNext;

	PFN_DEFINITION_POSTASYNCHLOAD	*m_fnPostLoad;
	struct EVENT_DATA				*m_pEventData;
};


class CDefinitionLoader 
{
	friend class CDefinitionContainer;

public:
	void Init(struct MEMORYPOOL * pMemPool);
	void ShutDown();
		
	BOOL RequestFile( 
		FILE_REQUEST_DATA & tData);

	BOOL ProcessFile( 
		FILE_REQUEST_DATA & tData,
		struct PAKFILE_LOAD_SPEC & tSpec,
		BOOL bLoadingCooked,
		BOOL bForceAddToPack);

	BOOL SaveFile(
		void * pSource,
		const char * pszFilePath,
		BOOL bIgnoreFilePath);

	BOOL SaveFile(
		void * pSource,
		const WCHAR * pszFilePath,
		BOOL bIgnoreFilePath);

	void Copy(
		void * pTarget,
		void * pSource,
		BOOL bZeroOutData);

	void InitValues(
		void * pTarget,
		BOOL bZeroOutData);

	// CHB 2006.09.29
	void InitValues2(
		void * pTarget,
		BOOL bZeroOutData);

	void AddPostLoadCallback(
		int nId,
		PFN_DEFINITION_POSTASYNCHLOAD * fnPostLoad,
		struct EVENT_DATA * pEventData);

	void CDefinitionLoader::CallAllPostLoadCallbacks(
		int nDefinitionId,
		void * pDefinition );

	void FreeElements(
		void * pData,
		int nCount,
		BOOL bFreeElements );

	void FreeElementData(
		DEFINITION_TRANSFER_CONTEXT & dtc);

	void InitElementData();

	CDefinitionLoader * CDefinitionLoader::AllocEmptyDefLoader();

	class CElement 
	{
	public:
		const char *			m_pszVariableName;
		DWORD					m_dwVariableNameHash;
		char					m_eElementType;
		char					m_eDataType;
		CDefinitionLoader *		m_pDefinitionLoader;
		CElement *				m_pNewElement;
		int						m_nOffset;
		int						m_nArrayLength;
		int						m_nVariableArrayOffset;
		int						m_nParam;
		int						m_nParam2;
//		char					m_pszDefaultValue[MAX_XML_STRING_LENGTH];
		const char *			m_pszDefaultValue;
		union
		{
			DWORD				m_DefaultValue;
			unsigned __int64	m_DefaultValue64;
		};
	};

//	char						m_pszDefinitionName[MAX_VARIABLE_NAME_LENGTH];
	const char *				m_pszDefinitionName;
	DWORD						m_dwDefinitionNameHash;
	int							m_nStructSize;
	DWORD						m_dwMagicNumber;
	DWORD						m_dwCRC;
	BOOL						m_bHeader;
	BOOL						m_bAlwaysCheckDirect;
	PFN_DEFINITION_POSTLOAD_PRIMARY *	m_fnPostLoad;
	int							m_nHashSize;
	CInOrderHash<DefinitionAsynchLoadCallback> * m_pCallbacks;
	struct MEMORYPOOL *			m_pMemPool;
	DWORD						m_nNumElements;
	CElement					m_pElements[MAX_ELEMENTS_PER_DEFINITION];

protected:
	void Load(
		void * pTarget,
		class CMarkup * pMarkup,
		BOOL bZeroOutData,
		BOOL bDefinitionFound );

	void Save(
		void * pSource,
		class CMarkup * pMarkup);

	BOOL SaveCooked(
		const void * source,
		WRITE_BUF_DYNAMIC & fbuf);

	BOOL LoadCooked(
		void * target,
		class BYTE_BUF & buf,
		BOOL bZeroOutData);

	BOOL TransferCooked(
		DEFINITION_TRANSFER_CONTEXT & dtc,					
		const void * source,
		BOOL bZeroOutData = FALSE);

	BOOL TransferCookedDefinitions(
		DEFINITION_TRANSFER_CONTEXT & dtc);

	BOOL TransferCookedData(
		DEFINITION_TRANSFER_CONTEXT & dtc,					
		const void * source,
		CDefinitionLoader * pOldDefs,
		BOOL bZeroOutData = FALSE);

	BOOL ElementIsDefault(
		void * ptr,
		const CDefinitionLoader::CElement * element);

	void FillWithDefaults(
		void * ptr,
		CDefinitionLoader::CElement * element,
		CDefinitionLoader::CElement * oldElement,
		const int iStart,
		const int iEnd);

	template <class TDataType>
	void LoadPath(
		void * pTarget, 
		class CMarkup * pMarkup,
		const char * pszDefault);

	template <class TDataType>
	void SavePath(
		void * pSource,
		class CMarkup * pMarkup);

	void ComputeVersion(
		void);

	void FixDefaults(void);
	
	void FixDefault(
		CDefinitionLoader::CElement * element);

private:
	void FreeElementData_(
		DEFINITION_TRANSFER_CONTEXT & dtc);

	template<typename T> BOOL TransferCookedPath(
		DEFINITION_TRANSFER_CONTEXT & dtc,
		T * path,
		CDefinitionLoader::CElement * element,
		int & file_array_length,
		int & skip_array_length);

	void InitChildMemPools(struct MEMORYPOOL * pMemPool);
};


//-------------------------------------------------------------------------------------------------
class CDefinitionContainer
{
public:
	enum
	{
		FLAG_EVEN_IF_NOT_LOADED				= MAKE_MASK(0),
		FLAG_NAME_HAS_PATH					= MAKE_MASK(1),
		FLAG_FORCE_SYNC_LOAD				= MAKE_MASK(2),
		FLAG_FORCE_DONT_WARN_IF_MISSING		= MAKE_MASK(3),
		FLAG_NULL_WHEN_MISSING				= MAKE_MASK(4),
	};

	CDefinitionContainer() 
		:	m_eDefinitionGroup( -1 ),
		m_pLoader( NULL ), 
		m_pszFilePath ( NULL ),
		m_pszFilePathOS ( NULL ),
		// CML 2008.05.06 - Definition headers now contain app-root-relative filepaths, so we shouldn't prepend the filepath ever.
		m_bIgnoreFilePathOnSave( TRUE ),
		m_bZeroOutData( TRUE ),
		m_bRemoveExtension( TRUE ),
		m_bWarnWhenMissing( FALSE ),
		m_bAlwaysWarnWhenMissing( FALSE ),
		m_bNodirectIgnoreMissing( FALSE ),
		m_bAlwaysLoadDirect( FALSE ),
		m_bNeverPutInPak( FALSE ),
		m_bAsyncLoad( FALSE ),
		m_nLoadPriority( 0 ),
		m_pMemPool( g_StaticAllocator ),
		m_nNextId( 0 )
	{
	}

	virtual ~CDefinitionContainer()
	{
		if(m_pLoader)
		{
			m_pLoader->ShutDown();
		}
		HashFree( m_Hash );
	}

	typedef struct DEFINITION_HASH
	{
		int nId;
		DEFINITION_HASH * pNext;

		void * pData;

		DEFINITION_HEADER * GetHeader() 
		{
			if ( pData )
			{
				return (DEFINITION_HEADER *)((BYTE *)pData);
			} else {
				return NULL;
			}
		}
	};

	void InitLoader()
	{
		if(m_pLoader)
		{
			m_pLoader->Init(m_pMemPool);
			
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
			HashInit( m_Hash, g_StaticAllocator, m_pLoader->m_nHashSize );
#else
			HashInit( m_Hash, NULL, m_pLoader->m_nHashSize );
#endif					
		}
	}

	static CDefinitionContainer * GetContainerByType( DEFINITION_GROUP_TYPE eType );
	static void InitCommon( );
	static void InitEngine( );
	static void Init( );
	static void Close( );
	const char * GetName() { return m_pLoader->m_pszDefinitionName; }

	virtual void Restore( void * pData ) {UNREFERENCED_PARAMETER(pData);}
	virtual void * Alloc ( int nCount ) = 0;
	virtual void Free( void * pData, int nCount ) = 0;
	virtual void Destroy () 
	{ 
		DEFINITION_HASH * pDefHash = HashGetFirst( m_Hash );
		while ( pDefHash )
		{
			Free( pDefHash->pData, 1 );
			pDefHash->pData = NULL;
			pDefHash = HashGetNext( m_Hash, pDefHash );
		}
		HashFree( m_Hash );
	}

	void AddPostLoadCallback(int nId, PFN_DEFINITION_POSTASYNCHLOAD * fnPostLoad, struct EVENT_DATA * eventData);

	void * GetByName( 
		const char * pszName, 
		DWORD dwFlags = 0,
		int nPriorityOverride = -1,
		PAK_ENUM ePakfile = PAK_DEFAULT);

	void * GetById	( int nId, BOOL bEvenIfNotLoaded = FALSE );
	int GetIdByName( const char * pszName, int nPriority, BOOL bForceSyncLoad, BOOL bForceIgnoreWarnIfMissing = FALSE );
	void Load( DEFINITION_HEADER* pHeader, BOOL bFilenameHasPath, int nId, BOOL bAsyncLoad, int nPriority, BOOL bForceIgnoreWarnIfMissing = FALSE, PAK_ENUM ePakfile = PAK_DEFAULT );
	BOOL Save		( void * pDefinition, BOOL bAndSaveCooked = FALSE );
	BOOL SaveCooked ( void* pDefinition );
	void Reload		( void* pDefinition );
	virtual void Copy		( void ** ppTarget, void * pSource );
	void RestoreAll	( );
	int GetCount () { return HashGetCount( m_Hash ); }
	void SetDefinitionGroup( DEFINITION_GROUP_TYPE eType ) { m_eDefinitionGroup = eType; }
	int GetFirst ( BOOL bEvenIfNotLoaded = FALSE );
	int GetNext ( int nId, BOOL bEvenIfNotLoaded = FALSE );

	const char * GetFilePath () { return m_pszFilePath; }
	const OS_PATH_CHAR * GetFilePathOS() { return m_pszFilePathOS; }
	void CallAllPostLoadCallbacks(
		int nId ) 
	{ 
		m_pLoader->CallAllPostLoadCallbacks( nId, GetById( nId ) );
	}

	static BOOL Closed(
		void)
	{
		return m_bClosed;
	}

	BOOL ShouldIgnoreFilePath() { return m_bIgnoreFilePathOnSave; }

protected:
	DEFINITION_GROUP_TYPE m_eDefinitionGroup;
	class CDefinitionLoader * m_pLoader;
	const char *m_pszFilePath;
	const OS_PATH_CHAR *m_pszFilePathOS;
	BOOL m_bIgnoreFilePathOnSave;
	BOOL m_bZeroOutData;
	BOOL m_bRemoveExtension;
	BOOL m_bWarnWhenMissing;
	BOOL m_bAlwaysWarnWhenMissing;
	BOOL m_bNodirectIgnoreMissing;
	BOOL m_bAlwaysLoadDirect;
	BOOL m_bNeverPutInPak;
	BOOL m_bAsyncLoad;
	BOOL m_nLoadPriority;
	static BOOL m_bClosed;
	struct MEMORYPOOL * m_pMemPool;

	int m_nNextId;
	CHash<DEFINITION_HASH> m_Hash;
};

void DefinitionForceNoAsync();

#if ISVERSION(DEVELOPMENT)
void testDefinition ();
#endif

#endif // _DEFINITION_PRIV_H_
