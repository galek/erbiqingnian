// ---------------------------------------------------------------------------
// dbhellgate.h
// ---------------------------------------------------------------------------
#ifndef _DBHELLGATE_H_
#define _DBHELLGATE_H_

#if ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef __UNITFILE_H_
#include "unitfile.h"
#endif

#ifndef __DB_UNIT_H_
#include "dbunit.h"
#endif


// ---------------------------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------------------------
#define MAX_DB_MSG_SIZE				1024
#define ACCT_NAME_FIELD_SIZE		20
#define CHAR_NAME_FIELD_SIZE		20
#define CHAR_WARDROBE_SIZE			64


// ---------------------------------------------------------------------------
// ERROR CODES
// ---------------------------------------------------------------------------
#define LOGINERROR_ACCOUNT_NOT_FOUND	1


// ---------------------------------------------------------------------------
// FORWARD DECLARATION
// ---------------------------------------------------------------------------
struct DATABASE;


// ---------------------------------------------------------------------------
// EXPORTED FUNCTIONS
// ---------------------------------------------------------------------------
DATABASE * DatabaseInit(
	TCHAR * DatabaseAddress,
	TCHAR * DatabaseServer,
	TCHAR * DatabaseUser,
	TCHAR * DatabasePassword,
	TCHAR * DatabaseDb);

void DatabaseFree(
	DATABASE * db);

BOOL DatabaseShowTables(
	DATABASE * db);

BOOL SendMessageToDatabase(
	DATABASE * db,
	BYTE command,
	MSG_STRUCT * msg);

BOOL DatabaseIsConnected(
	DATABASE * db);

BOOL DatabaseCreateTables(
	DATABASE * db);

BOOL DatabaseSendAcctLogin(
	DATABASE * db,
	NETCLIENTID idClient,
	WCHAR szAcctName,
	DWORD dwPasswordHash);

BOOL DatabaseSendNewUnit(
	DATABASE * db,
	PGUID idUnit,
	PGUID idOwner,
	PGUID idContainer,
	int nInvLoc,
	int nInvX,
	int nInvY,
	BYTE * buf,
	unsigned int bufsize);


//----------------------------------------------------------------------------
// MESSAGE STRUCTURES	(server to database messages)
//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_DBCCMD_EXIT)
	MSG_FIELD(0, DWORD, dwExitKey, INVALID_ID)
	END_MSG_STRUCT


DEF_MSG_STRUCT(MSG_DBCCMD_ACCTLOGIN)
	MSG_FIELD(0, NETCLIENTID, idClient, INVALID_CLIENTID)
	MSG_CHAR (1, szAcctName, MAX_CHARACTER_NAME)
	MSG_FIELD(2, BYTE, nNameLen, 0)
	MSG_FIELD(3, DWORD, dwPasswordHash, 0)
	END_MSG_STRUCT


DEF_MSG_STRUCT(MSG_DBCCMD_NEWUNIT)
	MSG_FIELD(0, PGUID, idUnit, INVALID_GUID)
	MSG_FIELD(1, PGUID, idOwner, INVALID_GUID)
	MSG_FIELD(2, PGUID, idContainer, INVALID_GUID)
	MSG_FIELD(3, int, nInvLoc, 0)
	MSG_FIELD(4, int, nInvX, 0)
	MSG_FIELD(5, int, nInvY, 0)
	MSG_BLOBW(6, buf, UNITFILE_MAX_SIZE_SINGLE_UNIT)
	END_MSG_STRUCT


DEF_MSG_STRUCT(MSG_DBCCMD_DELETEUNIT)
	MSG_FIELD(0, PGUID, idUnit, INVALID_GUID)
	END_MSG_STRUCT


