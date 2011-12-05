//----------------------------------------------------------------------------
// dbhellgate.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "database.h"
#include "mailslot.h"
#include "dbhellgate.h"
#include "prime.h"
#include "game.h"
#include "s_network.h"


#if ISVERSION(SERVER_VERSION)

// ---------------------------------------------------------------------------
// GLOBALS
// ---------------------------------------------------------------------------
// database connection settings
#define DB_USER						_T("phu")
#define DB_PASSWORD					_T("")
#define DB_HOST						_T("PHU64\\SQLEXPRESS")
#define DB_DATABASE					_T("hellgate")
// thread quit parameters
#define DB_WAITFOREXIT_INTERVAL		100			// milliseconds
#define DB_WAITFOREXIT				5000		// milliseconds
// magic numbers
#define DB_EXITKEY					0x66442211


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct DATABASE
{
	MSSQL_CONNECTION		conn;				// odbc db connection
	MAILSLOT				mailslot_in;		// incoming message mailslot
	HANDLE					signal;				// signal for messages in mailslot
	HANDLE					thread;				// database thread
	BOOL					running;			// is the thread alive?
	NET_COMMAND_TABLE *		clt_cmd_table;		// command table for incoming messages
	NET_COMMAND_TABLE *		srv_cmd_table;		// command table for outgoing messages
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void DatabaseValidateMessageHandlerTable(
	void);

BOOL DatabaseProcessMessage(
	DATABASE * db,
	MSG_STRUCT * msg);


// ---------------------------------------------------------------------------
// register command list
// ---------------------------------------------------------------------------
static BOOL DatabaseCommandTableInit(
	DATABASE * db)
{
	ASSERT(db->clt_cmd_table == NULL);

	DatabaseValidateMessageHandlerTable();

	// server to database message table
	db->clt_cmd_table = NetCommandTableInit(DBCCMD_LAST);
	ASSERT_RETFALSE(db->clt_cmd_table);

#undef  NET_MSG_TABLE_BEGIN
#undef  NET_MSG_TABLE_DEF
#undef  NET_MSG_TABLE_END

#define NET_MSG_TABLE_BEGIN
#define NET_MSG_TABLE_END(c)
#define NET_MSG_TABLE_DEF(c, s, r)		{  MSG_##c msg; MSG_STRUCT_DECL * msg_struct = msg.zz_register_msg_struct(db->clt_cmd_table); \
											NetRegisterCommand(db->clt_cmd_table, c, msg_struct, s, r, static_cast<MFP_PRINT>(&MSG_##c::Print), \
											static_cast<MFP_PUSHPOP>(&MSG_##c::Push), static_cast<MFP_PUSHPOP>(&MSG_##c::Pop)); }

#undef  _DBC_MESSAGE_ENUM_H_
#include "dbhellgate.h"

	NetCommandTableValidate(db->clt_cmd_table);

	// database to server message table
	db->srv_cmd_table = NetCommandTableInit(DBSCMD_LAST);
	ASSERT_RETFALSE(db->srv_cmd_table);

#undef  NET_MSG_TABLE_DEF
#define NET_MSG_TABLE_DEF(c, s, r)		{  MSG_##c msg; MSG_STRUCT_DECL * msg_struct = msg.zz_register_msg_struct(db->clt_cmd_table); \
											NetRegisterCommand(db->srv_cmd_table, c, msg_struct, s, r, static_cast<MFP_PRINT>(&MSG_##c::Print), \
											static_cast<MFP_PUSHPOP>(&MSG_##c::Push), static_cast<MFP_PUSHPOP>(&MSG_##c::Pop)); }

#undef  _DBS_MESSAGE_ENUM_H_
#include "dbhellgate.h"

	NetCommandTableValidate(db->clt_cmd_table);

	return TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void DatabaseCommandTableFree(
	DATABASE * db)
{
	ASSERT_RETURN(db);
	NetCommandTableFree(db->clt_cmd_table);
	db->clt_cmd_table = NULL;

	NetCommandTableFree(db->srv_cmd_table);
	db->srv_cmd_table = NULL;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL DatabaseProcessMessages(
	DATABASE * db)
{
	BOOL retval = TRUE;

	// remove all the messages and process them
	MSG_BUF * head, * tail;
	unsigned int count = MailSlotGetMessages(&db->mailslot_in, head, tail);
	REF(count);

	MSG_BUF * msg = NULL;
	MSG_BUF * next = head;
	while ((msg = next) != NULL)
	{
		next = msg->next;

	    BYTE message[MAX_SMALL_MSG_STRUCT_SIZE];
		if (!NetTranslateMessageForRecv(db->clt_cmd_table, msg->msg, msg->size, (MSG_STRUCT *)message))
		{
			continue;
		}

		if (!DatabaseProcessMessage(db, (MSG_STRUCT *)message))
		{
			retval = FALSE;
			break;
		}
	}

	MailSlotRecycleMessages(&db->mailslot_in, head, tail);

	return retval;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD WINAPI DatabaseThread(
	LPVOID param)
{
	ASSERT_RETZERO(param);

	DATABASE * db = (DATABASE *)param;
	ASSERT_RETZERO(db->signal);

	db->running = TRUE;

	while (1)
	{
		DWORD result = WaitForSingleObject(db->signal, INFINITE);
		ASSERT_BREAK(result == WAIT_OBJECT_0);

		if (!DatabaseProcessMessages(db))
		{
			break;
		}
	}

	db->running = FALSE;

	return 0;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DATABASE * DatabaseInit(
	TCHAR * DatabaseAddress,
	TCHAR * DatabaseServer,
	TCHAR * DatabaseUser,
	TCHAR * DatabasePassword,
	TCHAR * DatabaseDb)
{
	DATABASE * db = (DATABASE *)MALLOCZ(NULL, sizeof(DATABASE));
	ASSERT_RETNULL(db);

	if (!DatabaseCommandTableInit(db))
	{
		goto _error;
	}

	if (!db->conn.Init())
	{
		goto _error;
	}

	MailSlotInit(db->mailslot_in, NULL, MAX_SMALL_MSG_SIZE);

	if ((db->signal = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
	{
		goto _error;
	}

	DWORD dwThreadId;
	db->thread = (HANDLE)CreateThread(NULL,	0, DatabaseThread, db, 0, &dwThreadId);
	if (db->thread == NULL)
	{
		goto _error;
	}

	if (!db->conn.Connect(DatabaseAddress, DatabaseServer, DatabaseUser, DatabasePassword, DatabaseDb))
	{
		goto _error;
	}

	return db;

_error:
	DatabaseFree(db);
	return NULL;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void DatabaseFree(
	DATABASE * db)
{
	if (!db)
	{
		return;
	}

	if (db->thread != NULL)
	{
		MSG_DBCCMD_EXIT msg;
		msg.dwExitKey = DB_EXITKEY;
		ASSERT_RETURN(SendMessageToDatabase(db, DBCCMD_EXIT, &msg));

		// wait for the db thread to exit
		unsigned int iter = 0;
		while (db->running && iter < DB_WAITFOREXIT)
		{
			Sleep(DB_WAITFOREXIT_INTERVAL);
			iter += DB_WAITFOREXIT_INTERVAL;
		}
		CloseHandle(db->thread);
	}

	if (db->signal != NULL)
	{
		CloseHandle(db->signal);
	}

	MailSlotFree(db->mailslot_in);

	db->conn.Free();

	DatabaseCommandTableFree(db);

	FREE(NULL, db);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SendMessageToDatabase(
	DATABASE * db,
	BYTE command,
	MSG_STRUCT * msg)
{
	ASSERT_RETFALSE(db && DatabaseIsConnected(db));
	ASSERT_RETFALSE(db->clt_cmd_table);
	ASSERT_RETFALSE(db->signal);

	BYTE buf_data[MAX_SMALL_MSG_SIZE];
	BYTE_BUF_NET bbuf(buf_data, MAX_SMALL_MSG_SIZE);
	unsigned int size = 0;

	ASSERT_RETFALSE(NetTranslateMessageForSend(db->clt_cmd_table, command, msg, size, bbuf));
	
	MSG_BUF * msgbuf = MailSlotGetBuf(&db->mailslot_in);
	ASSERT_RETFALSE(msgbuf);
	msgbuf->size = size;
	msgbuf->source = MSG_SOURCE_DATABASE;
	msgbuf->idClient = (NETCLIENTID)INVALID_CLIENTID;
	memcpy(msgbuf->msg, buf_data, size);
	ASSERT_RETFALSE(MailSlotAddMsg(&db->mailslot_in, msgbuf));

	SetEvent(db->signal);

	return TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL DatabaseIsConnected(
	DATABASE * db)
{
	ASSERT_RETFALSE(db);
	return (db->conn.GetHDBC() != NULL);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL DatabaseCreateTables(
	DATABASE * db)
{
	ASSERT_RETFALSE(db && DatabaseIsConnected(db));
	ODBC_STATEMENT statement(db->conn.GetHDBC());

	// Accounts table (player accounts)
	ASSERT(ACCT_NAME_FIELD_SIZE == 20);

	static const TCHAR * sql_create_accounts_table = _T("\
		IF NOT EXISTS(SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = 'Accounts') \
		BEGIN \
			CREATE TABLE Accounts (AcctId bigint PRIMARY KEY NONCLUSTERED, \
				AcctName varchar(20) UNIQUE, \
				Password bigint, Active tinyint) \
			CREATE UNIQUE NONCLUSTERED INDEX IX_Accounts_AcctName ON Accounts (AcctName) \
		END");

	if (!DatabaseQuery(statement, sql_create_accounts_table))
	{
		ASSERTX(0, "DATABASE ERROR CREATING ACCOUNTS TABLE");
		return FALSE;
	}

	// Units table (characters & items)
	ASSERT(UNITFILE_MAX_SIZE_SINGLE_UNIT == 500);

/*
	static const TCHAR * sql_create_units_table = _T("\
		IF NOT EXISTS(SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = 'Units') \
		BEGIN \
			CREATE TABLE Units (UnitId bigint PRIMARY KEY NONCLUSTERED, \
				OwnerId bigint FOREIGN KEY REFERENCES Units(UnitId) ON DELETE NO ACTION ON UPDATE NO ACTION, \
				ContainerId bigint FOREIGN KEY REFERENCES Units(UnitId) ON DELETE NO ACTION ON UPDATE NO ACTION, \
				InvLoc int, InvX int, InvY int, Data varbinary (500)) \
			CREATE UNIQUE NONCLUSTERED INDEX IX_Units_OwnerId ON Units (OwnerId) \
		END");
*/

	static const TCHAR * sql_create_units_table = _T("\
		IF NOT EXISTS(SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = 'Units') \
		BEGIN \
			CREATE TABLE Units (UnitId bigint PRIMARY KEY NONCLUSTERED, \
				OwnerId bigint, \
				ContainerId bigint, \
				InvLoc int, InvX int, InvY int, Data varbinary (500)) \
			CREATE NONCLUSTERED INDEX IX_Units_OwnerId ON Units (OwnerId) \
		END");

	if (!DatabaseQuery(statement, sql_create_units_table))
	{
		ASSERTX(0, "DATABASE ERROR CREATING UNITS TABLE");
		return FALSE;
	}

	// Characters table (associates characters to accounts)
	ASSERT(CHAR_NAME_FIELD_SIZE == 20);
	ASSERT(CHAR_WARDROBE_SIZE == 64);

	static const TCHAR * sql_create_characters_table = _T("\
		IF NOT EXISTS(SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = 'Characters') \
		BEGIN \
			CREATE TABLE Characters (CharName varchar(20) PRIMARY KEY NONCLUSTERED, \
				AcctId bigint, \
				UnitId bigint, \
				Class int, CLevel int, Wardrobe varbinary(256)) \
			CREATE NONCLUSTERED INDEX IX_Char_AcctId ON Characters (AcctId) \
			CREATE NONCLUSTERED INDEX IX_Char_UnitId ON Characters (UnitId) \
		END");

	if (!DatabaseQuery(statement, sql_create_characters_table))
	{
		ASSERTX(0, "DATABASE ERROR CREATING CHARACTERS TABLE");
		return FALSE;
	}	

	// Active table (currently logged on accounts/characters)
	static const TCHAR * sql_create_active_table = _T("\
		IF NOT EXISTS(SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = 'Active') \
		BEGIN \
			CREATE TABLE Active (ServerId int, \
				AcctId bigint FOREIGN KEY REFERENCES Accounts(AcctId), \
				UnitId bigint FOREIGN KEY REFERENCES Units(UnitId)) \
			CREATE CLUSTERED INDEX IX_Active_ServerId ON Active (ServerId) \
		END");

	if (!DatabaseQuery(statement, sql_create_active_table))
	{
		ASSERTX(0, "DATABASE ERROR CREATING ACTIVE TABLE");
		return FALSE;
	}	

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sDBCExit(
	DATABASE * db,
	BYTE * data)
{
	MSG_DBCCMD_EXIT * msg = (MSG_DBCCMD_EXIT *)data;
	ASSERT_RETVAL(msg->dwExitKey == DB_EXITKEY, TRUE);
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sDBCAcctLogin(
	DATABASE * db,
	BYTE * data)
{
	MSG_DBCCMD_ACCTLOGIN * msg = (MSG_DBCCMD_ACCTLOGIN *)data;

	ASSERT_RETFALSE(db && DatabaseIsConnected(db));
	ODBC_STATEMENT statement(db->conn.GetHDBC());

	SQLRETURN result;
	result = SQLPrepare(statement, (SQLCHAR *)_T("SELECT AcctId, Password, Active FROM Accounts Where AcctName = ?"), SQL_NTS); 
	if (SQL_ERR(result))
	{
		statement.PrintError();
		return FALSE;
	}

	SQLCHAR		szAcctName[MAX_CHARACTER_NAME];
	SQLLEN		acctnamelen;

	ASSERT(sizeof(SQLCHAR) == sizeof(char));
	ASSERT_RETFALSE(statement.BindInputParameter(1, SQL_C_CHAR, SQL_VARCHAR, szAcctName, sizeof(szAcctName), &acctnamelen, sizeof(szAcctName), 0));
	acctnamelen = PStrCopy((char *)szAcctName, sizeof(szAcctName), msg->szAcctName, sizeof(msg->szAcctName));

	result = SQLExecute(statement);
	if (SQL_ERR(result))
	{
		statement.PrintError();
		ASSERTX(0, "Error executing SQL INSERT");
		return FALSE;
	}

	if (!statement.Fetch())
	{
		// return a message saying account not found
		MSG_DBSCMD_ACCTLOGIN response;
		response.idClient = msg->idClient;
		PStrCopy(response.szAcctName, (char *)szAcctName, sizeof(response.szAcctName));
		response.nNameLen = (BYTE)acctnamelen;
		response.dwErrorCode = LOGINERROR_ACCOUNT_NOT_FOUND;

		ASSERTX(0, "Functionality will need to be added for this type of mailbox posting to work..." );
		//AppPostMessageToMailbox(db->srv_cmd_table, MSG_SOURCE_DATABASE, msg->idClient, DBSCMD_ACCTLOGIN, &response);

		return TRUE;
	}

	while (statement.Fetch())
	{
		ODBC_RECORD record(statement);
		for (unsigned int ii = 0; ii < statement.GetColumnCount(); ++ii)
		{
			TCHAR data[512] = _T("");
			SQLLEN datalen = 0;
			record.GetData((USHORT)(ii + 1), data, sizeof(data), &datalen);
			TCHAR name[MAX_FIELD_NAME_LEN]=_T("");
			record.GetColumnName((USHORT)(ii + 1), name, sizeof(name));
		    trace("%15s>  %25s\n", name, data);
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sDBCNewUnit(
	DATABASE * db,
	BYTE * data)
{
	MSG_DBCCMD_NEWUNIT * msg = (MSG_DBCCMD_NEWUNIT *)data;

	ASSERT_RETFALSE(db && DatabaseIsConnected(db));
	ODBC_STATEMENT statement(db->conn.GetHDBC());

	SQLRETURN result;
	result = SQLPrepare(statement, (SQLCHAR *)_T("INSERT INTO Units (UnitId, OwnerId, ContainerId, InvLoc, InvX, InvY, Data) VALUES (?, ?, ?, ?, ?, ?, ?)"), SQL_NTS); 
	if (SQL_ERR(result))
	{
		statement.PrintError();
		return FALSE;
	}

	SQLBIGINT	idUnit;
	SQLBIGINT	idOwner;
	SQLBIGINT	idContainer;
	SQLINTEGER	nInvLoc;
	SQLINTEGER	nInvX;
	SQLINTEGER	nInvY;
	SQLCHAR		buf[UNITFILE_MAX_SIZE_SINGLE_UNIT];
	SQLLEN		buflen;

	ASSERT_RETFALSE(statement.BindInputParameter(1, SQL_C_SBIGINT, SQL_BIGINT, &idUnit, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(2, SQL_C_SBIGINT, SQL_BIGINT, &idOwner, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(3, SQL_C_SBIGINT, SQL_BIGINT, &idContainer, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(4, SQL_C_SLONG, SQL_INTEGER, &nInvLoc, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(5, SQL_C_SLONG, SQL_INTEGER, &nInvX, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(6, SQL_C_SLONG, SQL_INTEGER, &nInvY, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(7, SQL_C_BINARY, SQL_BINARY, buf, sizeof(buf), &buflen, sizeof(buf), 0));
	idUnit = msg->idUnit;
	idOwner = msg->idOwner;
	idContainer = msg->idContainer;
	nInvLoc = msg->nInvLoc;
	nInvX = msg->nInvX;
	nInvY = msg->nInvY;
	buflen = MSG_GET_BLOB_LEN(msg, buf);
	MemoryCopy(buf, UNITFILE_MAX_SIZE_SINGLE_UNIT, msg->buf, buflen);

	result = SQLExecute(statement);
	if (SQL_ERR(result))
	{
		statement.PrintError();
		ASSERTX(0, "Error executing SQL INSERT");
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sDBCDeleteUnit(
	DATABASE * db,
	BYTE * data)
{
	MSG_DBCCMD_DELETEUNIT * msg = (MSG_DBCCMD_DELETEUNIT *)data;

	ASSERT_RETFALSE(db && DatabaseIsConnected(db));
	ODBC_STATEMENT statement(db->conn.GetHDBC());

	SQLRETURN result;
	result = SQLPrepare(statement, (SQLCHAR *)_T("DELETE Units WHERE UnitId = ? OR OwnerId = ? OR ContainerId = ?"), SQL_NTS);
	if (SQL_ERR(result))
	{
		statement.PrintError();
		return FALSE;
	}

	SQLBIGINT	idUnit;
	SQLBIGINT	idOwner;
	SQLBIGINT	idContainer;

	ASSERT_RETFALSE(statement.BindInputParameter(1, SQL_C_SBIGINT, SQL_BIGINT, &idUnit, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(2, SQL_C_SBIGINT, SQL_BIGINT, &idOwner, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(3, SQL_C_SBIGINT, SQL_BIGINT, &idContainer, 0, NULL, 0, 0));
	idUnit = msg->idUnit;
	idOwner = msg->idUnit;
	idContainer = msg->idUnit;

	result = SQLExecute(statement);
	if (SQL_ERR(result))
	{
		statement.PrintError();
		ASSERTX(0, "Error executing SQL DELETE");
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sDBCUpdateUnitLoc(
	DATABASE * db,
	BYTE * data)
{
	MSG_DBCCMD_UPDATEUNIT_LOC * msg = (MSG_DBCCMD_UPDATEUNIT_LOC *)data;

	ASSERT_RETFALSE(db && DatabaseIsConnected(db));
	ODBC_STATEMENT statement(db->conn.GetHDBC());

	SQLRETURN result;
	result = SQLPrepare(statement, (SQLCHAR *)_T("UPDATE Units SET OwnerId = ?, ContainerId = ?, InvLoc = ?, InvX = ?, InvY = ? WHERE UnitId = ?"), SQL_NTS);
	if (SQL_ERR(result))
	{
		statement.PrintError();
		return FALSE;
	}

	SQLBIGINT	idUnit;
	SQLBIGINT	idOwner;
	SQLBIGINT	idContainer;
	SQLINTEGER	nInvLoc;
	SQLINTEGER	nInvX;
	SQLINTEGER	nInvY;

	ASSERT_RETFALSE(statement.BindInputParameter(1, SQL_C_SBIGINT, SQL_BIGINT, &idOwner, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(2, SQL_C_SBIGINT, SQL_BIGINT, &idContainer, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(3, SQL_C_SLONG, SQL_INTEGER, &nInvLoc, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(4, SQL_C_SLONG, SQL_INTEGER, &nInvX, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(5, SQL_C_SLONG, SQL_INTEGER, &nInvY, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(6, SQL_C_SBIGINT, SQL_BIGINT, &idUnit, 0, NULL, 0, 0));
	idUnit = msg->idUnit;
	idOwner = msg->idOwner;
	idContainer = msg->idContainer;
	nInvLoc = msg->nInvLoc;
	nInvX = msg->nInvX;
	nInvY = msg->nInvY;

	result = SQLExecute(statement);
	if (SQL_ERR(result))
	{
		statement.PrintError();
		ASSERTX(0, "Error executing SQL UPDATE");
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sDBCUpdateUnitData(
	DATABASE * db,
	BYTE * data)
{
	MSG_DBCCMD_UPDATEUNIT_DATA * msg = (MSG_DBCCMD_UPDATEUNIT_DATA *)data;

	ASSERT_RETFALSE(db && DatabaseIsConnected(db));
	ODBC_STATEMENT statement(db->conn.GetHDBC());

	SQLRETURN result;
	result = SQLPrepare(statement, (SQLCHAR *)_T("UPDATE Units SET OwnerId = ?, ContainerId = ?, InvLoc = ?, InvX = ?, InvY = ?, Data = ? WHERE UnitId = ?"), SQL_NTS);
	if (SQL_ERR(result))
	{
		statement.PrintError();
		return FALSE;
	}

	SQLBIGINT	idUnit;
	SQLBIGINT	idOwner;
	SQLBIGINT	idContainer;
	SQLINTEGER	nInvLoc;
	SQLINTEGER	nInvX;
	SQLINTEGER	nInvY;
	SQLCHAR		buf[UNITFILE_MAX_SIZE_SINGLE_UNIT];
	SQLLEN		buflen;

	ASSERT_RETFALSE(statement.BindInputParameter(1, SQL_C_SBIGINT, SQL_BIGINT, &idOwner, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(2, SQL_C_SBIGINT, SQL_BIGINT, &idContainer, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(3, SQL_C_SLONG, SQL_INTEGER, &nInvLoc, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(4, SQL_C_SLONG, SQL_INTEGER, &nInvX, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(5, SQL_C_SLONG, SQL_INTEGER, &nInvY, 0, NULL, 0, 0));
	ASSERT_RETFALSE(statement.BindInputParameter(6, SQL_C_BINARY, SQL_BINARY, buf, sizeof(buf), &buflen, sizeof(buf), 0));
	ASSERT_RETFALSE(statement.BindInputParameter(7, SQL_C_SBIGINT, SQL_BIGINT, &idUnit, 0, NULL, 0, 0));
	idUnit = msg->idUnit;
	idOwner = msg->idOwner;
	idContainer = msg->idContainer;
	nInvLoc = msg->nInvLoc;
	nInvX = msg->nInvX;
	nInvY = msg->nInvY;
	buflen = MSG_GET_BLOB_LEN(msg, buf);
	MemoryCopy(buf, UNITFILE_MAX_SIZE_SINGLE_UNIT, msg->buf, buflen);

	result = SQLExecute(statement);
	if (SQL_ERR(result))
	{
		statement.PrintError();
		ASSERTX(0, "Error executing SQL UPDATE");
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef BOOL FP_PROCESS_DBC_MESSAGE(DATABASE * db, BYTE * data);

struct PROCESS_DBC_MESSAGE_STRUCT
{
	NET_CMD						cmd;
	FP_PROCESS_DBC_MESSAGE *	fp;
	const char *				szFuncName;	
};

#define SRV_MESSAGE_PROC(c, f)		{ c, f, #f },

PROCESS_DBC_MESSAGE_STRUCT gfpDatabaseClientMessageHandler[] = 
{
	SRV_MESSAGE_PROC(DBCCMD_EXIT,						sDBCExit)
	SRV_MESSAGE_PROC(DBCCMD_ACCTLOGIN,					sDBCAcctLogin)
	SRV_MESSAGE_PROC(DBCCMD_NEWUNIT,					sDBCNewUnit)
	SRV_MESSAGE_PROC(DBCCMD_DELETEUNIT,					sDBCDeleteUnit)
	SRV_MESSAGE_PROC(DBCCMD_UPDATEUNIT_LOC,				sDBCUpdateUnitLoc)
	SRV_MESSAGE_PROC(DBCCMD_UPDATEUNIT_DATA,			sDBCUpdateUnitData)
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL DatabaseProcessMessage(
	DATABASE * db,
	MSG_STRUCT * msg)
{
	NET_CMD command = msg->hdr.cmd;

	ASSERT_RETFALSE(command >= 0 && command < DBCCMD_LAST);
	ASSERT_RETFALSE(gfpDatabaseClientMessageHandler[command].fp);

	NetTraceRecvMessage(db->clt_cmd_table, msg, "DBCRCV: ");

	//// perf debugging
	//#define MAX_PERF_TITLE (256)
	//char szPerfTitle[MAX_PERF_TITLE];
	//sprintf(szPerfTitle, "cProcessMessage - %s", gfpDatabaseClientMessageHandler[command].szFuncName);
	//TIMER_START(szPerfTitle)

	return gfpDatabaseClientMessageHandler[command].fp(db, (BYTE*)msg);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL DatabaseShowTables(
	DATABASE * db)
{
	ASSERT_RETFALSE(db && DatabaseIsConnected(db));

	ODBC_STATEMENT statement(db->conn.GetHDBC());

//	SQLExecDirect(statement, (SQLTCHAR *)_T("USE hellgate"), SQL_NTS);
	static const TCHAR * sql_stmt = _T("SELECT * FROM Accounts;");
//	static const TCHAR * sql_stmt = _T("SELECT * FROM Accounts WHERE Name = 'phu';");
//	static const TCHAR * sql_stmt = _T("INSERT INTO Accounts (Name, Id) VALUES('test', 1);");

	if (!statement.Query(sql_stmt))
	{
		statement.PrintError();
		return FALSE;
	}
	while (statement.Fetch())
	{
		ODBC_RECORD record(statement);
		for (unsigned int ii = 0; ii < statement.GetColumnCount(); ++ii)
		{
			TCHAR data[512] = _T("");
			SQLLEN datalen = 0;
			record.GetData((USHORT)(ii + 1), data, sizeof(data), &datalen);
			TCHAR name[MAX_FIELD_NAME_LEN]=_T("");
			record.GetColumnName((USHORT)(ii + 1), name, sizeof(name));
		    printf("%15s>  %25s\n", name, data);
		}
	}

	return TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL DatabaseSendAcctLogin(
	DATABASE * db,
	NETCLIENTID idClient,
	char * szAcctName,
	DWORD dwPasswordHash)
{
	ASSERT_RETFALSE(db);

	MSG_DBCCMD_ACCTLOGIN msg;
	msg.idClient = idClient;
	msg.nNameLen = (BYTE)PStrCopy(msg.szAcctName, MAX_CHARACTER_NAME, szAcctName, MAX_CHARACTER_NAME);
	msg.dwPasswordHash = dwPasswordHash;

	return SendMessageToDatabase(db, DBCCMD_ACCTLOGIN, &msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL DatabaseSendNewUnit(
	DATABASE * db,
	PGUID idUnit,
	PGUID idOwner,
	PGUID idContainer,
	int nInvLoc,
	int nInvX,
	int nInvY,
	BYTE * buf,
	unsigned int bufsize)
{
	ASSERT_RETFALSE(db);

	MSG_DBCCMD_NEWUNIT msg;
	msg.idUnit = idUnit;
	msg.idOwner = idOwner;
	msg.idContainer = idContainer;
	msg.nInvLoc = nInvLoc;
	msg.nInvX = nInvX;
	msg.nInvY = nInvY;
	MemoryCopy(msg.buf, UNITFILE_MAX_SIZE_SINGLE_UNIT, buf, bufsize);
	MSG_SET_BLOB_LEN(msg, buf, bufsize);

	return SendMessageToDatabase(db, DBCCMD_NEWUNIT, &msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sDBSAcctLogin(
	GAME * game,
	NETCLIENTID idNetClient,
	BYTE * data)
{
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef void FP_PROCESS_DBS_MESSAGE(GAME * game, NETCLIENTID idNetClient, BYTE * data);

struct PROCESS_DBS_MESSAGE_STRUCT
{
	NET_CMD						cmd;				// command
	FP_PROCESS_DBS_MESSAGE *	func;				// handler
};

#define DBS_MESSAGE_PROC(commmand, function) { commmand, function },

PROCESS_DBS_MESSAGE_STRUCT gfpDatabaseServerMessageHandler[] =
{	
	DBS_MESSAGE_PROC(DBSCMD_ACCTLOGIN,					sDBSAcctLogin)
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void DatabaseValidateMessageHandlerTable(
	void)
{
	ASSERT(arrsize(gfpDatabaseClientMessageHandler) == DBCCMD_LAST);
	for (unsigned int ii = 0; ii < arrsize(gfpDatabaseClientMessageHandler); ii++)
	{
		ASSERT(gfpDatabaseClientMessageHandler[ii].cmd == ii);
	}

	ASSERT(arrsize(gfpDatabaseServerMessageHandler) == DBSCMD_LAST);
	for (unsigned int ii = 0; ii < arrsize(gfpDatabaseServerMessageHandler); ii++)
	{
		ASSERT(gfpDatabaseServerMessageHandler[ii].cmd == ii);
	}
}

#endif

