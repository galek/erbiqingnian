//----------------------------------------------------------------------------
// ConnectionTable.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef	_CONNECTIONTABLE_H_
#define _CONNECTIONTABLE_H_


//----------------------------------------------------------------------------
// CONNECTION HELPER STRUCT
//----------------------------------------------------------------------------
struct CONNECTION
{
	#define CONNECTION_TYPE_NONE				0
	#define CONNECTION_TYPE_SERVICE_PROVIDER	1
	#define CONNECTION_TYPE_SERVICE_USER		2

	//	connection type members
	BYTE								ConnectionType;								// connection type flag
	SVRTYPE								RemoteServerType;							// the type of server on the other end of the connection
	FP_NET_REQUEST_COMMAND_HANDLER*		RequestCommandHandlers;						// immediate message handlers for inbound requests
	FP_NET_RESPONSE_COMMAND_HANDLER*	ResponseCommandHandlers;					// immediate message handlers for inbound service provider responses
	FP_NET_RAW_READ_HANDLER 	        RawReadHandler;					            // message handler for inbound raw data
	FP_GET_CMD_TBL						GetInboundCmdTbl;							// called to retrieve the inbound command table associated with a service
	FP_GET_CMD_TBL						GetOutboundCmdTbl;							// called to retrieve tha outbound command table associated with a service
	FP_NETSVR_CLIENT_ATTACHED			OnClientAttached;							// called when a client attaches to the service
	FP_NETSVR_CLIENT_DETACHED			OnClientDetached;							// called when a client detaches from a service

	BOOL								ConnectionSet;								// error checking flag

	//------------------------------------------------------------------------
	void Init(
		void );
};


//----------------------------------------------------------------------------
// CONNECTION TABLE
//----------------------------------------------------------------------------
struct CONNECTION_TABLE
{
	//	connection information
	BOOL								(*m_initFunc)(CONNECTION_TABLE*);			// the macro defined init func
	SVRTYPE								TableServerType;							// the server type that this table is for
	DWORD								TotalConnections;							// the total # of both services and required services for this table
	CONNECTION							OfferedServices [ TOTAL_SERVER_COUNT ];		// the array of all possible offered internal service connections
	CONNECTION							RequiredServices[ TOTAL_SERVER_COUNT ];		// the array of all possible required service connections

	//------------------------------------------------------------------------
	CONNECTION_TABLE(
		void ) :
		m_initFunc( NULL ),
		TableServerType( TOTAL_SERVER_COUNT )
	{}
	//------------------------------------------------------------------------
	BOOL Init(
		BOOL (*initFunc)( CONNECTION_TABLE * ) );
};


#endif	//	_CONNECTIONTABLE_H_
