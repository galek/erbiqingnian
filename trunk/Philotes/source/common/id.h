//----------------------------------------------------------------------------
// id.h
//
//----------------------------------------------------------------------------
#ifndef _ID_H_
#define _ID_H_


#if (!ISVERSION(DEBUG_ID))

#define		ID							DWORD
#define		INVALID_ID					((DWORD)-1)
#define		INVALID_CODE				((DWORD)-1)
#define		INVALID_FILEID				(FILEID())
#define		INVALID_FILE				INVALID_HANDLE_VALUE
#define		INVALID_CLIENTID			((APPCLIENTID)(-1))
#define		INVALID_GUID				((PGUID)-1)
#define		INVALID_CHANNELID			((CHANNELID)-1)
#define		INVALID_GAMECLIENTID		((GAMECLIENTID)-1)
#define		INVALID_UNIQUE_ACCOUNT_ID	((UNIQUE_ACCOUNT_ID)-1)
#define		ID_PRINTFIELD				"%d"
#define		ID_PRINT(id)				id

#else

#define		MAX_ID_DEBUG_NAME	60
class ID
{
public:
	DWORD	m_dwId;
	char	m_szName[MAX_ID_DEBUG_NAME];

	ID(
		void) : m_dwId(-1)
	{
		memcpy(m_szName, "INVALID_ID", 11);
	}

	ID(
		const ID& b) : m_dwId(b.m_dwId)
	{
		PStrCopy(m_szName, b.m_szName, MAX_ID_DEBUG_NAME);
	}

	ID(
		DWORD id) : m_dwId(id)
	{
		PStrCopy(m_szName, "unknown", MAX_ID_DEBUG_NAME);
	}

	ID(
		DWORD id,
		const char* name) : m_dwId(id)
	{
		PStrCopy(m_szName, name, sizeof(m_szName));
		m_szName[sizeof(m_szName)-1] = 0;
	}

	bool operator==(
		DWORD b) const
	{
		return (m_dwId == b);
	}


	bool operator==(
		const ID& b) const
	{
		return (m_dwId == b.m_dwId);
	}

	bool operator!=(
		const ID& b) const
	{
		return (m_dwId != b.m_dwId);
	}

	bool operator!=(
		DWORD b) const
	{
		return (m_dwId != b);
	}

	ID& operator++(
		void)
	{
		m_dwId++;
		return *this;
	}

	operator unsigned int(
		void) const
	{
		return m_dwId;
	}

	void operator=(
		const ID& b)
	{
		m_dwId = b.m_dwId;;
		PStrCopy(m_szName, b.m_szName, sizeof(m_szName));
	}
};

static ID INVALID_ID;
#define		INVALID_FILEID				(FILEID())
#define		INVALID_FILE				INVALID_HANDLE_VALUE
#define		INVALID_GUID				((PGUID)-1)

#define		ID_PRINTFIELD				"%d (%s)"
#define		ID_PRINT(id)				DWORD(id), id.m_szName
#endif

#define		INVALID_GAMEID				INVALID_ID							// even if GAMEID isn't DWORD sized, let's make this our default INVALID_GAMEID
#define		GAMEID_IS_INVALID(id)		(((id) & 0xffffffff) == INVALID_GAMEID)

typedef unsigned __int64				GAMEID;
typedef DWORD							ROOMID;
typedef DWORD							APPCLIENTID;
typedef unsigned __int64				GAMECLIENTID;
typedef DWORD							PARTYID;
typedef	unsigned __int64				PGUID;
#define GUID_GET_HIGH_DWORD(guid)		((DWORD)((guid & (PGUID)0xFFFFFFFF00000000i64) >> 32i64))
#define GUID_GET_LOW_DWORD(guid)		((DWORD)(guid & (PGUID)0x00000000FFFFFFFFi64))
#define SPLIT_GUID(guid, hi, lo)		{ hi = GUID_GET_HIGH_DWORD(guid); lo = GUID_GET_LOW_DWORD(guid); }
#define CREATE_GUID(hi, lo)				((PGUID)(((unsigned __int64)hi << 32i64) + (unsigned __int64)lo))
typedef DWORD							CHANNELID;

typedef ULONGLONG						UNIQUE_ACCOUNT_ID; // This is the identifier for a customer account.
typedef int								ASYNC_REQUEST_ID;

#if (!ISVERSION(DEBUG_ID))
typedef DWORD							UNITID;
#else
typedef ID								UNITID;
#endif

struct FILEID
{
	union
	{
		struct
		{
			DWORD	idDirectory;
			DWORD	idFile;
		};
		UINT64		id;
	};

	operator unsigned __int64(void) const	{ return id; }

	FILEID(void) : id(UINT64(-1)) {}
};


#endif // _ID_H_
