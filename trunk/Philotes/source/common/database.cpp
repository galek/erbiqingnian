//----------------------------------------------------------------------------
// database.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
// NOTES
// further optimization in big way, statements can be held in global db
// struct instead of reinstatiated each time, then prepare would only have
// to occur once,
// updates can be batched,
// way too many memcopies
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#include "mailslot.h"
#include "database.h"
#define _MAX__TIME64_T     0x793406fffi64 

#if ISVERSION(SERVER_VERSION)
#include "database.cpp.tmh"
#endif


// ---------------------------------------------------------------------------
// class ODBC_CONNECTION 
// ---------------------------------------------------------------------------

BOOL ODBC_CONNECTION::Connect(LPCTSTR source)
{
	int result;

	ONCE
	{
		result = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv);
		if (SQL_ERR(result))
		{
			break;
		}
		result = SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
		if (SQL_ERR(result))
		{
			PrintError(SQL_HANDLE_ENV, m_hEnv);
			break;
		}
		result = SQLAllocHandle(SQL_HANDLE_DBC, m_hEnv, &m_hDBC);
		if (SQL_ERR(result))
		{
			PrintError(SQL_HANDLE_ENV, m_hEnv);
			break;
		}
		SQLSetConnectOption(m_hDBC,	SQL_LOGIN_TIMEOUT, 5);
		short shortResult = 0;
		SQLTCHAR szOutConnectString[1024];
		result = SQLDriverConnect(m_hDBC, NULL, (SQLTCHAR *)source, (SQLSMALLINT)_tcslen(source), szOutConnectString, (SQLSMALLINT)sizeof(szOutConnectString), &shortResult, SQL_DRIVER_NOPROMPT);
		if (SQL_ERR(result))
		{
			PrintError(SQL_HANDLE_DBC, m_hDBC);
			break;
		}
		return TRUE;
	}

	Free();
	return FALSE;
}


BOOL ODBC_CONNECTION::IsConnected(void)
{
	if(m_hDBC)
	{
		SQLINTEGER connectionDead = SQL_CD_TRUE;
		SQLINTEGER nLen = 0;
		SQLRETURN hret = SQLGetConnectAttr(
							m_hDBC,
							SQL_ATTR_CONNECTION_DEAD,
							&connectionDead,
							0,
							&nLen);
		return SQL_OK(hret) && (connectionDead == SQL_CD_FALSE);
	}
	return FALSE;
}


void ODBC_CONNECTION::Free(void)
{
	Disconnect();
	if (m_hDBC)
	{
		SQLFreeHandle(SQL_HANDLE_DBC, m_hDBC);
		m_hDBC = NULL;
	}
	if	(m_hEnv)
	{
		SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
		m_hEnv = NULL;
	}
}


void ODBC_CONNECTION::PrintError(
	SQLSMALLINT handletype, 
	SQLHANDLE handle)
{
	SQLCHAR state[MAX_SQL_STATE_LEN];
	SQLINTEGER errorcode;
	SQLCHAR msg[MAX_ERROR_MSG_LEN];
	SQLSMALLINT msglen;

	SQLRETURN result;
	unsigned short ii = 1;
	while ((result = SQLGetDiagRec(handletype, handle, ii, state, &errorcode, msg, sizeof(msg), &msglen)) != SQL_NO_DATA)
	{
		TraceError("SQL ERROR: state(%s)  error(%d)  %s", (char*)state, errorcode, (char*)msg);
		ii++;
	}
}


// ---------------------------------------------------------------------------
// class MSSQL_CONNECTION
// ---------------------------------------------------------------------------

