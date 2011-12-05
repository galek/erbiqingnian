//----------------------------------------------------------------------------
// s_playerEmailDB.cpp
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "svrstd.h"
#include "s_playerEmail.h"
#include "s_message.h"
#include "c_message.h"
#include "ChatServer.h"
#include "game.h"
#include "clients.h"
#include "units.h"
#include "GameChatCommunication.h"
#include "GameServer.h"
#include "inventory.h"
#include "economy.h"
#include "GlobalIndex.h"
#include "gameglobals.h"
#include "dbunit.h"
#include "database.h"
#include "DatabaseManager.h"
#include "gamelist.h"
#include "items.h"
#include "email.h"
#include "utilitygame.h"

#if ISVERSION(SERVER_VERSION)
#include "playertrace.h"
#include "s_playerEmailDB.h"
#include "s_playerEmailDB.cpp.tmh"
#include "s_auctionNetwork.h"


//----------------------------------------------------------------------------
// MOVE ITEM INTO ESCROW
//----------------------------------------------------------------------------

DB_REQUEST_RESULT PlayerEmailDBMoveItemTreeIntoEscrow::DoSingleRequest(
	HDBC hDBC,
	DataType* pRequestData)
{
	ASSERT_RETX(hDBC && pRequestData, DB_REQUEST_RESULT_FAILED);
	DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;

	SQLRETURN hRet;
	SQLLEN sqlnts = SQL_NTS;
	LONG spRet = 0;

	// the function params:
	//sp_move_item_tree_into_escrow
	//	@email_id AS BIGINT,
	//	@sender_character_id AS BIGINT,
	//	@target_character_name AS NVARCHAR(50),
	//	@escrow_location_code AS INT,
	//	@item_tree_root AS BIGINT,
	//	@item_tree_leaf_1 AS BIGINT,
	//	@item_tree_leaf_2 AS BIGINT,
	//	@item_tree_leaf_3 AS BIGINT,
	//	@item_tree_leaf_4 AS BIGINT,
	//	@item_tree_leaf_5 AS BIGINT,
	//	@item_tree_leaf_6 AS BIGINT,
	//	@item_tree_leaf_7 AS BIGINT,
	//	@item_tree_leaf_8 AS BIGINT,
	//	@item_tree_leaf_9 AS BIGINT,
	//	@item_tree_leaf_10 AS BIGINT
	
	const char * szStatement = "{ ? = CALL sp_move_item_tree_into_escrow(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) }";
	CScopedSQLStmt hStmt(hDBC, szStatement);
	ASSERT_GOTO(hStmt, _DONE);

	hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_OUTPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &spRet, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->MessageId, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->CurrentOwnerCharacterId, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, MAX_CHARACTER_NAME, 0, pRequestData->NewOwnerCharacterName, 0, &sqlnts);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pRequestData->NewInventoryLocation, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 6, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->RootItemId, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	for (UINT ii = 0; ii < arrsize(pRequestData->ItemTreeNodes); ++ii)
	{
		if (ii >= pRequestData->ItemTreeNodeCount)
			pRequestData->ItemTreeNodes[ii] = INVALID_CLUSTERGUID;

		hRet = SQLBindParameter(hStmt, 7 + ii, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->ItemTreeNodes[ii], 0, 0);
		ASSERT_GOTO(SQL_OK(hRet), _DONE);
	}

	hRet = SQLExecute(hStmt);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	toRet = DB_REQUEST_RESULT_SUCCESS;

_DONE:
	if (toRet != DB_REQUEST_RESULT_SUCCESS)
	{
		TraceError("Item Email DB Error. Statement: %s, SQL Error: %s", szStatement, GetSQLErrorStr(hStmt).c_str());
		TraceError(" DB Parameters: %#018I64x, %#018I64x, %S, %u, %#018I64x", pRequestData->MessageId, pRequestData->CurrentOwnerCharacterId, pRequestData->NewOwnerCharacterName, pRequestData->NewInventoryLocation, pRequestData->RootItemId);
	}

	return (toRet == DB_REQUEST_RESULT_SUCCESS && spRet == 0) ? DB_REQUEST_RESULT_SUCCESS : DB_REQUEST_RESULT_FAILED;
}

DB_REQUEST_BATCHER<PlayerEmailDBMoveItemTreeIntoEscrow>::_BATCHER_DATA DB_REQUEST_BATCHER<PlayerEmailDBMoveItemTreeIntoEscrow>::s_dat;


//----------------------------------------------------------------------------
// REMOVE ITEM FROM ESCROW
//----------------------------------------------------------------------------

