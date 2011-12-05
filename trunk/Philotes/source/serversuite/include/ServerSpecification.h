//----------------------------------------------------------------------------
// ServerSpecification.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef	_SERVERSPECIFICATION_H_
#define _SERVERSPECIFICATION_H_


//----------------------------------------------------------------------------
// SERVER METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//	init function called before the servers network connections are made.
//	svrConfig will be freed after this call returns.
//	NOTE: not safe to call any of the server runner api methods when called,
//			so info is passed back to server.
//----------------------------------------------------------------------------
typedef LPVOID						(*FP_SERVER_PRE_NET_INIT)(
										struct MEMORYPOOL *			pool,
										SVRID						svrId,
										const class SVRCONFIG *		svrConfig );

//----------------------------------------------------------------------------
//	entry point of the specified server. If/when this returns the server will
//		be destroyed.
//----------------------------------------------------------------------------
typedef void						(*FP_SERVER_ENTRY_POINT)(
										LPVOID				context );

//----------------------------------------------------------------------------
//	called whenever the controlling server runner issues a command to the
//		running server. DWORD parameter is one of the SR_COMMAND enumerated
//		values as declared in ServerRunnerAPI.h
//----------------------------------------------------------------------------
typedef void						(*FP_SERVER_SRCOMMAND_CALLBACK)(
										LPVOID				context,
										SR_COMMAND			srCommand );

//----------------------------------------------------------------------------
//	called when the server runner receives an XML message for the server.
//	passed the server context, the definition provided for who sent the message,
//		and the definition of the message itself.
//	SEE: ServerRunnerAPI.h for XMLMESSAGE and XMLMESSAGE_ELEMENT definitions.
//	SEE: XmlMessageMapper.h for helpfull command to function mapping.
//----------------------------------------------------------------------------
typedef void						(*FP_SERVER_XMLMESSAGE_CALLBACK)(
										LPVOID				context,
										struct XMLMESSAGE *	senderSpecification,
										struct XMLMESSAGE *	msgSpecification,
										LPVOID				userData );


//----------------------------------------------------------------------------
// CONNECTION COMMAND TABLE METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//	called to create a net message command table, 
//		guaranteed to be called at most once.
//----------------------------------------------------------------------------
typedef struct NET_COMMAND_TABLE * (*FP_GET_CMD_TBL)(
										void );

//----------------------------------------------------------------------------
//	incoming service immediate message processing callback, passed id of sender, 
//		the translated message, and the data associated with the sending client.
//----------------------------------------------------------------------------
typedef void					   (*FP_NET_REQUEST_COMMAND_HANDLER)(
										LPVOID				svrContext,
										SERVICEUSERID		sender,
										struct MSG_STRUCT * msg,
										LPVOID				cltContext);

//----------------------------------------------------------------------------
//	incoming service response immediate message processing callback, passed id
//		of responding service and the translated message.
//----------------------------------------------------------------------------
typedef void					   (*FP_NET_RESPONSE_COMMAND_HANDLER)(
										LPVOID				svrContext,
										SVRID				sender,
										struct MSG_STRUCT * msg );

//----------------------------------------------------------------------------
//	incoming service response raw read callback, passed id
//		of responding service and the translated message.
//----------------------------------------------------------------------------
typedef BOOL					   (*FP_NET_RAW_READ_HANDLER)(
										LPVOID				svrContext,
										SVRID				sender,
										BYTE               *buffer,
                                        DWORD               bufferLength);
//----------------------------------------------------------------------------
//	called when a service user attaches to a svr service, returned pointer
//		is associated with this client.
//----------------------------------------------------------------------------
typedef LPVOID					   (*FP_NETSVR_CLIENT_ATTACHED)(
										LPVOID				svrContext,
										SERVICEUSERID		attachedUser,
                                        BYTE                *pAcceptData,
                                        DWORD                dwAcceptDataLen);

//----------------------------------------------------------------------------
//	called when a service user disconnects from a svr service, passed the
//		data pointer returned from client attached callback.
//----------------------------------------------------------------------------
typedef void					   (*FP_NETSVR_CLIENT_DETACHED)(
										LPVOID				svrContext,
										SERVICEUSERID		detachedUser,
										LPVOID				cltContext );


//----------------------------------------------------------------------------
// CONNECTION TABLE INIT MACROS
//----------------------------------------------------------------------------

typedef void (*FP_CONNECTION_TABLE_INIT_FUNC)( struct CONNECTION_TABLE * tbl );

//----------------------------------------------------------------------------
//	generates the agreed upon init function name token for a given server type
//----------------------------------------------------------------------------
#define CONNECTION_TABLE_INIT_FUNC_NAME( serverType ) \
			serverType##_CONNECTION_TABLE_INIT

