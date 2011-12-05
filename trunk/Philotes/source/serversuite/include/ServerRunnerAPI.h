//----------------------------------------------------------------------------
// ServerRunnerAPI.h
//
// Server runner library interface functions for servers being managed by
//		the runner framework.
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef	_SERVERRUNNERAPI_H_
#define _SERVERRUNNERAPI_H_
#include "servers.h" // TOTAL_SERVER_COUNT

//****************************************************************************
//*********************	SERVER RUNNER TYPES AND DEFINES	**********************
//****************************************************************************


//----------------------------------------------------------------------------
// HARD LIMIT DEFINES
//----------------------------------------------------------------------------
#define					MAX_SVR_NAME					128
#define					MAX_SVR_INSTANCES				0xFFFE		// 0xFFFF is reserved for invalid instance value
#define					MAX_SVR_INSTANCES_PER_MACHINE	2			// arbitrary static limit
#define					THREAD_ERROR					((DWORD)-1)	// return code for SvrRunnerFreeThread when thread close times out or system error
#if ISVERSION(SERVER_VERSION)
#define					MAX_SVR_THREADS					16			// arbitrary static limit
#else
#define					MAX_SVR_THREADS					2
#endif

#define					KILOBYTES						1024
#define					MEGABYTES						(1024 * KILOBYTES)
#define					GAME_STACK_SIZE					(MEGABYTES * 2)

//----------------------------------------------------------------------------
// FRAMEWORK TYPES
//----------------------------------------------------------------------------
typedef UINT16			SVRTYPE;				// the enumerated type of a server
typedef UINT16			SVRINSTANCE;			// the instance number of a server type
typedef UINT32			SVRCONNECTIONID;		// unique id for every client of a given service, may duplicate across services


//----------------------------------------------------------------------------
// VALUE DEFINES
//----------------------------------------------------------------------------
#define					INVALID_SVRTYPE					((SVRTYPE)-1)
#define					INVALID_SVRINSTANCE				((SVRINSTANCE)-1)
#define					INVALID_SVRCONNECTIONID			((SVRCONNECTIONID)-1)


//----------------------------------------------------------------------------
// SERVER ID
//----------------------------------------------------------------------------
typedef UINT32			SVRID;					// cluster unique server id
#define					INVALID_SVRID			0xFFFFFFFF							//			|   server type   |instance number |
inline  SVRID			ServerId(				SVRTYPE     type,					//			|********|********|********|********|
												SVRINSTANCE instance )				{ return ((UINT32)type << 16) | (UINT32)instance; }
inline  SVRTYPE			ServerIdGetType(		SVRID svrId )						{ return (SVRTYPE)(svrId >> 16); }
inline  SVRINSTANCE		ServerIdGetInstance(	SVRID svrId )						{ return (SVRINSTANCE)svrId; }


//----------------------------------------------------------------------------
// SERVICE ID
//----------------------------------------------------------------------------
typedef UINT64			SERVICEID;
#define					INVALID_SERVICEID	   0xFFFFFFFFFFFFFFFF
inline SERVICEID		ServiceId( SVRTYPE     providerType,						//			|   user instance |    user type    |  provider type  |provider instance|
								   SVRINSTANCE providerInstance,					//			|********|********|********|********|********|********|********|********|
								   SVRTYPE     userType,
								   SVRINSTANCE userInstance )						{ return ((UINT64)userInstance << 48) | ((UINT64)userType << 32) | ((UINT64)providerType << 16) | (UINT64)providerInstance; }
inline SVRTYPE			ServiceIdGetProviderType(	  SERVICEID serviceid )			{ return (SVRTYPE)(serviceid >> 16); }
inline SVRINSTANCE		ServiceIdGetProviderInstance( SERVICEID serviceid )			{ return (SVRINSTANCE)serviceid; }
inline SVRTYPE			ServiceIdGetUserType(		  SERVICEID serviceid )			{ return (SVRTYPE)(serviceid >> 32); }
inline SVRINSTANCE		ServiceIdGetUserInstance(	  SERVICEID serviceid )			{ return (SVRINSTANCE)(serviceid >> 48); }
inline SVRID			ServiceIdGetServer(			  SERVICEID serviceid )			{ return (SVRID)serviceid; }


