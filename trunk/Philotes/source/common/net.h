// ---------------------------------------------------------------------------
// FILE:		net.h
// DESCRIPTION:	io completion port based common client/server network library
//
//  Copyright 2003, Flagship Studios
// ---------------------------------------------------------------------------
#ifndef _NET_H_
#define _NET_H_


// ---------------------------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------------------------
#define DEFAULT_SERVER_PORT			5001


#define MSG_VAR						(-1)							// size of variable-length message in NET_CMD_DESC
#define MAX_NET_ADDRESS_LEN			1025							// this = NI_MAXHOST in WS2tcpip.h
#define NET_NAME_SIZE				9								// debug name for client/server instance

#define MAX_READ_BUFFER				4096							// should be page size on windows 

#define INVALID_NETCLIENTID			(NETCLIENTID)(-1)				// invalid client id
#define INVALID_NETCLIENTID64		(NETCLIENTID64)(-1)				// invalid client id for 64 bits


// ---------------------------------------------------------------------------
// TYPEDEFS
// ---------------------------------------------------------------------------
typedef DWORD						NETCLIENTID;					// client id
typedef UINT64						NETCLIENTID64;					// 64 bit clientID for serverrunner.
typedef BOOL (*FP_NETSERVER_ACCEPT_CALLBACK)(void *pAcceptContext,NETCLIENTID idClient, struct sockaddr_storage *local,struct sockaddr_storage *remote);	// callback on accept
typedef BOOL (*FP_NETSERVER_DISCONNECT_CALLBACK)(struct NETCOM *net,NETCLIENTID idClient);				// callback on disconnect
typedef BOOL (*FP_NETSERVER_REMOVE_CALLBACK)(NETCLIENTID idClient, void * data);	// callback on remove
typedef void (*FP_NETCLIENT_INIT_USERDATA)(void * data);			// init user data function
typedef void (*FP_NETCLIENT_CONNECT_CALLBACK)(NETCLIENTID idClient, void * cltData, DWORD status );	//	called once client connect status is finalized, passed status connected or status connect failed

// Direct IO read function
//
// READ_CALLBACK is called when data arrives on the socket.
//      - fills in pdwBytesConsumed, Net layer will buffer the remaining.
//      - returns FALSE to stop processing. Further data received will be
//        dropped (anticipating that the connection will be closed soon.
//
//
typedef BOOL (*FP_DIRECT_READ_CALLBACK)(struct NETCOM *pNet,
                                        NETCLIENTID clientId,
                                        DWORD dwError,
                                        BYTE *pBuffer,
                                        DWORD dwBufSize,
                                        DWORD *pdwBytesConsumed);

// ENUMERATIONS
// ---------------------------------------------------------------------------
enum MSG_SOURCE
{
	MSG_SOURCE_CLIENT,
	MSG_SOURCE_GAMESERVER,
	MSG_SOURCE_DATABASE,
};

enum CONNECT_STATUS
{
	CONNECT_STATUS_DISCONNECTED,
	CONNECT_STATUS_CONNECTING,
	CONNECT_STATUS_CONNECTED,
	CONNECT_STATUS_CONNECTFAILED,
	CONNECT_STATUS_DISCONNECTING,
};

enum NAGLE_TYPE
{
	NAGLE_DEFAULT,
	NAGLE_OFF,
};


// ---------------------------------------------------------------------------
// STRUCTS
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// FORWARD DECLARATION
// ---------------------------------------------------------------------------
struct NETCOM;
struct NETSRV;
struct NETCLT;
struct NetPerThreadBufferPool;


// ---------------------------------------------------------------------------
// DESC:	mail message container
// ---------------------------------------------------------------------------
struct MSG_BUF
{
	union 
	{
		MSG_BUF	*					next;							// singly-linked list
		LONGLONG					buf;
	};
    NetPerThreadBufferPool *        parent_ptbp;

    unsigned int                    allocated_size;                 // size of the buffer
	unsigned int					size;							// size of data in msg
	unsigned int					source;							// source enumeration
	NETCLIENTID						idClient;						// client that sent the message
	BYTE							msg[1];
};


// ---------------------------------------------------------------------------
// DESC:	net setup structure
// ---------------------------------------------------------------------------
struct net_setup
{
	MEMORYPOOL	*					mempool;						// memory pool
	const char *					name;							// name
	unsigned int					max_clients;					// maximum # of connections
	unsigned int					user_data_size;					// size of user data block for each NETCLT
    long                            num_completion_ports;           // allow multiple completion ports;
};