BOOL MSSQL_CONNECTION::Connect(
	LPCTSTR Address, 
	LPCTSTR Server, 
	LPCTSTR User, LPCTSTR Pass, 
	LPCTSTR Database, 
	BOOL Trusted, 
	enumProtocols Proto)
{
	TCHAR str[512]=_T("");
	_stprintf_s(str, sizeof(str), _T("Driver={SQL Server};Address=%s;Server=%s;Uid=%s;Pwd=%s;Trusted_Connection=%s;Database=%s;Network="),
		SAFE_STR(Address), SAFE_STR(Server), SAFE_STR(User), SAFE_STR(Pass), (Trusted ? _T("Yes") : _T("No")), SAFE_STR(Database));
	switch(Proto)
	{
	case protoNamedPipes:
		_tcscat_s(str, sizeof(str), _T("dbnmpntw"));
		break;
	case protoWinSock:
		_tcscat_s(str, sizeof(str), _T("dbmssocn"));
		break;
	case protoIPX:
		_tcscat_s(str, sizeof(str), _T("dbmsspxn"));
		break;
	case protoBanyan:
		_tcscat_s(str, sizeof(str), _T("dbmsvinn"));
		break;
	case protoRPC:
		_tcscat_s(str, sizeof(str), _T("dbmsrpcn"));
		break;
	default:
		_tcscat_s(str, sizeof(str), _T("dbmssocn"));
		break;
	}
	_tcscat_s(str, sizeof(str), _T(";"));
	return ODBC_CONNECTION::Connect(str);
}


// ---------------------------------------------------------------------------
// class ODBC_STATEMENT
// ---------------------------------------------------------------------------

ODBC_STATEMENT::ODBC_STATEMENT(HDBC hDBC)
{
	ASSERT(hDBC);
	if (hDBC == NULL)
	{
		m_hStmt = INVALID_HANDLE_VALUE;
		return;
	}

	SQLRETURN result;
	result = SQLAllocHandle(SQL_HANDLE_STMT, hDBC, &m_hStmt);
	if (SQL_ERR(result))
	{
		m_hStmt = INVALID_HANDLE_VALUE;
		return;
	}
	SQLSetStmtAttr(m_hStmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER)SQL_CONCUR_ROWVER, 0);
	SQLSetStmtAttr(m_hStmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN, 0);
}


/*virtual*/ ODBC_STATEMENT::~ODBC_STATEMENT(void)
{
	if	(m_hStmt != INVALID_HANDLE_VALUE)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, m_hStmt);
	}
}


void ODBC_STATEMENT::PrintError(void)
{
	SQLCHAR state[MAX_SQL_STATE_LEN];
	SQLINTEGER errorcode;
	SQLCHAR msg[MAX_ERROR_MSG_LEN];
	SQLSMALLINT msglen;

	SQLRETURN result;
	unsigned short ii = 1;
	while ((result = SQLGetDiagRec(SQL_HANDLE_STMT, m_hStmt, ii, state, &errorcode, msg, sizeof(msg), &msglen)) != SQL_NO_DATA)
	{
		TraceError("SQL ERROR: state(%s)  error(%d)  %s", (char*)state, errorcode, (char*)msg);
		ii++;
	}
}


BOOL ODBC_STATEMENT::BindInputParameter(
	SQLUSMALLINT parameter,
	SQLSMALLINT value_type,
	SQLSMALLINT param_type,
	SQLPOINTER ptr,
	SQLINTEGER buflen,
	SQLLEN * strlen_ptr,
	SQLUINTEGER column_size,
	SQLSMALLINT decimal_digits)
{
	SQLRETURN result = SQLBindParameter(m_hStmt, parameter, SQL_PARAM_INPUT, value_type, param_type, column_size, 
		decimal_digits, ptr, buflen, strlen_ptr);
	if (SQL_ERR(result))
	{
		PrintError();
		return FALSE;
	}
	return TRUE;
}


USHORT ODBC_STATEMENT::GetColumnCount(void)
{
	short columns = 0;
	if (SQL_ERR(SQLNumResultCols(m_hStmt, &columns)))
	{
		return 0;
	}
	return columns;
}


SQLLEN ODBC_STATEMENT::GetChangedRowCount(void)
{
	SQLLEN rows = 0;
	if (SQL_ERR(SQLRowCount(m_hStmt, &rows)))
	{
		return 0;
	}
	return rows;
}

 
// ---------------------------------------------------------------------------
// class ODBC_RECORD
// ---------------------------------------------------------------------------

USHORT ODBC_RECORD::GetColumnCount(void)
{
	short columns = 0;
	SQLRETURN result = SQLNumResultCols(m_hStmt , &columns);
	if (SQL_ERR(result))
	{
		return 0;
	}
	return columns;	
}