DB_REQUEST_RESULT PlayerEmailDBRemoveItemTreeFromEscrow::DoSingleRequest(
	HDBC hDBC,
	DataType* pRequestData)
{
	ASSERT_RETX(hDBC && pRequestData, DB_REQUEST_RESULT_FAILED);
	DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;

	SQLRETURN hRet;
	LONG spRet = 0;

	const char * szStatement = "{ ? = CALL sp_remove_item_tree_from_escrow(?,?,?,?) }";
	CScopedSQLStmt hStmt(hDBC, szStatement);
	ASSERT_GOTO(hStmt, _DONE);

	hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_OUTPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &spRet, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->MessageId, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->CharacterId, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pRequestData->NewInventoryLocation, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->RootItemId, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLExecute(hStmt);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	toRet = DB_REQUEST_RESULT_SUCCESS;

_DONE:
	if (toRet != DB_REQUEST_RESULT_SUCCESS)
	{
		TraceError("Item Email DB Error. Statement: %s, SQL Error: %s", szStatement, GetSQLErrorStr(hStmt).c_str());
		TraceError(" DB Parameters: %#018I64x, %#018I64x, %u, %#018I64x", pRequestData->MessageId, pRequestData->CharacterId, pRequestData->NewInventoryLocation, pRequestData->RootItemId);
	}

	return (toRet == DB_REQUEST_RESULT_SUCCESS && spRet == 0) ? DB_REQUEST_RESULT_SUCCESS : DB_REQUEST_RESULT_FAILED;
}

DB_REQUEST_BATCHER<PlayerEmailDBRemoveItemTreeFromEscrow>::_BATCHER_DATA DB_REQUEST_BATCHER<PlayerEmailDBRemoveItemTreeFromEscrow>::s_dat;


//----------------------------------------------------------------------------
// MOVE MONEY INTO ESCROW
//----------------------------------------------------------------------------

DB_REQUEST_RESULT PlayerEmailDBMoveMoneyIntoEscrow::DoSingleRequest(
	HDBC hDBC,
	DataType* pRequestData)
{
	ASSERT_RETX(hDBC && pRequestData, DB_REQUEST_RESULT_FAILED);
	DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;

	SQLRETURN hRet;
	SQLLEN sqlnts = SQL_NTS;
	LONG spRet = 0;

	const char * szStatement = "{ ? = CALL sp_move_money_into_escrow(?,?,?,?,?,?) }";
	CScopedSQLStmt hStmt(hDBC, szStatement);
	ASSERT_GOTO(hStmt, _DONE);

	hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_OUTPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &spRet, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->MessageId, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->CurrentOwnerCharacterId, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, MAX_CHARACTER_NAME, 0, pRequestData->NewOwnerCharacterName, 0, &sqlnts);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pRequestData->NewInventoryLocation, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 6, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pRequestData->MoneyAmmount, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 7, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->MoneyUnitId, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLExecute(hStmt);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	toRet = DB_REQUEST_RESULT_SUCCESS;

_DONE:
	if (toRet != DB_REQUEST_RESULT_SUCCESS)
	{
		TraceError("Money email DB Error. Statement: %s, SQL Error: %s", szStatement, GetSQLErrorStr(hStmt).c_str());
		TraceError(" DB Parameters: %#018I64x, %#018I64x, %S, %u, %u, %#018I64x", pRequestData->MessageId, pRequestData->CurrentOwnerCharacterId, pRequestData->NewOwnerCharacterName, pRequestData->NewInventoryLocation, pRequestData->MoneyAmmount, pRequestData->MoneyUnitId);
	}

	return (toRet == DB_REQUEST_RESULT_SUCCESS && spRet == 0) ? DB_REQUEST_RESULT_SUCCESS : DB_REQUEST_RESULT_FAILED;
}

DB_REQUEST_BATCHER<PlayerEmailDBMoveMoneyIntoEscrow>::_BATCHER_DATA DB_REQUEST_BATCHER<PlayerEmailDBMoveMoneyIntoEscrow>::s_dat;


//----------------------------------------------------------------------------
// REMOVE MONEY FROM ESCROW
//----------------------------------------------------------------------------

DB_REQUEST_RESULT PlayerEmailDBRemoveMoneyFromEscrow::DoSingleRequest(
	HDBC hDBC,
	DataType* pRequestData)
{
	ASSERT_RETX(hDBC && pRequestData, DB_REQUEST_RESULT_FAILED);
	DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;

	SQLRETURN hRet;
	LONG spRet = 0;

	const char * szStatement = "{ ? = CALL sp_remove_money_from_escrow(?,?,?) }";
	CScopedSQLStmt hStmt(hDBC, szStatement);
	ASSERT_GOTO(hStmt, _DONE);

	hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_OUTPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &spRet, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->MessageId, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->CharacterId, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &pRequestData->MoneyUnitId, 0, 0);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	hRet = SQLExecute(hStmt);
	ASSERT_GOTO(SQL_OK(hRet), _DONE);

	toRet = DB_REQUEST_RESULT_SUCCESS;

_DONE:
	if (toRet != DB_REQUEST_RESULT_SUCCESS)
	{
		TraceError("Money email DB Error. Statement: %s, SQL Error: %s", szStatement, GetSQLErrorStr(hStmt).c_str());
		TraceError(" DB Parameters: %#018I64x, %#018I64x, %#018I64x", pRequestData->MessageId, pRequestData->CharacterId, pRequestData->MoneyUnitId);
	}

	return (toRet == DB_REQUEST_RESULT_SUCCESS && spRet == 0) ? DB_REQUEST_RESULT_SUCCESS : DB_REQUEST_RESULT_FAILED;
}

DB_REQUEST_BATCHER<PlayerEmailDBRemoveMoneyFromEscrow>::_BATCHER_DATA DB_REQUEST_BATCHER<PlayerEmailDBRemoveMoneyFromEscrow>::s_dat;


#endif	//	ISVERSION(SERVER_VERSION)