//----------------------------------------------------------------------------
// SERVICE USER ID
//----------------------------------------------------------------------------
typedef UINT64			SERVICEUSERID;
#define					INVALID_SERVICEUSERID			0xFFFFFFFFFFFFFFFF
inline SERVICEUSERID	ServiceUserId( SVRTYPE			userType,					//			|    user type    |  user instance  |          connection id            |
									   SVRINSTANCE		userInstance,				//			|********|********|********|********|********|********|********|********|
									   SVRCONNECTIONID	connectionId )				{ return ((UINT64)userType << 48) | ((UINT64)userInstance << 32) | (UINT64)connectionId; }
inline SERVICEUSERID	ServiceUserId( SVRID			serverId,
									   SVRCONNECTIONID	connectionId )				{ return ((UINT64)serverId << 32) | (UINT64)connectionId; }
inline SVRTYPE			ServiceUserIdGetUserType(	  SERVICEUSERID serviceUserId )	{ return (SVRTYPE)(serviceUserId >> 48); }
inline SVRCONNECTIONID	ServiceUserIdGetConnectionId( SERVICEUSERID serviceUserId )	{ return (SVRCONNECTIONID)serviceUserId; }
inline BOOL				ServiceUserIdUserIsExternal(  SERVICEUSERID serviceUserId ) { return ( ServiceUserIdGetUserType( serviceUserId ) >= TOTAL_SERVER_COUNT ); }
inline SVRINSTANCE		ServiceUserIdGetUserInstance( SERVICEUSERID serviceUserId )	{ return ( ServiceUserIdUserIsExternal( serviceUserId ) ) ? INVALID_SVRINSTANCE : (SVRINSTANCE)(serviceUserId >> 32); }
inline SVRID			ServiceUserIdGetServer(		  SERVICEUSERID serviceUserId ) { return ( ServiceUserIdUserIsExternal( serviceUserId ) ) ? INVALID_SVRID       : (SVRID)( serviceUserId >> 32 ); }


//----------------------------------------------------------------------------
// SERVER VERSION
// NOTE: see revision.h for a timestamp based compile-time revision number macro.
//----------------------------------------------------------------------------		//			|  major version  |  minor version  |              revision             |
typedef UINT64			SVRVERSION;													//			|********|********|********|********|********|********|********|********|
#define					ServerVersion( major, minor, revision )						((UINT64)( ((UINT64)major << 48) | ((UINT64)minor << 32) | (UINT64)revision ))
#define					ServerVersionMajor( version )								((UINT16)(version >> 48))
#define					ServerVersionMinor( version )								((UINT16)(version >> 32))
#define					ServerVersionRevision( version )							((UINT32)version)


//****************************************************************************
//**************************	MAILBOX INTERFACE	**************************
//****************************************************************************

//----------------------------------------------------------------------------
// MAILBOX MESSAGE TYPE VALUES
//----------------------------------------------------------------------------
enum MSGTYPE
{
	MSG_TYPE_NONE,
	MSG_TYPE_REQUEST,
	MSG_TYPE_RESPONSE
};

//----------------------------------------------------------------------------
// MAILBOX MESSAGE STRUCT
//----------------------------------------------------------------------------
struct MSG_STRUCT;
struct MAILBOX_MSG
{
	BYTE				m_Msg[MAX_SMALL_MSG_STRUCT_SIZE];		// the MSG_STRUCT buffer
	MSGTYPE				MessageType;					// enumerated message type
	BOOL				IsFakeMessage;					// if TRUE, this message came from a call to post fake message.
	union 
	{
		SERVICEUSERID	RequestingClient;				// the user id of the client sending the request
		SVRID			RespondingServer;				// the server id of the server sending the response
	};

	MSG_STRUCT * GetMsg(
		void) 
	{ 
		return (MSG_STRUCT *)m_Msg; 
	}
};


//----------------------------------------------------------------------------
// SERVER MAILBOX INTERFACE
//----------------------------------------------------------------------------

struct SERVER_MAILBOX;
struct MAILBOX_MSG_LIST;

//----------------------------------------------------------------------------
//	get a new mailbox with an initial reference already set.
//	mailboxes are thread safe.
//----------------------------------------------------------------------------
SERVER_MAILBOX *	SvrMailboxNew(
						struct MEMORYPOOL * pool );

//----------------------------------------------------------------------------
//	release a handle to a mailbox, not destroyed until internal ref count == 0.
//----------------------------------------------------------------------------
void				SvrMailboxRelease(
						SERVER_MAILBOX * );

