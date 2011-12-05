//----------------------------------------------------------------------------
// ServerRunnerNetwork.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _SERVERRUNNERNETWORK_H_
#define _SERVERRUNNERNETWORK_H_


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
typedef UINT16					SVRMACHINEINDEX;		// machine specific server index for a specific server instance on a machine
														//	(e.g. CHATSVR # 4 is the 2nd CHATSVR on machine 120.440.50.33, => it has a machine index of 1 while the 1st CHATSVR on the same machine has machine index 0)
typedef UINT16                  SVRPORT;

struct NETMSG
{
	BYTE *						Data;			// raw message bytes
	unsigned int				DataLength;		// number of raw message bytes
	NET_COMMAND_TABLE *			CmdTbl;			// the command table to use for decoding
	SERVER_MAILBOX *			Mailbox;		// the associated mailbox for storing
};


//----------------------------------------------------------------------------
// VALUE DEFINES
//----------------------------------------------------------------------------
#define INVALID_SVRPORT			((SVRPORT)0)
#define INVALID_SVRMACHINEINDEX	((SVRMACHINEINDEX)(-1))
#define RUNNER_NET_MAX_CLIENTS	60000


//----------------------------------------------------------------------------
// NETWORK SYSTEM FUNCTIONS
//----------------------------------------------------------------------------

struct NET_CONS;

BOOL
	RunnerNetInit(
		struct MEMORYPOOL *	pool,
		struct NET_MAP *	netMap,
		const char *		netName );

BOOL
	RunnerNetSetLocalIp(
		const char *		localIp );

void
	RunnerNetClose(
		void );

void
	RunnerNetAttemptOutstandingConnections(
		void );

const char *
	RunnerNetGetLocalIp(
		void );

BOOL
	RunnerNetAreAddressesEqual(
		SOCKADDR_STORAGE& lhs,
		SOCKADDR_STORAGE& rhs);

BOOL
	RunnerNetSockAddrToString(
		char * strDest,
		DWORD strLen,
		SOCKADDR_STORAGE * addr );

BOOL
	RunnerNetStringToSockAddr(
		char * strSource,
		SOCKADDR_STORAGE * addrDest );

//----------------------------------------------------------------------------
// SERVER NETWORK CONNECTIONS
//----------------------------------------------------------------------------

NET_CONS *
	RunnerNetEstablishConnections(
		struct MEMORYPOOL * pool,
		CONNECTION_TABLE *  tbl,
		SVRINSTANCE			serverInstance,
		SVRMACHINEINDEX		serverMachineIndex );

void
	RunnerNetFreeConnections(
		NET_CONS *			cons );

//----------------------------------------------------------------------------
// CLIENT-SIDE METHODS
//----------------------------------------------------------------------------

SRNET_RETURN_CODE
	RunnerNetSendRequest(
		NET_CONS *			cons,
		SERVICEID			serviceId,
		MSG_STRUCT *		msg,
		NET_CMD				request );
void
	RunnerNetAttachResponseMailbox(
		NET_CONS *			cons,
		SERVICEID			serviceId,
		SERVER_MAILBOX *	mailbox );

CONNECT_STATUS
	RunnerNetGetServiceStatus(
		NET_CONS *			cons,
		SERVICEID			serviceId );

//----------------------------------------------------------------------------
// SERVER-SIDE METHODS
//----------------------------------------------------------------------------
SRNET_RETURN_CODE
	RunnerNetSendResponse(
		NET_CONS *			cons,
		SERVICEUSERID		userId,
		MSG_STRUCT *		msg,
		NET_CMD				response );

SRNET_RETURN_CODE RunnerNetSendBuffer(
    	NET_CONS *		cons,
    	SERVICEUSERID	userId,
    	BYTE *	        buffer,
    	DWORD			bufferLen );
SRNET_RETURN_CODE
	RunnerNetSendResponseToClients(NET_CONS *cons,
		SVRID server,
		SVRCONNECTIONID *	connectedClients,
		UINT				numClients,
		MSG_STRUCT *		message,
		NET_CMD				command);

void
	RunnerNetAttachRequestMailbox(
		NET_CONS *			cons,
		SERVICEUSERID		userId,
		SERVER_MAILBOX *	mailbox );

CONNECT_STATUS
	RunnerNetGetClientStatus(
		NET_CONS *			cons,
		SERVICEUSERID		userId );

LPVOID
	RunnerNetDisconnectClient(
		NET_CONS *			cons,
		SERVICEUSERID		userId );

LPVOID
	RunnerNetGetClientContext(
		NET_CONS *		cons,
		SERVICEUSERID	userId );

//----------------------------------------------------------------------------
// MAILBOX METHODS
//----------------------------------------------------------------------------
BOOL
	RunnerNetPopMailboxMessage(
		SERVER_MAILBOX *,
		MAILBOX_MSG * );

BOOL
	RunnerNetPopMailboxMessage(
		MAILBOX_MSG_LIST *,
		MAILBOX_MSG * );

BOOL
	RunnerNetPostMailboxMsg(
		SERVER_MAILBOX *	mailbox,
		struct RAW_MSG_METADATA * metadata,
		MSG_STRUCT *		msg,
		NET_CMD				command );

BOOL
	RunnerNetTranslateMessageForSend(
		SVRTYPE provider,
		SVRTYPE user,
		BOOL bRequest,
		NET_CMD cmd,
		MSG_STRUCT * msg,
		unsigned int & size,
		BYTE_BUF_NET & bbuf);

BOOL 
	RunnerNetTranslateMessageForRecv(
		SVRTYPE provider,
		SVRTYPE user,
		BOOL bRequest,
		BYTE * data,
		unsigned int size,
		MSG_STRUCT * msg);

//----------------------------------------------------------------------------
// PRIVATE METHODS
//----------------------------------------------------------------------------
IMuxClient *RunnerNetGetMuxClient(NET_CONS* cons,SERVICEID serviceId);

NETCOM *
	RunnerNetHijackNetcom(
		void );

#endif	//	_SERVERRUNNERNETWORK_H_