DEF_MSG_STRUCT(MSG_DBCCMD_UPDATEUNIT_LOC)
	MSG_FIELD(0, PGUID, idUnit, INVALID_GUID)
	MSG_FIELD(1, PGUID, idOwner, INVALID_GUID)
	MSG_FIELD(2, PGUID, idContainer, INVALID_GUID)
	MSG_FIELD(3, int, nInvLoc, 0)
	MSG_FIELD(4, int, nInvX, 0)
	MSG_FIELD(5, int, nInvY, 0)
	END_MSG_STRUCT


DEF_MSG_STRUCT(MSG_DBCCMD_UPDATEUNIT_DATA)
	MSG_FIELD(0, PGUID, idUnit, INVALID_GUID)
	MSG_FIELD(1, PGUID, idOwner, INVALID_GUID)
	MSG_FIELD(2, PGUID, idContainer, INVALID_GUID)
	MSG_FIELD(3, int, nInvLoc, 0)
	MSG_FIELD(4, int, nInvX, 0)
	MSG_FIELD(5, int, nInvY, 0)
	MSG_BLOBW(6, buf, UNITFILE_MAX_SIZE_SINGLE_UNIT)
	END_MSG_STRUCT


//----------------------------------------------------------------------------
// MESSAGE STRUCTURES	(database to server messages)
//----------------------------------------------------------------------------

DEF_MSG_STRUCT(MSG_DBSCMD_ACCTLOGIN)
	MSG_FIELD(0, NETCLIENTID, idClient, INVALID_NETCLIENTID)
	MSG_CHAR (1, szAcctName, MAX_CHARACTER_NAME)
	MSG_FIELD(2, BYTE, nNameLen, 0)
	MSG_FIELD(3, DWORD, dwErrorCode, 0)
	END_MSG_STRUCT

#else

struct DATABASE;

inline void DatabaseFree(
	DATABASE * db)
{
	REF(db);
}

inline DATABASE * DatabaseInit(
	TCHAR * DatabaseAddress,
	TCHAR * DatabaseServer,
	TCHAR * DatabaseUser,
	TCHAR * DatabasePassword,
	TCHAR * DatabaseDb)
{
	REF(DatabaseAddress);
	REF(DatabaseServer);
	REF(DatabaseUser);
	REF(DatabasePassword);
	REF(DatabaseDb);
	return NULL;
}

inline BOOL DatabaseCreateTables(
	DATABASE * db)
{
	REF(db);
	return FALSE;
}

#endif // ISVERSION(SERVER_VERSION)

#endif // _DBHELLGATE_H_


#if ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
// MESSAGE ENUMERATIONS  (server to database messages)
//----------------------------------------------------------------------------
#ifndef _DBC_MESSAGE_ENUM_H_
#define _DBC_MESSAGE_ENUM_H_


NET_MSG_TABLE_BEGIN
	// command												sendlog	recvlog
	NET_MSG_TABLE_DEF(DBCCMD_EXIT,							TRUE,	TRUE)
	NET_MSG_TABLE_DEF(DBCCMD_ACCTLOGIN,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(DBCCMD_NEWUNIT,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(DBCCMD_DELETEUNIT,					TRUE,	TRUE)
	NET_MSG_TABLE_DEF(DBCCMD_UPDATEUNIT_LOC,				TRUE,	TRUE)
	NET_MSG_TABLE_DEF(DBCCMD_UPDATEUNIT_DATA,				TRUE,	TRUE)
NET_MSG_TABLE_END(DBCCMD_LAST)


#endif // _DBC_MESSAGE_ENUM_H_

#endif	// ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
// MESSAGE ENUMERATIONS  (database to server messages)
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)

#ifndef _DBS_MESSAGE_ENUM_H_
#define _DBS_MESSAGE_ENUM_H_

NET_MSG_TABLE_BEGIN
	// command												sendlog	recvlog
	NET_MSG_TABLE_DEF(DBSCMD_ACCTLOGIN,						TRUE,	TRUE)
NET_MSG_TABLE_END(DBSCMD_LAST)


#endif // _DBS_MESSAGE_ENUM_H_

#endif // ISVERSION(SERVER_VERSION)