//----------------------------------------------------------------------------
//	get a handle to a waitable semaphore object that is incremented whenever
//		a message is added to the mailbox.
//----------------------------------------------------------------------------
HANDLE				SvrMailboxGetSemaphoreHandle(
						SERVER_MAILBOX * );

//----------------------------------------------------------------------------
//	mailbox is not necessarily empty if SvrNetPopMailboxMessage returns FALSE,
//		it may return false if it pops an ill-formed or illegal message.
//	use this method when popping messages from a mailbox.
//----------------------------------------------------------------------------
BOOL				SvrMailboxHasMoreMessages(
						SERVER_MAILBOX * );

//----------------------------------------------------------------------------
//	removes any pending message buffers from the list of held messages.
//----------------------------------------------------------------------------
void				SvrMailboxEmptyMailbox(
						SERVER_MAILBOX * );

//----------------------------------------------------------------------------
//	frees all memory tied up in message free lists.
//----------------------------------------------------------------------------
void				SvrMailboxFreeMailboxBuffers(
						SERVER_MAILBOX * );

//----------------------------------------------------------------------------
//	remove all net messages and place them into a mailbox message list for later
//		processing, lists are not thread safe.
//	list must be released to the same mailbox before this may be called again.
//----------------------------------------------------------------------------
MAILBOX_MSG_LIST *	SvrMailboxGetAllMessages(
						SERVER_MAILBOX * );

//----------------------------------------------------------------------------
//	message list is not necessarily empty if SvrNetPopMailboxListMessage returns
//		FALSE, it may return false if it pops an ill-formed message.
//		use this method when popping from a message list.
//----------------------------------------------------------------------------
BOOL				SvrMailboxListHasMoreMessages(
						MAILBOX_MSG_LIST * );

//----------------------------------------------------------------------------
//	free the list of messages, list MUST be freed to the mailbox it	was 
//	  retrieved from, and must be freed before get all messages is called again.
//----------------------------------------------------------------------------
void				SvrMailboxReleaseMessageList(
						SERVER_MAILBOX *,
						MAILBOX_MSG_LIST * );


//****************************************************************************
//*********************	SERVER PROVISIONING METHODS	**************************
//****************************************************************************


//----------------------------------------------------------------------------
// SERVER RUNNER COMMANDS
//----------------------------------------------------------------------------
enum SR_COMMAND
{
	SR_COMMAND_SHUTDOWN,
	//	TODO: fill in all possible commands the server runner needs to send to a hosted server...

	//	KEEP LAST ENTRY
	SR_COMMAND_COUNT
};


//----------------------------------------------------------------------------
// CLUSTER XML MESSAGING STRUCTS
//	pre-parsed simple XML message element containers
//----------------------------------------------------------------------------

#define MAX_XMLMESSAGE_ELEMENTS			32

struct XMLMESSAGE_ELEMENT
{
	const WCHAR *		ElementName;
	const WCHAR *		ElementValue;
};

struct XMLMESSAGE
{
	const WCHAR *		MessageName;
	DWORD				ElementCount;
	XMLMESSAGE_ELEMENT	Elements[MAX_XMLMESSAGE_ELEMENTS];
};


//----------------------------------------------------------------------------
// ACTIVE SERVER FUNCTIONS
//----------------------------------------------------------------------------
LPVOID				CurrentSvrGetContext(
						void );
SVRTYPE				CurrentSvrGetType(
						void );
SVRINSTANCE			CurrentSvrGetInstance(
						void );
MEMORYPOOL *		CurrentSvrGetMemPool(
						void );
// This function should rarely be needed.  It sets up the server context for the current thread.  
// Normally, the server runner sets this context before calling into server functions.  
// If you are getting callbacks from the OS, you may need to set the server context yourself.
// It's considered good practice to reset the thread context to INVALID_SRVID after you are done.
void				CurrentSvrSetThreadContext(
						SVRID serverId );


#if ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
// DATABASE FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum DATABASE_SELECTION
{
	DS_INVALID = -1,
	
	DS_GAME,				// the game db
	DS_LOG_WAREHOUSE,		// the log warehouse db

	DS_NUM_SELECTIONS
		
};

