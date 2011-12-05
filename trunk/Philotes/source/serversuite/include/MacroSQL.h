#ifndef _MACRO_SQL_H_
#define _MACRO_SQL_H_
//General, not on-off macros.
#define MACRO_SQL_NO_BATCH(x) \
	static const DWORD MaxBatchWaitTime = INFINITE; \
	static const UINT  PartialBatchSize = 0; \
	static const UINT  FullBatchSize    = 0; \
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * ) \
	{ \
		ASSERT_MSG( #x " does not support batched requests!"); \
		return DB_REQUEST_RESULT_FAILED; \
	} \
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )  \
	{ \
		ASSERT_MSG( #x " does not support batched requests!"); \
		return DB_REQUEST_RESULT_FAILED; \
	}

#endif//_MACRO_SQL_H_

#ifdef MACRO_SQL_MSG_DEF
#undef SQL_MACRO_START
#undef SQL_MACRO_INT
#undef SQL_MACRO_BIGINT
#undef SQL_MACRO_END
#define SQL_MACRO_START(name) DEF_MSG_STRUCT(MSG_##name)
#define SQL_MACRO_INT(n, x, y)		MSG_FIELD(n, int, 		x, 0)
#define SQL_MACRO_BIGINT(n, x, y)	MSG_FIELD(n, ULONGLONG,	x, 0)
#define SQL_MACRO_END END_MSG_STRUCT
#undef MACRO_SQL_MSG_DEF
#endif

/*
#ifdef MACRO_SQL_STRUCT_DEF
//We can get a normal struct out of these defines, but it's simpler just to use the MSG one.
#define SQL_MACRO_INT(x) int x;
#define SQL_MACRO_BIGINT(x) INT64 x;

#undef MACRO_SQL_STRUCT_DEF
#endif
*/

#ifdef MACRO_SQL_TRANSFER_TO_STRUCT_DEF
#undef SQL_MACRO_START
#undef SQL_MACRO_INT
#undef SQL_MACRO_BIGINT
#undef SQL_MACRO_END
#define SQL_MACRO_START(name)
#define SQL_MACRO_INT(n, x, y)		pTransferContext->y = msg->x;
#define SQL_MACRO_BIGINT(n, x, y)	pTransferContext->y = msg->x;
#define SQL_MACRO_END
#undef MACRO_SQL_TRANSFER_TO_STRUCT_DEF
#endif

#ifdef MACRO_SQL_TRANSFER_TO_MSG_DEF
#undef SQL_MACRO_START
#undef SQL_MACRO_INT
#undef SQL_MACRO_BIGINT
#undef SQL_MACRO_END
#define SQL_MACRO_START(name)
#define SQL_MACRO_INT(n, x, y)		reqData.x = pTransferContext->y;
#define SQL_MACRO_BIGINT(n, x, y)	reqData.x = pTransferContext->y;
#define SQL_MACRO_END
#undef MACRO_SQL_TRANSFER_TO_MSG_DEF
#endif

#ifdef MACRO_SQL_COLUMN_BIND_DEF
#undef SQL_MACRO_START
#undef SQL_MACRO_INT
#undef SQL_MACRO_BIGINT
#undef SQL_MACRO_END
#define SQL_MACRO_START(name) 	\
	SQLHANDLE hStmt = NULL; \
	SQLRETURN sqlRet = SQL_ERROR; \
	SQLLEN len; \
	BOOL bRet = TRUE; \
 \
	CBRA(db); \
	CBRA(reqData); \
 \
	hStmt = DatabaseManagerGetStatement(db, szQuery); \
	CBRA(hStmt);
#define SQL_MACRO_INT(n, x, y)		CSRA(SQLBindCol(hStmt,n+1,SQL_INTEGER, &reqData->x,sizeof(reqData->x),&len));
#define SQL_MACRO_BIGINT(n, x, y)	CSRA(SQLBindCol(hStmt,n+1,SQL_C_SBIGINT, &reqData->x,sizeof(reqData->x),&len));
#define SQL_MACRO_END \
	CSRA(SQLExecute(hStmt) ); \
	CSRA(SQLFetch(hStmt) );
#undef MACRO_SQL_COLUMN_BIND_DEF
#endif

#ifdef MACRO_SQL_PARAM_BIND_DEF
#undef SQL_MACRO_START
#undef SQL_MACRO_INT
#undef SQL_MACRO_BIGINT
#undef SQL_MACRO_END
#define SQL_MACRO_START(name) 	\
	SQLHANDLE hStmt = NULL; \
	SQLRETURN sqlRet = SQL_ERROR; \
	SQLLEN len; \
	REF(len); \
	BOOL bRet = TRUE; \
	\
	CBRA(db); \
	CBRA(reqData); \
	\
	hStmt = DatabaseManagerGetStatement(db, szQuery); \
	CBRA(hStmt);
#define SQL_MACRO_INT(n, x, y)		CSRA(SQLBindParameter(hStmt, n+1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->x, (SQLINTEGER)0, (SQLLEN *)0) );
#define SQL_MACRO_BIGINT(n, x, y)	CSRA(SQLBindParameter(hStmt, n+1, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->x, (SQLINTEGER)0, (SQLLEN *)0) );
#define SQL_MACRO_END \
	CSRA(SQLExecute(hStmt) );
#undef MACRO_SQL_PARAM_BIND_DEF
#endif

//define MACRO_SQL_START and MACRO_SQL_END as complex shit.