// ---------------------------------------------------------------------------
// DESC:	common setup base structure
// ---------------------------------------------------------------------------
struct net_cmn_setup
{
	const char *					name;							// name

	const char *					local_addr;						// local address
	unsigned short					local_port;						// local port

	unsigned int					nagle_value;					// NAGLE_DEFAULT uses default nagle buffering, NAGLE_OFF turns it off, any other value sets custom nagle buffer

    void *                          user_data; //allow directly specifying userdata
	BOOL							sequential_writes;				// only allow one write at a time
    BOOL                            dont_enforce_thread_safe_writes; //don't put a lock around send(), which is usually
                                                                     // required so that encryption does not get 
                                                                     // messed up (Bug 50689).
    // Multiplexing 
    BOOL                            multiplex_connection;
	//
	BOOL							disable_send_buffer;
    //
    BOOL                            non_blocking_socket;
    long                            completion_port_index; //which completion port to associate with
	unsigned int                    max_pending_sends;			// number of sends outstanding.
	unsigned int                    max_pending_bytes;			// allowed pending bytes per client connection.
};


// ---------------------------------------------------------------------------
// DESC:	server setup structure
// ---------------------------------------------------------------------------
struct net_srv_setup : public net_cmn_setup
{
	BOOL							local;							// set to TRUE if we don't want to connect to the network

	unsigned int					max_clients;					// maximum # of connections
	unsigned int					socket_precreate_count;			// # of sockets for each addr to precreate
	unsigned int					accept_backlog;					// # of accepts to post
	unsigned int					listen_backlog;					// listen backlog


	struct MAILSLOT *				mailslot;						// mail slot for incoming messages

	struct NET_COMMAND_TABLE *		cmd_tbl;						// command table


    void *                          accept_context;
	FP_NETSERVER_ACCEPT_CALLBACK	fp_AcceptCallback;				// callback on accept
	FP_NETSERVER_DISCONNECT_CALLBACK	fp_DisconnectCallback;		// callback on disconnect
	FP_NETSERVER_REMOVE_CALLBACK	fp_RemoveCallback;				// callback on remove
	FP_NETCLIENT_INIT_USERDATA		fp_InitUserData;				// init user data function
    FP_DIRECT_READ_CALLBACK         fp_DirectReadCallback;
};


// ---------------------------------------------------------------------------
// DESC:	client setup structure
// ---------------------------------------------------------------------------
struct net_clt_setup : public net_cmn_setup
{


	const char *					remote_addr;					// remote address
	unsigned short					remote_port;					// remote port

	unsigned int					socket_precreate_count;			// # of sockets to precreate


	struct MAILSLOT *				mailslot;						// mail slot for incoming messages

	struct NET_COMMAND_TABLE *		cmd_tbl;						// command table

	FP_NETCLIENT_INIT_USERDATA		fp_InitUserData;				// init user data function

	FP_NETCLIENT_CONNECT_CALLBACK	fp_ConnectCallback;				// called once connection success or failure is determined

    FP_DIRECT_READ_CALLBACK         fp_DirectReadCallback;
};


// ---------------------------------------------------------------------------
// DESC:	local connection structure
// ---------------------------------------------------------------------------
struct net_loc_setup
{
	const char *					srv_name;						// name to use for server-side client
	const char *					clt_name;						// name to use for client-side client
	
	struct MAILSLOT *				clt_mailslot;					// client side mailslot (where client-client receives mail)

	struct NET_COMMAND_TABLE *		clt_cmd_tbl;					// client side command table (messages to the client)

	FP_NETCLIENT_INIT_USERDATA		clt_fpInitUserData;				// init user data function
	FP_NETCLIENT_INIT_USERDATA		srv_fpInitUserData;				// init user data function
};


// ---------------------------------------------------------------------------
// DESC:	exported functions
// ---------------------------------------------------------------------------
NETCOM * NetInit(
	const net_setup & setup);

void NetClose(
	NETCOM * net);

// frees any previously set lists.
BOOL NetSetBanList(
	NETCOM * net,
	NETSRV * srv,
	struct IPADDRDEPOT * list );

void SetThreadName(
	const char * name,
	DWORD dwThreadID = -1);

unsigned int NetGetLocalIpAddr(
	char * str,
	ULONG * family,
	unsigned int size,
	unsigned int count);