//----------------------------------------------------------------------------
//	starts the creation of a server connection table init function
//----------------------------------------------------------------------------
#define CONNECTION_TABLE_INIT_FUNC_START( serverType ) \
			static __checkReturn BOOL CONNECTION_TABLE_INIT_FUNC_NAME( serverType )( \
							CONNECTION_TABLE* tbl ) \
			{	SetServerType( tbl, (SVRTYPE)serverType ); \
				SVRTYPE myType = serverType;

//----------------------------------------------------------------------------
//	registers an offered service with the connection table
//	offerTo : the enumerated type of internal server or external client that you
//				wish to offer a service to.
//	requestCmdHdlrArray : array of FP_NET_REQUEST_COMMAND_HANDLER methods
//				to be called whenever a request is received.
//				NOTE: methods are indexed by net command value.
//	getRequestTbl : a FP_GET_CMD_TBL method that returns the command table for
//				all inbound request messages.
//	getResponseTbl : a FP_GET_CMD_TBL method that returns the command table for
//				all outbound response messages.
//	cltAttached : a FP_NETSVR_CLIENT_ATTACHED method to be called every time
//				a service user attaches to the server.
//	cltDetached : a FP_NETSVR_CLIENT_DETACHED method to be called every time
//				an attached service user detaches from the server.
//----------------------------------------------------------------------------
#define OFFER_SERVICE( offerTo, requestCmdHdlrArray, getRequestTbl, getResponseTbl, cltAttached, cltDetached ) \
			OfferService(	tbl, \
							offerTo, \
							requestCmdHdlrArray, \
							getRequestTbl, \
							getResponseTbl, \
							cltAttached, \
							cltDetached );

//----------------------------------------------------------------------------
//	registers a service requirement with the connection table
//	requireFrom : the enumerated type of server you wish the current server
//				to connect to as a client. NOTE:specified server type must offer
//				a service to the type requiring the service.
//	responseCmdHdlrArray : array of FP_NET_RESPONSE_COMMAND_HANDLER methods
//				to be called whenever a response is recieved.
//				NOTE: methods are indexed by net command value.
//----------------------------------------------------------------------------
#define REQUIRE_SERVICE( requireFrom, responseCmdHdlrArray,rawReadHandler ) \
			RequireService(	tbl, \
							requireFrom, \
							responseCmdHdlrArray,\
                            rawReadHandler );

//----------------------------------------------------------------------------
//	closes a connection table init function
//----------------------------------------------------------------------------
#define CONNECTION_TABLE_INIT_FUNC_END \
			UNREFERENCED_PARAMETER(myType); \
			ASSERT_RETFALSE( ValidateTable( tbl, myType ) );return TRUE; }


//----------------------------------------------------------------------------
// CONNECTION TABLE FUNCTIONS
// Forward declarations for the functions used in the connection
//		table init function generation macros.
//----------------------------------------------------------------------------
struct	CONNECTION_TABLE;
void	SetServerType(
			CONNECTION_TABLE *,
			SVRTYPE );
void	OfferService(
			CONNECTION_TABLE *,
			SVRTYPE,
			FP_NET_REQUEST_COMMAND_HANDLER[],
			FP_GET_CMD_TBL,
			FP_GET_CMD_TBL,
			FP_NETSVR_CLIENT_ATTACHED,
			FP_NETSVR_CLIENT_DETACHED );
void	RequireService(
			CONNECTION_TABLE *,
			SVRTYPE,
			FP_NET_RESPONSE_COMMAND_HANDLER[],
            FP_NET_RAW_READ_HANDLER  );
BOOL	ValidateTable(
			CONNECTION_TABLE *,
			SVRTYPE );


//----------------------------------------------------------------------------
// SPECIFICATION NAME GENERATOR MACRO
// Somewhere in a servers static lib there must be a static server
//	specification that is named according to this macro.
//----------------------------------------------------------------------------
#define SERVER_SPECIFICATION_NAME( typeName )		typeName##_SPECS


//----------------------------------------------------------------------------
// SPECIFICATION STRUCT
//----------------------------------------------------------------------------
struct SERVER_SPECIFICATION
{
	SVRTYPE							svrType;				// the enumerated type of the server being specified
	const char *					svrName;				// c-string name of this server
	
	FP_SERVER_PRE_NET_INIT			svrPreNetSetupInit;		// init method called before any service entrances are brought online
	FP_SVRCONFIG_CREATOR			svrConfigCreator;		// custom SVRCONFIG creator method. See: ServerConfiguration.h
	FP_SERVER_ENTRY_POINT			svrEntryPoint;			// entry point function for this server
	FP_SERVER_SRCOMMAND_CALLBACK	svrCommandCallback;		// callback for server runner commands
	FP_SERVER_XMLMESSAGE_CALLBACK	svrXmlMessageCallback;	// callback for cluster level XML messages
	
