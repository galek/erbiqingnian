// ---------------------------------------------------------------------------
// database.h
// ---------------------------------------------------------------------------
#ifndef _DATABASE_H_
#define _DATABASE_H_

// ---------------------------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------------------------
// string lengths
#define MAX_FIELD_NAME_LEN			40
#define MAX_ERROR_MSG_LEN			1024
#define MAX_SQL_STATE_LEN			6
#define MAX_SQL_STATEMENT_LEN		4096


// ---------------------------------------------------------------------------
// MACROS
// ---------------------------------------------------------------------------
#define SQL_OK(res)		((res) == SQL_SUCCESS || (res) == SQL_SUCCESS_WITH_INFO)
#define SQL_ERR(res)	(!SQL_OK(res))
#define SAFE_STR(str)	((str == NULL) ? _T("") : str)


// ---------------------------------------------------------------------------
// CLASSES
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
class ODBC_CONNECTION
{
private:
	HENV            m_hEnv; 
	HDBC			m_hDBC;

public:
	HDBC GetHDBC(void) { return m_hDBC; }

	BOOL Connect(LPCTSTR source);
	BOOL IsConnected(void);
	void Disconnect(void) { if (m_hDBC) SQLDisconnect(m_hDBC); }

	BOOL Init(void) { m_hDBC = NULL; m_hEnv = NULL; return TRUE; }
	void Free(void);

	void PrintError(SQLSMALLINT handletype, SQLHANDLE handle);
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
class MSSQL_CONNECTION : public ODBC_CONNECTION
{
public:
	enum enumProtocols
	{
		protoNamedPipes,
		protoWinSock,
		protoIPX,
		protoBanyan,
		protoRPC
	};

public:
	BOOL Connect(
		LPCTSTR Address = _T(""),
		LPCTSTR Server = _T("(local)"),
		LPCTSTR User = _T(""),
		LPCTSTR Pass = _T(""),
		LPCTSTR Database = _T(""),
		BOOL Trusted = 1, 
        enumProtocols Proto = protoNamedPipes);
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
class ODBC_STATEMENT
{
private:
	HSTMT m_hStmt;

public:
	operator HSTMT(void) { return m_hStmt; }

	ODBC_STATEMENT(HDBC hDBC);
	virtual ~ODBC_STATEMENT(void);

	BOOL IsValid(void) { return m_hStmt != INVALID_HANDLE_VALUE; }
	void PrintError(void);

	BOOL BindInputParameter(
		SQLUSMALLINT parameter,
		SQLSMALLINT value_type,
		SQLSMALLINT param_type,
		SQLPOINTER ptr,
		SQLINTEGER buflen,
		SQLLEN * strlen_ptr,
		SQLUINTEGER column_size,
		SQLSMALLINT decimal_digits);

	USHORT GetColumnCount(void);
	SQLLEN GetChangedRowCount(void);

	BOOL Query(LPCTSTR strSQL)
		{ return SQL_OK(SQLExecDirect(m_hStmt, (SQLTCHAR *)strSQL, SQL_NTS)); }

	BOOL Fetch(void)
		{ return SQL_OK(SQLFetch(m_hStmt)); }
	BOOL FecthRow(USHORT row)
		{ return SQL_OK(SQLSetPos(m_hStmt, row, SQL_POSITION, SQL_LOCK_NO_CHANGE)); }
	BOOL FetchRow(ULONG row, BOOL absolute = TRUE)
		{ return SQL_OK(SQLFetchScroll(m_hStmt, (absolute ? SQL_FETCH_ABSOLUTE : SQL_FETCH_RELATIVE), row)); }
	BOOL FetchPrevious(void)
		{ return SQL_OK(SQLFetchScroll(m_hStmt, SQL_FETCH_PRIOR, 0)); }
	BOOL FetchNext(void)
		{ return SQL_OK(SQLFetchScroll(m_hStmt, SQL_FETCH_NEXT, 0)); }
	BOOL FetchFirst(void)
		{ return SQL_OK(SQLFetchScroll(m_hStmt, SQL_FETCH_FIRST, 0)); }
	BOOL FetchLast(void)
		{ return SQL_OK(SQLFetchScroll(m_hStmt, SQL_FETCH_LAST, 0)); }

	BOOL Cancel(void)
		{ return SQL_OK(SQLCancel(m_hStmt)); }
};
 
 
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
class ODBC_RECORD
{
private:
	HSTMT m_hStmt;

public:
	ODBC_RECORD(HSTMT hStmt) { m_hStmt = hStmt; };
	~ODBC_RECORD(void) { }

	USHORT GetColumnCount(void);
	BOOL BindColumn(
		USHORT column,
		LPVOID buffer,
		ULONG buflen,
		SQLLEN * ret_buflen = NULL,
		USHORT type = SQL_C_TCHAR);

	USHORT GetColumnByName(LPCTSTR column);
	BOOL GetData(
		USHORT column, 
		LPVOID buffer, 
		ULONG buflen, 
		SQLLEN * datalen = NULL, 
		SQLSMALLINT type = SQL_C_DEFAULT);

	int GetColumnType(USHORT column);
	UINT GetColumnSize(USHORT column);
	USHORT GetColumnScale(USHORT column);
	BOOL GetColumnName(
		USHORT column,
		LPTSTR name,
		SHORT bufsize);
	BOOL IsColumnNullable(USHORT column);
};


// ---------------------------------------------------------------------------
// EXPORTED FUNCTIONS
// ---------------------------------------------------------------------------
BOOL DatabaseQuery(
	ODBC_STATEMENT & statement,
	const TCHAR * sql_stmt);


// ---------------------------------------------------------------------------
// SQL datetime functions
// these functions treat time_t = 0 and SQL_TIMESTAMP_STRUCT = { 0 } as null 
// ---------------------------------------------------------------------------

bool IsSQLTimeStampNull(const SQL_TIMESTAMP_STRUCT& SQLTimeStamp);

time_t ConvertSQLTimeStampToTimeT(const SQL_TIMESTAMP_STRUCT& SQLTimeStamp);

SQL_TIMESTAMP_STRUCT ConvertTimeTToSQLTimeStamp(time_t TimeT);


#endif // _DATABASE_H_