unsigned int NetGetServerIpToUse(
	char * str,
	unsigned int size,
	unsigned int count);

BOOL NetGetClientMacAddress(
	const char * targetUrlOrIPv4Address,
	char * outMacAddress,
	DWORD outMacAddressLen );

NETSRV * NetCreateServer(
	NETCOM * net,
	const net_srv_setup & setup);

NETCLIENTID NetCreateClient(
	NETCOM * net,
	const net_clt_setup & setup);

BOOL NetCreateLocalConnection(
	NETCOM * net,
	NETSRV * srv,
	const net_loc_setup & setup,
	NETCLIENTID * idClient,
	NETCLIENTID * idServer);

CONNECT_STATUS NetClientGetConnectionStatus(
	NETCOM * net,
	NETCLIENTID idClient);

BOOL NetClientIsConnected(
	NETCOM * net,
	NETCLIENTID idClient);

// Returns < 0 for error, 0 if the send threshold was exceeded and no data was sent.
int NetSend(
	NETCOM * net,
	NETCLIENTID idClient,
	const void * data,
	unsigned int size);

int NetMultipointSend(
	NETCOM * net,
	__in_ecount(numClients) NETCLIENTID *idClient,
    DWORD        numClients,
	__in_bcount(size) const void * data,
	unsigned int size);

BOOL NetCreatePrivatePerThreadBufferPool(
	NETCOM *net);

BOOL NetCloseClient(
	NETCOM * net,
	NETCLIENTID idClient,
	BOOL bGraceful = TRUE);

//	frees any set ban list.
void NetCloseServer(
	NETCOM * net,
	NETSRV * srv);	

BOOL NetGetClientAddress(
	NETCOM * net,
	NETCLIENTID idClient,
	sockaddr_storage * addrDest );

BOOL NetGetClientServerPort(
	NETCOM *	net,
	NETCLIENTID	idClient,
	UINT16 *	port );

BOOL NetAddClientRef(CRITSEC_FUNCARGS(
	NETCOM * net,
	NETCLIENTID idClient));

BOOL NetSubClientRef(CRITSEC_FUNCARGS(
	NETCOM * net,
	NETCLIENTID idClient));

// note, these functions don't lock the user data, they simply make sure that NETCLT isn't deleted while you've got it
// every call to NetClientGetUserData() must be followed by a call to NetClientReleaseUserData()

void* NetGetClientUserData(CRITSEC_FUNCARGS(
	NETCOM * net, 
	NETCLIENTID idClient));
	
void NetReleaseClientUserData(CRITSEC_FUNCARGS(
	NETCOM * net, 
	NETCLIENTID idClient));

void NetSetClientMailbox(
	NETCOM * net,
	NETCLIENTID idClient,
	struct MAILSLOT * mailslot);

void NetSetClientResponseTable(
	NETCOM * net,
	NETCLIENTID idClient,
	struct NET_COMMAND_TABLE * newTable);

#include "encoder_def.h"

void NetInitClientSendEncryption(
	NETCOM * net,
	NETCLIENTID idClient,
	BYTE * key,
	size_t nBytes,
	ENCRYPTION_TYPE eEncryptionType = ENCRYPTION_TYPE_ARC4);

void NetInitClientReceiveEncryption(
	NETCOM * net,
	NETCLIENTID idClient,
	BYTE * key,
	size_t nBytes,
	ENCRYPTION_TYPE eEncryptionType = ENCRYPTION_TYPE_ARC4);

inline void NetInitClientEncryption(
	NETCOM * net,
	NETCLIENTID idClient,
	BYTE * sendkey,
	size_t nSendBytes,
	BYTE * receivekey,
	size_t nReceiveBytes)
{
	NetInitClientSendEncryption(net, idClient, sendkey, nSendBytes);
	NetInitClientReceiveEncryption(net, idClient, receivekey, nReceiveBytes);
}


void DbgPrintBuf(
	char * str,
	int strlen,
	const BYTE * buf,
	int buflen);

inline NETCLIENTID NetClientID64to32(NETCLIENTID64 idNetClient)
{
	ASSERT( (idNetClient >> 32) == 0 || NETCLIENTID(idNetClient >> 32) == NETCLIENTID(-1) );
	return NETCLIENTID(idNetClient); //tell me if this code fails.  -rjd
}


#endif // _NET_H_