BOOL ODBC_RECORD::BindColumn(
	USHORT column,
	LPVOID buffer,
	ULONG buflen,
	SQLLEN * ret_buflen,
	USHORT type)
{
	SQLLEN ret_len = 0;
	SQLRETURN result = SQLBindCol(m_hStmt, column, type, buffer, buflen, &ret_len);
	if (ret_buflen)
	{
		*ret_buflen = ret_len;
	}
	return SQL_OK(result);
}


USHORT ODBC_RECORD::GetColumnByName(LPCTSTR column)
{
	USHORT columns = GetColumnCount();
	if (columns == 0)
	{
		return 0;
	}
	for (USHORT ii = 0; ii < columns; ++ii)
	{
		TCHAR name[MAX_FIELD_NAME_LEN] = _T("");
		GetColumnName(ii + 1, name, sizeof(name));
		if (_tcsicmp(name, column) == 0)
		{
			return ii + 1;
		}
	}
	return 0;
}


BOOL ODBC_RECORD::GetData(
	USHORT column, 
	LPVOID buffer, 
	ULONG buflen, 
	SQLLEN * datalen, 
	SQLSMALLINT type)
{
	SQLLEN temp = 0;
	SQLRETURN result = SQLGetData(m_hStmt, column, type, buffer, buflen, &temp);
	if	(datalen)
	{
		*datalen = temp;
	}
	return SQL_OK(result);
}


int ODBC_RECORD::GetColumnType(USHORT column)
{
	SQLTCHAR colname[256]=_T("");
	SQLSMALLINT retNameLen = 0, retDataType = 0, retScale = 0, retNull = 0;
	SQLULEN retColSize;
	SQLDescribeCol(m_hStmt, column, colname, sizeof(colname), &retNameLen, &retDataType, &retColSize, &retScale, &retNull);
	return (int)retDataType;
}


UINT ODBC_RECORD::GetColumnSize(USHORT column)
{
	SQLTCHAR colname[256]=_T("");
	SQLSMALLINT retNameLen = 0, retDataType = 0, retScale = 0, retNull = 0;
	SQLULEN retColSize;
	SQLDescribeCol(m_hStmt, column, colname, sizeof(colname), &retNameLen, &retDataType, &retColSize, &retScale, &retNull);
	return (UINT)retColSize;
}


USHORT ODBC_RECORD::GetColumnScale(USHORT column)
{
	SQLTCHAR colname[256]=_T("");
	SQLSMALLINT retNameLen = 0, retDataType = 0, retScale = 0, retNull = 0;
	SQLULEN retColSize;
	SQLDescribeCol(m_hStmt, column, colname, (SQLSMALLINT)sizeof(colname), &retNameLen, &retDataType, &retColSize, &retScale, &retNull);
	return retScale;
}


BOOL ODBC_RECORD::GetColumnName(
	USHORT column,
	LPTSTR name,
	SHORT bufsize)
{
	SQLSMALLINT retNameLen = 0, retDataType = 0, retScale = 0, retNull = 0;
	SQLULEN retColSize;
	SQLRETURN result = SQLDescribeCol(m_hStmt, column, (SQLTCHAR *)name, bufsize, &retNameLen, &retDataType, &retColSize, &retScale, &retNull);
	return SQL_OK(result);
}