	BOOL						  (*svrConnectionTableInit)(CONNECTION_TABLE*);	// macro defined init function for the table of connections for this server

	//------------------------------------------------------------------------
	BOOL Validate(
		void )
	{
		ASSERT_RETFALSE( svrType < TOTAL_SERVER_COUNT );
		ASSERT_RETFALSE( svrName && svrName[0] );
		ASSERT_RETFALSE( svrPreNetSetupInit );
		ASSERT_RETFALSE( svrCommandCallback );
		ASSERT_RETFALSE( svrConnectionTableInit );
		return TRUE;
	}
};


#endif	//	_SERVERSPECIFICATION_H_


/*****************************************************************************
 ************************* EXAMPLE SERVER SPECIFICATIONS *********************
 *****************************************************************************

 //----------------------------------------------------------------------------
 // MSG HANDLER ARRAYS
 //----------------------------------------------------------------------------
 FP_NET_RESPONSE_COMMAND_HANDLER ssTSResponseCmdHdlrs[] =
 {
 ResponseTSTextMsg
 };
 FP_NET_REQUEST_COMMAND_HANDLER  ssTSRequestCmdHdlrs[] =
 {
 RequestTSTextMsg
 };

 //----------------------------------------------------------------------------
 // TEST SERVER CONNECTION TABLES
 //----------------------------------------------------------------------------

 CONNECTION_TABLE_INIT_FUNC_START( ESVR_1 )
 OFFER_SERVICE( ESVR_2, ssTSRequestCmdHdlrs, ssTSGetRequestTbl, ssTSGetResponseTbl, ssTSClientAttached, ssTSClientDetached )
 OFFER_SERVICE( ESVR_3, ssTSRequestCmdHdlrs, ssTSGetRequestTbl, ssTSGetResponseTbl, ssTSClientAttached, ssTSClientDetached )
 CONNECTION_TABLE_INIT_FUNC_END

 CONNECTION_TABLE_INIT_FUNC_START( ESVR_2 )
 REQUIRE_SERVICE( ESVR_1, ssTSResponseCmdHdlrs )
 REQUIRE_SERVICE( ESVR_3, ssTSResponseCmdHdlrs )
 CONNECTION_TABLE_INIT_FUNC_END

 CONNECTION_TABLE_INIT_FUNC_START( ESVR_3 )
 REQUIRE_SERVICE( ESVR_1, ssTSResponseCmdHdlrs )
 OFFER_SERVICE(   ESVR_2, ssTSRequestCmdHdlrs, ssTSGetRequestTbl, ssTSGetResponseTbl, ssTSClientAttached, ssTSClientDetached )
 CONNECTION_TABLE_INIT_FUNC_END

 //----------------------------------------------------------------------------
 // TEST SERVER SPECIFICATIONS
 //----------------------------------------------------------------------------

 //----------------------------------------------------------------------------
 // EXAMPLE SERVER 1
 //----------------------------------------------------------------------------
 SERVER_SPECIFICATION SERVER_SPECIFICATION_NAME( ESVR_1 ) =
 {
 ESVR_1,
 G_SVR_NAMES[ ESVR_1 ],
 NULL,	//ssTSGetDefContainer,
 ssTSPreNetSetupInit,
 ssTSEntryPoint,
 ssTSCommandCallback,
 CONNECTION_TABLE_INIT_FUNC_NAME( ESVR_1 )
 };

 //----------------------------------------------------------------------------
 // EXAMPLE SERVER 2
 //----------------------------------------------------------------------------
 SERVER_SPECIFICATION SERVER_SPECIFICATION_NAME( ESVR_2 ) =
 {
 ESVR_2,
 G_SVR_NAMES[ ESVR_2 ],
 NULL,	//ssTSGetDefContainer,
 ssTSPreNetSetupInit,
 ssTSEntryPoint,
 ssTSCommandCallback,
 CONNECTION_TABLE_INIT_FUNC_NAME( ESVR_2 )
 };

 //----------------------------------------------------------------------------
 // EXAMPLE SERVER 3
 //----------------------------------------------------------------------------
 SERVER_SPECIFICATION SERVER_SPECIFICATION_NAME( ESVR_3 ) =
 {
 ESVR_3,
 G_SVR_NAMES[ ESVR_3 ],
 NULL,	//ssTSGetDefContainer,
 ssTSPreNetSetupInit,
 ssTSEntryPoint,
 ssTSCommandCallback,
 CONNECTION_TABLE_INIT_FUNC_NAME( ESVR_3 )
 };

 *****************************************************************************/