struct DATABASE_MANAGER* SvrGetGameDatabaseManager( DATABASE_SELECTION eDatabase = DS_GAME );
#endif // ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
// SERVER RUNNER SYSTEM FUNCTIONS
// NOTE: Only one thread opperation is allowed per server at any one time.
//----------------------------------------------------------------------------
HANDLE				SvrRunnerCreateThread(
						DWORD stacksize,
						LPTHREAD_START_ROUTINE,
						LPVOID );
DWORD				SvrRunnerFreeThread(
						HANDLE thread,
						DWORD  timeout = 0 );
void				SvrRunnerKillServer(
						void );


//----------------------------------------------------------------------------
// SERVER RUNNER NETWORKING FUNCTIONS
//----------------------------------------------------------------------------
enum SRNET_RETURN_CODE
{
	SRNET_FAILED = -1,
	SRNET_TEMPORARY_FAILURE = 0, // can retry
	SRNET_SUCCESS =1
};

//	service usage methods
__checkReturn SRNET_RETURN_CODE				SvrNetSendRequestToService(
						SVRID,
						MSG_STRUCT *,
						NET_CMD );
				//	pass NULL for mailslot to un-attach
void				SvrNetAttachResponseMailbox(
						SVRID,
						SERVER_MAILBOX * );
BOOL				SvrNetPostFakeServerResponseToMailbox(
						SERVER_MAILBOX *	mailbox,
						SVRID				sendingService,
						MSG_STRUCT *		msg,
						NET_CMD				command );

//	service provisioning methods
__checkReturn SRNET_RETURN_CODE				SvrNetSendResponseToClient(
						SERVICEUSERID,	//	use ServiceUserId(<type>,<inst>,INVALID_SVRCONNECTIONID) for server to server communications, svr to user requires valid SVRCONNECTIONID
						MSG_STRUCT *,
						NET_CMD );
__checkReturn SRNET_RETURN_CODE				SvrNetSendResponseToClients(
						SVRID				serverId,
						SVRCONNECTIONID *	connectedClients,
						UINT				numClients,
						MSG_STRUCT *		message,
						NET_CMD				command );
__checkReturn SRNET_RETURN_CODE				SvrNetSendBufferToClient(
						SERVICEUSERID userId,
						BYTE *pBuffer,
						DWORD bufferLen );
				//	pass NULL for mailslot to un-attach
void				SvrNetAttachRequestMailbox(
						SERVICEUSERID,
						SERVER_MAILBOX * );
BOOL				SvrNetPostFakeClientRequestToMailbox(
						SERVER_MAILBOX *	mailbox,
						SERVICEUSERID		sendingClient,
						MSG_STRUCT *		msg,
						NET_CMD				command );
LPVOID				SvrNetGetClientContext(
						SERVICEUSERID );
LPVOID				SvrNetDisconnectClient(
						SERVICEUSERID );

//	net message mailbox methods
				//	SEE: SvrMailboxHasMoreMessages
BOOL				SvrNetPopMailboxMessage(
						SERVER_MAILBOX *,
						MAILBOX_MSG * );
				//	SEE: SvrMailboxListHasMoreMessages
BOOL				SvrNetPopMailboxListMessage(
						MAILBOX_MSG_LIST *,
						MAILBOX_MSG * );

BOOL				SvrNetTranslateMessageForSend(
						SVRTYPE provider,
						SVRTYPE user,
						BOOL bRequest,
						NET_CMD cmd,
						MSG_STRUCT * msg,
						unsigned int & size,
						BYTE_BUF_NET & bbuf);

BOOL				SvrNetTranslateMessageForRecv(
						SVRTYPE provider,
						SVRTYPE user,
						BOOL bRequest,
						BYTE * data,
						unsigned int size,
						MSG_STRUCT * msg);

//----------------------------------------------------------------------------
// SERVER CLUSTER FUNCTIONS
//----------------------------------------------------------------------------
UINT16				SvrClusterGetServerCount(
						SVRTYPE );

#if ISVERSION(SERVER_VERSION)

BOOL				SvrClusterSendXMLMessagingMessage(
						WCHAR * message );

BOOL				SvrClusterSendXMLMessageToServerMonitorFromServer(
						WCHAR * bodyMsg );

#endif

//----------------------------------------------------------------------------
// SERVER RUNNER LoadBalancer Interface
//----------------------------------------------------------------------------
BOOL                SvrDBHeartbeat(
						SVRID id );


#endif	//	_SERVERRUNNERAPI_H_