BOOL ODBC_RECORD::IsColumnNullable(USHORT column)
{
	SQLTCHAR colname[256]=_T("");
	SQLSMALLINT retNameLen = 0, retDataType = 0, retScale = 0, retNull = 0;
	SQLULEN retColSize;
	SQLDescribeCol(m_hStmt, column, colname, sizeof(colname), &retNameLen, &retDataType, &retColSize, &retScale, &retNull);
	return (retNull == SQL_NULLABLE);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
#define DatabaseTraceError(sql, fmt, ...)		DatabaseTraceErrorDbg(sql, __FILE__, __LINE__, fmt, __VA_ARGS__)
void DatabaseTraceErrorDbg(
	const char * file,
	unsigned int line,
	char * format,
	...)
{
	UNREFERENCED_PARAMETER(line);
	UNREFERENCED_PARAMETER(file);
#else
void DatabaseTraceError(
	char * format,
	...)
{
#endif

	va_list args;
	va_start(args, format);

	vtrace(format, args);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL DatabaseQuery(
	ODBC_STATEMENT & statement,
	const TCHAR * sql_stmt)
{
	if (!statement.Query(sql_stmt))
	{
		statement.PrintError();
		return FALSE;
	}
	return TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void DatabasePrintResult(
	ODBC_STATEMENT & statement)
{
	while (statement.Fetch())
	{
		ODBC_RECORD record(statement);
		USHORT colCount = statement.GetColumnCount();
		ASSERT(colCount < USHRT_MAX - 1);
		for (USHORT ii = 0; ii < colCount; ++ii)
		{
			TCHAR data[512] = _T("");
			SQLLEN datalen = 0;
			record.GetData(ii + 1, data, sizeof(data), &datalen);
			TCHAR name[MAX_FIELD_NAME_LEN]=_T("");
			record.GetColumnName(ii + 1, name, sizeof(name));
		    trace("%15s>  %25s\n", name, data);
		}
	}
}


// ---------------------------------------------------------------------------
// SQL datetime functions
// these functions treat time_t = 0 and SQL_TIMESTAMP_STRUCT = { 0 } as null 
// ---------------------------------------------------------------------------

bool IsSQLTimeStampNull(const SQL_TIMESTAMP_STRUCT& SQLTimeStamp)
{
	return SQLTimeStamp.second == 0 && SQLTimeStamp.minute == 0 && SQLTimeStamp.hour == 0 
		&& SQLTimeStamp.day == 0 && SQLTimeStamp.month == 0 && SQLTimeStamp.year == 0;
}


time_t ConvertSQLTimeStampToTimeT(const SQL_TIMESTAMP_STRUCT& SQLTimeStamp)
{
	if (IsSQLTimeStampNull(SQLTimeStamp))
		return 0;

	tm Tm = { 0 };
	Tm.tm_sec = SQLTimeStamp.second;
	Tm.tm_min = SQLTimeStamp.minute;
	Tm.tm_hour = SQLTimeStamp.hour;
	Tm.tm_mday = SQLTimeStamp.day;
	Tm.tm_mon = SQLTimeStamp.month - 1;
	Tm.tm_year = SQLTimeStamp.year - 1900;
	return _mkgmtime(&Tm);
}


SQL_TIMESTAMP_STRUCT ConvertTimeTToSQLTimeStamp(time_t TimeT)
{
	SQL_TIMESTAMP_STRUCT SQLTimeStamp = { 0 };
	if (TimeT == 0)
		return SQLTimeStamp;

	tm Tm = { 0 };
	//This code is vulnerable: the mail system allows user specified times,
	//and Microsoft's implementation intentionally crashes for times outside
	//the value range.  Thus we have to seperately pre-check the range.
	ASSERT_RETVAL(TimeT > 0 && TimeT < _MAX__TIME64_T, SQLTimeStamp);

	ASSERT(gmtime_s(&Tm, &TimeT) == 0);
	SQLTimeStamp.year = static_cast<SQLSMALLINT>(Tm.tm_year + 1900);
	SQLTimeStamp.month = static_cast<SQLSMALLINT>(Tm.tm_mon + 1);
	SQLTimeStamp.day = static_cast<SQLSMALLINT>(Tm.tm_mday);
	SQLTimeStamp.hour = static_cast<SQLSMALLINT>(Tm.tm_hour);
	SQLTimeStamp.minute = static_cast<SQLSMALLINT>(Tm.tm_min);
	SQLTimeStamp.second = static_cast<SQLSMALLINT>(Tm.tm_sec);
	return SQLTimeStamp;
}


//namespace {
//struct SUnitTestTimeConversion
//{
//	SUnitTestTimeConversion(void)
//	{
//		for (time_t Time = 0; Time < 1000000; ++Time)
//			ASSERT(Time == ConvertSQLTimeStampToTimeT(ConvertTimeTToSQLTimeStamp(Time)));	
//	}
//};
//SUnitTestTimeConversion UnitTestTimeConversion;
//}; // namespace


