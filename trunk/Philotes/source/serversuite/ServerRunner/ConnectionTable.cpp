//----------------------------------------------------------------------------
// ConnectionTable.cpp
// (C)Copyright 2007, Ping0 Interactive Limited. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"


//----------------------------------------------------------------------------
// SERVER CONNECTION INIT METHOD
//----------------------------------------------------------------------------
void CONNECTION::Init(
		void )
{
	ConnectionType				= CONNECTION_TYPE_NONE;
	RemoteServerType			= INVALID_SVRTYPE;
	RequestCommandHandlers		= NULL;
	ResponseCommandHandlers		= NULL;
	GetInboundCmdTbl			= NULL;
	GetOutboundCmdTbl			= NULL;
	OnClientAttached			= NULL;
	OnClientDetached			= NULL;
	ConnectionSet				= FALSE;
}


//----------------------------------------------------------------------------
// CONNECTION TABLE METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CONNECTION_TABLE::Init(
	BOOL (*initFunc)( CONNECTION_TABLE * ) )
{
	for( int ii = 0; ii < TOTAL_SERVER_COUNT; ++ii )
	{
		OfferedServices[ii].Init();
		RequiredServices[ii].Init();
	}

	m_initFunc = initFunc;
	ASSERTX_RETFALSE( m_initFunc, "CONNECTION TABLE error: no connection table init function given." );
	ASSERT_RETFALSE(m_initFunc( this ));

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SetServerType(
	CONNECTION_TABLE *	tbl,
	SVRTYPE				svrType )
{
	ASSERT_RETURN( tbl );
	ASSERT_RETURN( svrType < TOTAL_SERVER_COUNT );

	//	set type and zero out connection count
	tbl->TableServerType	= svrType;
	tbl->TotalConnections	= 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void OfferService(
	CONNECTION_TABLE *					tbl,
	SVRTYPE								typeOfferedTo,
	FP_NET_REQUEST_COMMAND_HANDLER *	netRequestCommandHandlers,
	FP_GET_CMD_TBL						getInboundCmdTbl,
	FP_GET_CMD_TBL						getOutboundCmdTbl,
	FP_NETSVR_CLIENT_ATTACHED			onClientAttached,
	FP_NETSVR_CLIENT_DETACHED			onClientDetached )
{
	//	try to make debugging life easier by not allowing bogus connection init data.
	ASSERT_RETURN( tbl );
	ASSERTX( typeOfferedTo < TOTAL_SERVER_COUNT,	"CONNECTION TABLE definition error: invalid server type." );
	ASSERTX( getInboundCmdTbl,						"CONNECTION TABLE definition error: net inbound command table retrieval callback may not be NULL." );

	//	get the connection and fill client type specific data
	CONNECTION * connection;
	connection = &tbl->OfferedServices[ typeOfferedTo ];
	ASSERTX( !connection->ConnectionSet,			"CONNECTION TABLE definition error: server service already offered." );
	connection->ConnectionType			= CONNECTION_TYPE_SERVICE_PROVIDER;

	//	fill common service data
	connection->RemoteServerType		= typeOfferedTo;
	connection->RequestCommandHandlers	= netRequestCommandHandlers;
	connection->ResponseCommandHandlers	= NULL;
	connection->GetInboundCmdTbl		= getInboundCmdTbl;
	connection->GetOutboundCmdTbl		= getOutboundCmdTbl;
	connection->OnClientAttached		= onClientAttached;
	connection->OnClientDetached		= onClientDetached;

	//	mark as initialized
	connection->ConnectionSet			= TRUE;

	//	inc tbl connection count
	++tbl->TotalConnections;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RequireService(
	CONNECTION_TABLE *					tbl,
	SVRTYPE								svrTypeRequiredFrom,
	FP_NET_RESPONSE_COMMAND_HANDLER *	netResponseCommandHandlers,
	FP_NET_RAW_READ_HANDLER             fpRawReadHandler)
{
	//	try to make debugging life easier by not allowing bogus connection init data.
	ASSERT_RETURN( tbl );
	ASSERTX_RETURN( svrTypeRequiredFrom < TOTAL_SERVER_COUNT,	"CONNECTION TABLE definition error: invalid server type." );
	ASSERTX_RETURN( (netResponseCommandHandlers != NULL) ^ (fpRawReadHandler != NULL), "CONNECTION TABLE definition error: net response command handler callback array required, function pointers within array may be NULL." );

	//	get the corresponding connection struct
	CONNECTION & connection = tbl->RequiredServices[ svrTypeRequiredFrom ];
	ASSERTX( !connection.ConnectionSet,					"CONNECTION TABLE definition error: server type service already required." );

	//	fill out connection as service user
	connection.ConnectionType			= CONNECTION_TYPE_SERVICE_USER;
	connection.RemoteServerType			= svrTypeRequiredFrom;
	connection.RequestCommandHandlers	= NULL;
	connection.ResponseCommandHandlers	= netResponseCommandHandlers;
	connection.RawReadHandler       	= fpRawReadHandler;
	connection.GetInboundCmdTbl			= NULL;
	connection.GetOutboundCmdTbl		= NULL;
	connection.OnClientAttached			= NULL;
	connection.OnClientDetached			= NULL;

	//	mark initialized
	connection.ConnectionSet			= TRUE;

	//	inc tbl connection count
	++tbl->TotalConnections;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL	ValidateTable(
			CONNECTION_TABLE *	tbl,
			SVRTYPE				svrType )
{
	ASSERT_RETFALSE( tbl );
	ASSERT_RETFALSE( tbl->m_initFunc );
	ASSERT_RETFALSE( tbl->TableServerType == svrType );
#if ISVERSION( DEBUG_VERSION )
	WARNX( tbl->TotalConnections > 0, "No connections specified for server." );
#endif
	return TRUE;
}
