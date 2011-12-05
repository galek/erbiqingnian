// ---------------------------------------------------------------------------
// FILE:	net.cpp
// DESC:	io completion port based common client/server network library
//
// TODO:	last BLOB field doesn't need to write size
//			keepalive
//			hacklist
//			create sockets in separate thread?
//			4K msg bufs?
//			short-lived connection mode
//			encryption / sequence #s
//			NOTE - self nagle buffering is really slow because it's done inside
//				a lock.  if we can make certain assumptions (ie. user maintained
//				sequentiallity, we can remove the locking and it'll be fast)
//
// REMEMBER:	ordering issues possible on NetSend() see function for details
//
//  Copyright 2003, Flagship Studios
// ---------------------------------------------------------------------------

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <mstcpip.h>
#include <iphlpapi.h>
#include <conio.h>
#include "singleton.h"
#include "cpu_usage.h"
#include "mailslot.h"
#include "random.h"
#include "ipaddrdepot.h"
#include "encoder.h"
#include <list>
#include <vector>
#if ISVERSION(SERVER_VERSION)
#include "net.cpp.tmh"
#include <stdCommonIncludes.h>
#endif


// ---------------------------------------------------------------------------
// LIBRARIES
// ---------------------------------------------------------------------------
#pragma comment(lib, "ws2_32.lib")


// ---------------------------------------------------------------------------
// DEBUGGING CONSTANTS
// ---------------------------------------------------------------------------
#if ISVERSION(RETAIL_VERSION)
#define NET_TRACE_0						0										// trace low-level network operations
#define NET_TRACE_1						0										// trace high-level network operations
#define NET_TRACE_MSG0					0										// trace messages on translate
#define NET_TRACE_MSG					0										// trace message contents
#define NET_TRACE_MSG2					0										// trace messages on translate
#define NET_DEBUG						0										// keep debug counts
#define NET_MSG_COUNT					0										// count number of messages
#define NET_STRAND_DEBUG				0										// debug buffer strand system
#else
#define NET_TRACE_0						1										// trace low-level network operations
#define NET_TRACE_1						1										// trace high-level network operations
#define NET_TRACE_MSG0					1										// trace messages on translate
#define NET_TRACE_MSG					0										// trace message contents
#define NET_TRACE_MSG2					0//!ISVERSION(SERVER_VERSION)				// trace messages on translate
#define NET_DEBUG						1										// keep debug counts
#define SEND							1										// do sends?
#define NET_MSG_COUNT					1										// count number of messages
#define NET_STRAND_DEBUG				1										// debug buffer strand system
#endif

#define SEND							1										// do sends?

#if ISVERSION(DEBUG_VERSION)
#define ENFORCE_NO_NAGLE				1
#else
#define ENFORCE_NO_NAGLE				0
#endif


// ---------------------------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------------------------
#define HUERISTIC_VALUE					1										// # of worker threads per processor
#define WORKER_PRIORITY					THREAD_PRIORITY_NORMAL					// worker thread priority

#define DEFAULT_MAX_CLIENTS				32										// default max client connections
#define MAX_SERVERS						64										// max servers 
#define DEFAULT_BACKLOG					10      								// listen backlog

#define DEFAULT_FAMILY					PF_UNSPEC								// unspecified.  IPv4 = PF_INET, IPv6 = PF_INET6
#define DEFAULT_SOCKTYPE				SOCK_STREAM								// TCP.   UDP = SOCK_DGRAM

																				// constants for message definitions
#define	MAX_MSG_STRUCT_STACK			16										// max nesting level for msg struct
#define MAX_MSG_FIELDS_TOTAL			1024									// max fields total
#define MAX_MSG_FIELDS					21										// max msg fields (if you change this, add appropriate functions to BASE_MSG_STRUCT

#define PER_THREAD_PRECREATE_COUNT		100										// pre-create a bunch of overlaps and msg_bufs per thread

// no more sends permitted at a time to a client
#if ISVERSION(SERVER_VERSION)
const DWORD MAX_SENDS_PERMITTED=(DWORD)-1;
const DWORD MAX_SEND_BYTES_PERMITTED=(DWORD)-1;
#else
const DWORD MAX_SENDS_PERMITTED=(DWORD)-1 ;// don't confuse the client with errors
const DWORD MAX_SEND_BYTES_PERMITTED=(DWORD)-1;
#endif

#if ISVERSION(SERVER_VERSION)
#define DEFAULT_MAX_STRAND_SIZE			1024									// max size of msgbuf or overlap strands (see CStrandPool)
#else
#define DEFAULT_MAX_STRAND_SIZE			16									
#endif


#define MAX_WORKER_THREADS				32										// maximum number of iocp worker threads


// ---------------------------------------------------------------------------
// MACROS
// ---------------------------------------------------------------------------
#define NETCLIENTID_GET_INDEX(id)		((id) & 0xffff)							// get the array index from the client id
#define NETCLIENTID_GET_ITER(id)		(WORD)((id) >> 16)						// get the iter from the client id
#define MAKE_NETCLIENTID(idx, itr)		(((itr) << 16) + ((idx) & 0xffff))		// construct client id from index + iter


// ---------------------------------------------------------------------------
// TYPEDEFS
// ---------------------------------------------------------------------------
typedef DWORD (WINAPI *FP_GETADAPTERSADDRESSES)(ULONG Family, DWORD Flags, PVOID Reserved, PIP_ADAPTER_ADDRESSES pAdapterAddresses, PULONG pOutBufLen);
typedef DWORD (WINAPI *FP_GETBESTINTERFACE)(IPAddr dwDestAddr, PDWORD pdwBestIfIndex);
typedef DWORD (WINAPI *FP_GETIFTABLE)(PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder);
typedef BOOL (*FP_NET_SEND)(NETCOM * net, struct NETCLT * clt, const void * data, unsigned int size);


// ---------------------------------------------------------------------------
// ENUMERATIONS
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// DESC:	status of server (used for both NETCOM and NETSRV)
// ---------------------------------------------------------------------------
enum SERVER_STATE
{
	SERVER_STATE_NONE,												// uninitialized server
	SERVER_STATE_READY,												// listening server
	SERVER_STATE_SHUTDOWN,											// shutting down
};


// ---------------------------------------------------------------------------
// DESC:	status of client
// ---------------------------------------------------------------------------
enum NETCLIENT_STATE
{
	NETCLIENT_STATE_NONE,											// disconnected client
	NETCLIENT_STATE_LISTENING,										// subject of acceptex (server clt only)
	NETCLIENT_STATE_CONNECTING,										// attempting to connect (client clt only)
	NETCLIENT_STATE_CONNECTED,										// connected client
	NETCLIENT_STATE_DISCONNECTING,									// disconnecting client
	NETCLIENT_STATE_CONNECTFAILED,									// failed to connect for some reason (client clt only)
};


// ---------------------------------------------------------------------------
// DESC:	type of io operation (used in overlap)
// ---------------------------------------------------------------------------
enum IOType 
{
	IOError,
	IOConnect,														// connect ex
	IOAccept,														// accept ex
	IORead,															// read complete
	IOWrite,														// write complete
    IOMultipointWrite,												// multi-point write complete

	IODisconnectEx,													// disconnect complete

	IOLast,
};


// ---------------------------------------------------------------------------
// STRUCTS
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// DESC:	extended OVERLAPPED structure
// ---------------------------------------------------------------------------
struct OVERLAPPEDEX
{
	OVERLAPPED							m_Overlapped;
	IOType								m_ioType;
	NETCLIENTID							m_idClient;
	WSABUF								m_wsabuf;
        MSG_BUF *						m_buf;
	OVERLAPPEDEX *						m_pNext;
    NetPerThreadBufferPool *        	m_pParentPTBP;
#if ISVERSION(DEBUG_VERSION)
	DWORD								m_dwTickCount;
#endif
};


// ---------------------------------------------------------------------------
// DESC:	read buffer
// ---------------------------------------------------------------------------
struct NET_READ_BUF
{
	BYTE *								m_Buffer;								// read buffer
    unsigned int                    	m_nBufferSize;							// actual size of buffer
	unsigned int						m_nLen;									// bytes in read buffer	
	unsigned int						m_nCurMessageSize;						// cache of current message size
	NET_CMD								m_curCmd;								// cache of current command
};


// ---------------------------------------------------------------------------
// DESC:	write buffer
// ---------------------------------------------------------------------------
struct NET_WRITE_BUF
{
	// the following section is volatile
	CCritSectLite						m_csWriteBuf;							// critical section
	MSG_BUF *							m_pPendingHead;							// head/tail list to pending send buffers
	MSG_BUF *							m_pPendingTail;
	BOOL								m_bDefaultBufferInUse;					// 0 = not using m_Buffer, 1 = using m_Buffer
	BOOL								m_bSending;								// serial: 1 = sending

	MSG_BUF *							m_Buffer;								// write buffer
};



// ---------------------------------------------------------------------------
// DESC:	node for socket pool
// ---------------------------------------------------------------------------
struct CSocketNode
{
	SOCKET								m_hSocket;								// this will be a valid socket handle if on sockets list and INVALID_SOCKET on free list
	CSocketNode *						m_pNext;
};


// ---------------------------------------------------------------------------
// DESC:	socket pool
//			placed in WSOCK_EXTENDED because sockets are created specifically
//			for a particular family / socktype/ protocol
//			sockets are added to the sockets list once they've been used
//			the free list contains nodes for use by the socket list but
//			don't contain sockets
// ---------------------------------------------------------------------------
struct CSocketPool
{
	// this section is assumed to be set on init
	unsigned int						m_nFamily;
	unsigned int						m_nSocktype;
	unsigned int						m_nProtocol;

	// this section is volatile
	CCritSectLite						m_csSockets;							// crit sect for socket list
	CSocketNode *						m_pSocketsHead;							// head of socket list
	CSocketNode *						m_pSocketsTail;							// tail of socket list

	CCritSectLite						m_csFreeNodes;							// crit sect for free nodes list
	CSocketNode *						m_pFreeNodes;							// head of free nodes list
	volatile LONG						m_nSocketCount;							// number of sockets created
};


// ---------------------------------------------------------------------------
// DESC:	provides extended winsock functions
//			NETCOM initializes these whenever a new server is created that
//			doesn't match the family of an existing WSOCK_EXTENDED
// ---------------------------------------------------------------------------
BOOL FakeConnectEx(
   NETCOM *net,
   SOCKET sock,
   const struct sockaddr * name,
   int namelen,
   OVERLAPPEDEX * lpOverlapped);

BOOL FakeDisconnectEx(
		NETCOM *net,
		SOCKET sock,
		OVERLAPPEDEX * lpOverlapped,
		DWORD dwFlags);

struct WSOCK_EXTENDED
{
public:
	CSocketPool							m_SocketPool;							// pool of sockets of a given family

private:
	BOOL								m_bInitialized;

	LPFN_ACCEPTEX						m_fnAcceptEx;							// AcceptEx winsock function
	LPFN_GETACCEPTEXSOCKADDRS			m_fnGetAcceptExSockaddrs;				// GetAcceptExSockaddrs function
	LPFN_CONNECTEX						m_fnConnectEx;							// ConnectEx winsock function
	LPFN_DISCONNECTEX					m_fnDisconnectEx;						// DisconnectEx winsock function

	// get AcceptEx winsock function
	void GetAcceptEx(
		SOCKET sock)
	{
		GUID gufn = WSAID_ACCEPTEX;
		DWORD dwBytes;
		m_fnAcceptEx = NULL;
		WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &gufn, sizeof(gufn), &m_fnAcceptEx, sizeof(m_fnAcceptEx), &dwBytes, NULL, NULL);
		ASSERT(m_fnAcceptEx);
		// if unavailable, we could provide a fake version?
	}

	// get GetAcceptExSockaddrs winsock function
	void GetGetAcceptExSockaddrs(
		SOCKET sock)
	{
		GUID gufn = WSAID_GETACCEPTEXSOCKADDRS;
		DWORD dwBytes;
		m_fnGetAcceptExSockaddrs = NULL;
		WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &gufn, sizeof(gufn), &m_fnGetAcceptExSockaddrs, sizeof(m_fnGetAcceptExSockaddrs), &dwBytes, NULL, NULL);
		ASSERT(m_fnGetAcceptExSockaddrs);
		// if unavailable, we could provide a fake version?
	}

	// get ConnectEx winsock function
	void GetConnectEx(
		SOCKET sock)
	{
		GUID  gufn = WSAID_CONNECTEX;
		DWORD dwBytes;
		m_fnConnectEx = NULL;
		WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &gufn, sizeof(gufn), &m_fnConnectEx, sizeof(m_fnConnectEx), &dwBytes, NULL, NULL);
		ASSERT(m_fnConnectEx);
		// if unavailable, we could provide a fake version?
	}

	// get DisconnectEx winsock function
	void GetDisconnectEx(
		SOCKET sock)
	{
		GUID  gufn = WSAID_DISCONNECTEX;
		DWORD dwBytes;
		m_fnDisconnectEx = NULL;
		WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &gufn, sizeof(gufn), &m_fnDisconnectEx, sizeof(m_fnDisconnectEx), &dwBytes, NULL, NULL);
		ASSERT(m_fnDisconnectEx);
		// if unavailable, we could provide a fake version?
	}

public:
	void Initialize(
		SOCKET sock,
		unsigned int family,
		unsigned int socktype,
		unsigned int protocol)
	{
		ASSERT_RETURN(sock != INVALID_SOCKET);
		if (m_bInitialized)
		{
			ASSERT(family == m_SocketPool.m_nFamily);
			ASSERT(socktype == m_SocketPool.m_nSocktype);
			ASSERT(protocol == m_SocketPool.m_nProtocol);
			return;
		}
		GetAcceptEx(sock);
		GetGetAcceptExSockaddrs(sock);
		GetConnectEx(sock);
		GetDisconnectEx(sock);
		m_SocketPool.m_nFamily = family;
		m_SocketPool.m_nSocktype = socktype;
		m_SocketPool.m_nProtocol = protocol;
		m_bInitialized = TRUE;
	}

	// wrapper for AcceptEx winsock function
	BOOL NetAcceptEx(
		SOCKET sockListen,
		SOCKET sockAccept,
		PVOID lpOutputBuffer,
		DWORD dwReceiveDataLength,
		DWORD dwLocalAddressLength,
		DWORD dwRemoteAddressLength,
		LPDWORD lpdwBytesReceived,
		OVERLAPPEDEX * lpOverlapped)
	{
		ASSERT_RETFALSE(m_fnAcceptEx);
		return m_fnAcceptEx(sockListen, sockAccept, lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, (LPOVERLAPPED)lpOverlapped);
	}

	// wrapper for GetAcceptExSockaddrs winsock function
	BOOL NetGetAcceptExSockaddrs(
		PVOID lpOutputBuffer,
		DWORD dwReceiveDataLength,
		DWORD dwLocalAddressLength,
		DWORD dwRemoteAddressLength,
		struct sockaddr **LocalSockaddr,
		LPINT LocalSockaddrLength,
		struct sockaddr **RemoteSockaddr,
		LPINT RemoteSockaddrLength)
	{
		ASSERT_RETFALSE(m_fnGetAcceptExSockaddrs);
		m_fnGetAcceptExSockaddrs(lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength, LocalSockaddr, LocalSockaddrLength, RemoteSockaddr, RemoteSockaddrLength);
		return TRUE;
	}

	// wrapper for ConnectEx winsock function
	BOOL ConnectEx(
		SOCKET sock,
		const struct sockaddr * name,
		int namelen,
		PVOID lpSendBuffer,
		DWORD dwSendDataLength,
		LPDWORD lpdwBytesSent,
		OVERLAPPEDEX * lpOverlapped)
	{
		ASSERT_RETFALSE(m_fnConnectEx);
		return m_fnConnectEx(sock, name, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, (LPOVERLAPPED)lpOverlapped);
	}

	// wrapper for DisconnectEx winsock function
	BOOL NetDisconnectEx(
		SOCKET sock,
		OVERLAPPEDEX * lpOverlapped,
		DWORD dwFlags)
	{
		ASSERT_RETFALSE(m_fnDisconnectEx);
		return m_fnDisconnectEx(sock, (LPOVERLAPPED)lpOverlapped, dwFlags, 0);
	}
};


// ---------------------------------------------------------------------------
// DESC:	structure for a client
//			this is used both for clients on the actual client and
//			clients of a server.  if on the actual client, the m_pServer
//			member will be NULL, otherwise it'll be set to the NETSRV
//			the client is connected to
//			uses m_ref to control locking (creation/destruction)
//			however, this isn't a real lock, so members should be considered
//			volatile
// ---------------------------------------------------------------------------
struct NETCLT
{
	// this section is only altered on initialization or destruction
	// which may occur from any thread, but only if no other thread has
	// a soft lock
	SOCKET								m_hSocket;								// client socket
	NETCLIENTID							m_id;									// client id
	NETCLT *							m_pNext;								// next client in free list or client list
	NETCLT *							m_pPrev;								// prev client in client list

	NETSRV *							m_pServer;								// listener this client is connected to (NULL for client side NETCLT)
	FP_NETCLIENT_CONNECT_CALLBACK		m_fpConnectCallback;					// client side connect status callback

	char								m_szName[NET_NAME_SIZE];				// debug name
	WORD								m_wIter;								// current iter

	WSOCK_EXTENDED *					m_WsockFn;								// winsock extended functions
	FP_NET_SEND							m_fpNetSend;							// send function

	CCritSectLite						m_csCmdTbl;                     		// lock for changing command table.
	NET_COMMAND_TABLE *					m_CmdTbl;								// command table

	sockaddr_storage					m_addr;									// remote ip address of client

	LONG								m_bCounted;								// set to 1 on accept

	unsigned int						m_nNagle;								// nagle type (0 == on, everything else makes it use custom nagle buffering)
	BOOL								m_bSequentialWrites;					// only allow one write at a time

    BOOL                            	m_bCloseCalled;                 		// don't invoke callbacks if NetCloseClient was called.
	BOOL								m_bWasAccepted;							// don't invoke removed if accept refused.
    BOOL                            	m_bNonBlocking;

	NETCLIENTID							m_idLocalConnection;					// clientid if this is a local connection
    LONG                                m_nWorkerCompletionPortIndex;
    LONG                            	m_nMaxOutstandingSends;         		// fail send call if too many sends pending
    LONG                            	m_nMaxOutstandingBytes;         		// fail send call if too many bytes pending

	// WARNING regarding state: client state is very soft &
	// is highly unsafe so never do anything with it that's 
	// dependent upon it being the same after you read it
	volatile LONG						m_eClientState;							// state of client

	volatile LONG						m_nOutstandingOverlaps[IOLast];			// track overlaps

	volatile LONG						m_nOutstandingBytes; //tracks bytes outstanding.
#if NET_MSG_COUNTER
	LONG								m_nMsgSendCounter;
	LONG								m_nMsgRecvCounter;
#endif 

	// this section contains stuff that is volatile
	CRefCount							m_ref;									// main lock for client

	NET_READ_BUF						m_bufRead;								// read buffer
	NET_WRITE_BUF						m_bufWrite;								// write buffer

	Encoder								m_encoderSend;							// handles NetSend encryption
	Encoder								m_encoderReceive;						// handles NetProcessReadBuffer decryption
	unsigned int						m_bBytesUnencrypted;					// number of remaining unencrypted bytes

	volatile LONG						m_nMsgIn;								// message counts
	volatile LONG						m_nMsgOut;
	volatile LONG						m_bBytesIn;
	volatile LONG						m_bBytesOut;

	// WARNING the mailslot of a client can be set in the main thread
	// AFTER the usual initialization/destruction process
	CCritSectLite						m_csMailSlot;							// crit sect for mail slot
	MAILSLOT *							m_mailslot;								// mailslot for incoming messages

    // no cmd table interpretation etc.
	FP_DIRECT_READ_CALLBACK				m_fpDirectReadCallback;

    CCritSectLite                       m_csSendLock;
    BOOL                                m_bDontLockOnSend;
};


// ---------------------------------------------------------------------------
// DESC:	client table
//			contained in NETCOM.  the client table is shared by both client
//			and server-side clients.
// ---------------------------------------------------------------------------
struct CClientTable
{
	// this section is only written to on init/free 
	// and is assumed non-volatile
	void *								m_pClients;								// array of client structures
	unsigned int						m_nArraySize;							// size of client array (max # of clients)
	unsigned int						m_nClientSize;							// sizeof(NETCLT) + m_nUserDataSize rounded to closest bigger power 2
	unsigned int						m_nArrayShift;							// amount to shift a ptr address by to get the index

	BYTE *								m_pReadBuffer;							// read buffer for all clients in flat memory
	BYTE *								m_pWriteBuffer;							// write buffer for all clients in flat memory

	// this section is volatile
	volatile LONG						m_nClientCount;							// client count
	volatile LONG						m_nCurrentIter;							// current iter

	CCritSectLite						m_csFreeList;							// crit sect for free list of clients
	NETCLT *							m_pFreeList;							// free (recycle) list of clients 
};


// ---------------------------------------------------------------------------
// DESC:	structure for a "connection" ie a listener object
//			these should only get created & destroyed from the main thread
//			they are contained by NETCOM in an array
//			the structure isn't locked, so individual members should be
//			treated as volatile
// ---------------------------------------------------------------------------
struct NETSRV
{
	// this section is only written to on init/free 
	// and is assumed non-volatile
	SOCKET								m_hSocket;								// accept connections on this socket
	NETCOM *							net;									// pointer to net structure
	BOOL								m_bLocal;								// set to TRUE if it's a local server (for single player)

	char								m_szName[NET_NAME_SIZE];				// debug name

	sockaddr_storage					m_addr;									// ip address of server (local)
	unsigned int						m_addrlen;								// length of m_addr
	char								m_szAddr[NI_MAXHOST];					// str address
	unsigned short						m_nPort;								// port
	unsigned int						m_nFamily;								// socket family
	unsigned int						m_nSockType;							// socket type
	unsigned int						m_nProtocol;							// protocol

	WSOCK_EXTENDED *					m_WsockFn;								// winsock extended functions
	FP_NET_SEND							m_fpNetSend;							// send function

	LONG								m_nAcceptBacklog;						// # of desired outstanding AcceptEx
    LONG                            	m_nMaxOutstandingSends;         		// fail send call if too many sends pending
    LONG                            	m_nMaxOutstandingBytes;         		// fail send call if too many bytes pending
    LONG                                m_nWorkerCompletionPortIndex;

	unsigned int						m_nNagle;								// nagle type (0 == on, everything else makes it use custom nagle buffering)
	BOOL								m_bSequentialWrites;					// only allow one write at a time
    BOOL                                m_bDontLockOnSend;                      // don't enforce "one send at a time";
    BOOL                            	m_bNonBlocking;

	NET_COMMAND_TABLE *					m_CmdTbl;								// command table
	MAILSLOT *							m_mailslot;								// mailslot for incoming messages

	// this section is volatile
	volatile LONG						m_eServerState;							// state of server

	volatile LONG						m_nOutstandingAccepts;					// # of async AcceptEx requests

	volatile LONG						m_nClientCount;							// # of clients

    void *                          	m_pAcceptContext;
	FP_NETSERVER_ACCEPT_CALLBACK		m_fpAcceptCallback;						// callback on accept
	FP_NETSERVER_DISCONNECT_CALLBACK	m_fpDisconnectCallback;					// callback on disconnect
	FP_NETSERVER_REMOVE_CALLBACK		m_fpRemoveCallback;						// callback on remove
	FP_NETCLIENT_INIT_USERDATA			m_fpInitUserData;						// init user data function

    // no cmd table interpretation etc.
    FP_DIRECT_READ_CALLBACK				m_fpDirectReadCallback;

	IPADDRDEPOT *						m_banList;								// list of banned ip addresses
	CReaderWriterLock_NS_FAIR			m_banListLock;							// banned list sync lock

    // server perf counters
    PERF_INSTANCE(CommonNetServer) *	m_PerfInstance;
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static inline MEMORYPOOL * NetGetMemPool(
	NETCOM * net);


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static inline OVERLAPPEDEX * & StrandGetNext(
	OVERLAPPEDEX * overlap)
{
	return overlap->m_pNext;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static inline MSG_BUF * & StrandGetNext(
	MSG_BUF * msgbuf)
{
	return msgbuf->next;
}


// ---------------------------------------------------------------------------
// DESC:	global pools for strands of stuff (msgbufs and overlaps)
// ---------------------------------------------------------------------------
template <typename T>
struct CStrandPool
{
private:
	static const unsigned int MAX_GLOBAL_TOTAL_STRAND_SIZE = DEFAULT_MAX_STRAND_SIZE*5;

	template <typename T>
	struct CStrandPoolNodeT
	{
		T *								m_pStrand;
		unsigned int					m_nLength;
		CStrandPoolNodeT<T> *			m_pNext;
	};
	typedef CStrandPoolNodeT<T>			CStrandPoolNode;

	CCritSectLite						m_csStrandPool;
	CStrandPoolNode *					m_pHead;
	CStrandPoolNode *					m_pGarbage;
    DWORD                               m_pTotalLength;

	inline void sFreeStrand(
		MEMORYPOOL * pool,
		T * strand,
		unsigned int length)
	{
		T * curr = NULL;
		T * next = strand;
#if NET_STRAND_DEBUG
		unsigned int count = 0;
#endif
		while ((curr = next) != NULL)
		{
			next = StrandGetNext(curr);
			FREE(pool, curr);
#if NET_STRAND_DEBUG
			++count;
#endif
		}
#if NET_STRAND_DEBUG
		ASSERT(length == count);
#else
		REF(length);	// CHB 2007.07.30
#endif
	}

	inline void sFreeStrandList(
		MEMORYPOOL * pool)
	{
		CStrandPoolNode * curr = NULL;
		CStrandPoolNode * next = m_pHead;
		while ((curr = next) != NULL)
		{
			next = curr->m_pNext;
			sFreeStrand(pool, curr->m_pStrand, curr->m_nLength);
			FREE(pool, curr);
		}
		m_pHead = NULL;
	}

	inline void sFreeGarbageList(
		MEMORYPOOL * pool)
	{
		CStrandPoolNode * curr = NULL;
		CStrandPoolNode * next = m_pGarbage;
		while ((curr = next) != NULL)
		{
			next = curr->m_pNext;
			FREE(pool, curr);
		}
		m_pGarbage = NULL;
	}

	inline void sRecycleStrandPoolNode(
		CStrandPoolNode * node)
	{
		node->m_pStrand = NULL;
		node->m_pNext = m_pGarbage;
		m_pGarbage = node;
	}

	inline CStrandPoolNode * sGetStrandPoolNode(
		MEMORYPOOL * pool)
	{
		if (!m_pGarbage)
		{
			CStrandPoolNode * node = (CStrandPoolNode *)MALLOC(pool, sizeof(CStrandPoolNode));
			return node;
		}
		CStrandPoolNode * node = m_pGarbage;
		m_pGarbage = node->m_pNext;
		return node;
	}

public:
	void Init(
		void)
	{
		m_csStrandPool.Init();
		m_pHead = NULL;
		m_pGarbage = NULL;
        m_pTotalLength = 0;
	}

	void Free(
		MEMORYPOOL * pool)
	{
		m_csStrandPool.Free();
		sFreeStrandList(pool);
		sFreeGarbageList(pool);
	}

	inline T * GetStrand(
		unsigned int & length)
	{
		CSLAutoLock autolock(&m_csStrandPool);
		if (!m_pHead)
		{
			return NULL;
		}
		CStrandPoolNode * curr = m_pHead;
		m_pHead = m_pHead->m_pNext;
		T * strand = curr->m_pStrand;
		length = curr->m_nLength;
        m_pTotalLength -= length;
		sRecycleStrandPoolNode(curr);
		return strand;
	}

	inline void AddStrand(
		MEMORYPOOL * pool,
		T * strand,
		unsigned int length)
	{
		ASSERT_RETURN(strand);

		CSLAutoLock autolock(&m_csStrandPool);
        if(m_pTotalLength + length > MAX_GLOBAL_TOTAL_STRAND_SIZE)
        {
 #if ISVERSION(SERVER_VERSION)
            TraceWarn(TRACE_FLAG_COMMON_NET,"LogProcessor:NetMemPoolImbalance freeing strand to memory manager instead of caching");
#endif
            sFreeStrand(pool,strand,length); 
        }
        else 
        {
		CStrandPoolNode * node = sGetStrandPoolNode(pool);
		node->m_pStrand = strand;
		node->m_nLength = length;
		node->m_pNext = m_pHead;
		m_pHead = node;

            m_pTotalLength += length;
        }
	}
};


// ---------------------------------------------------------------------------
// DESC:	reuse pool  (msgbufs and overlaps)
//			(maintains 2 strands, not thread safe)  when both strands are full
//			recycles one of the strands back into the global pool
//			if both strands are empty, tries to get a strand from the global
//			pool before doing allocation
// ---------------------------------------------------------------------------
template <typename T, unsigned int TSIZE>
struct CReusePool
{
private:
	static const unsigned int NUM_STRANDS = 2;
	static const unsigned int MAX_STRAND_SIZE = DEFAULT_MAX_STRAND_SIZE;

	T *									m_pStrand[NUM_STRANDS];
	unsigned int						m_nLength[NUM_STRANDS];
	NetPerThreadBufferPool *			m_pParentPTBP;
	CStrandPool<T> *					m_GlobalPool;

	inline void sFreeStrand(
		MEMORYPOOL * pool,
		T * strand)
	{
		T * curr = NULL;
		T * next = strand;
		while ((curr = next) != NULL)
		{
			next = StrandGetNext(curr);
			FREE(pool, curr);

		}
	}

	template <unsigned int STRIDX>
	inline void sAddToStrand(
		T * item)
	{
		memclear(item, sizeof(T));
		StrandGetNext(item) = m_pStrand[STRIDX];
		m_pStrand[STRIDX] = item;
		++m_nLength[STRIDX];
	}

	template <unsigned int STRIDX>
	inline T * sGetFromStrand(
		void)
	{
		T * item = m_pStrand[STRIDX];
		if (item)
		{
			m_pStrand[STRIDX] = StrandGetNext(item);
			--m_nLength[STRIDX];
			memclear(item, sizeof(T));

		}
		return item;
	}

	inline T * sGetStrandFromGlobal(
		unsigned int & length)
	{
		T * strand = m_GlobalPool->GetStrand(length);
#if NET_STRAND_DEBUG
		ASSERT(strand == NULL || length == MAX_STRAND_SIZE);
#endif
		return strand;
	}

	inline void sAddStrandToGlobal(
		MEMORYPOOL * pool)
	{
		T * strand = m_pStrand[1];
		m_GlobalPool->AddStrand(pool, strand, m_nLength[1]);
		m_pStrand[1] = NULL;
		m_nLength[1] = 0;
	}

public:
	void Init(
		NetPerThreadBufferPool * pPTBP,
		CStrandPool<T> * pGlobalPool)
	{
		ASSERT_RETURN(pPTBP);
		ASSERT_RETURN(pGlobalPool);

		m_pParentPTBP = pPTBP;
		m_GlobalPool = pGlobalPool;
		for (unsigned int ii = 0; ii < NUM_STRANDS; ++ii)
		{
			m_pStrand[ii] = NULL;
			m_nLength[ii] = 0;
		}
	}

	void Free(
		MEMORYPOOL * pool)
	{
		for (unsigned int ii = 0; ii < NUM_STRANDS; ++ii)
		{
			sFreeStrand(pool, m_pStrand[ii]);
			m_pStrand[ii] = NULL;
			m_nLength[ii] = 0;
		}
		m_pParentPTBP = NULL;
	}

	inline T * Get(
		MEMORYPOOL * pool)
	{
		T * item = NULL;
		if ((item = sGetFromStrand<0>()) != NULL)
		{
			return item;
		}
		if ((item = sGetFromStrand<1>()) != NULL)
		{
			return item;
		}

		// populate & use strand 0
		m_pStrand[0] = sGetStrandFromGlobal(m_nLength[0]);
		if ((item = sGetFromStrand<0>()) != NULL)
		{
			return item;
		}


		item = (T *)MALLOC(pool, TSIZE);
		return item;
	}

	void Recycle(
		MEMORYPOOL * pool,
		T * item)
	{

		if (m_nLength[0] < MAX_STRAND_SIZE)
		{
			sAddToStrand<0>(item);
			return;
		}
		if (m_nLength[1] < MAX_STRAND_SIZE)
		{
			sAddToStrand<1>(item);
			if (m_nLength[1] >= MAX_STRAND_SIZE)
			{
				sAddStrandToGlobal(pool);
			}
			return;
		}
	}
};

#define MESSAGE_BUFFER_ALLOC_SIZE	LARGE_MSG_BUF_SIZE
#define MAX_MESSAGE_SIZE			MAX_LARGE_MSG_SIZE

void test() STATIC_CHECK(MESSAGE_BUFFER_ALLOC_SIZE <= MAX_READ_BUFFER, TooLargeASizeToBePooled)

typedef CReusePool<OVERLAPPEDEX, sizeof(OVERLAPPEDEX)>				COverlapPool;
typedef CReusePool<MSG_BUF, MESSAGE_BUFFER_ALLOC_SIZE>				CMessagePool;

//When we implement large messages, we need to revisit the pooling and
//avoid massive amounts of wasted memory for small message types.

// ---------------------------------------------------------------------------
// any thread calling into net (including net worker threads) will get one of
// these.
// ---------------------------------------------------------------------------
struct NetPerThreadBufferPool 
{
public:
	NETCOM *							m_Net;
	unsigned int						m_idx;									// index in NETCOM's vector
	COverlapPool           				m_OverlapPool;							// pool of overlaps
	CMessagePool		            	m_MessagePool;							// pool of msgbufs
	PERF_INSTANCE(CommonNetBufferPool) *m_PerfInstance;

private:
	void sCreatePerfInstance(
		void)
	{
#if ISVERSION(SERVER_VERSION)
	    WCHAR myname[MAX_PERF_INSTANCE_NAME + 1];
		PStrPrintf(myname, arrsize(myname), L"%p-%d", this, m_idx);
		m_PerfInstance = PERF_GET_INSTANCE(CommonNetBufferPool, myname);
		ASSERT(m_PerfInstance);
#else
		m_PerfInstance = NULL;
#endif
	}

public:
	// -----------------------------------------------------------------------
    void Init(
		int idx,
		NETCOM * net,
		CStrandPool<OVERLAPPEDEX> * pGlobalOverlapPool,
		CStrandPool<MSG_BUF> * pGlobalMessagePool)
    {
        m_Net = net;
        m_idx = idx;
        m_OverlapPool.Init(this, pGlobalOverlapPool);
        m_MessagePool.Init(this, pGlobalMessagePool);

		sCreatePerfInstance();
	}

	void Free(
		void)
	{
		ASSERT_RETURN(m_Net);
		m_OverlapPool.Free(NetGetMemPool(m_Net));
		m_MessagePool.Free(NetGetMemPool(m_Net));

		if (m_PerfInstance)
		{
			PERF_FREE_INSTANCE(m_PerfInstance);
		}

		m_idx = (unsigned int)-1;
		m_Net = NULL;
	}
};


// ---------------------------------------------------------------------------
// collection of per thread buffer pools which share global pools
// ---------------------------------------------------------------------------
struct NPTBPCollection
{
private:
	CCriticalSection					m_CS;
	SIMPLE_DYNAMIC_ARRAY<
		NetPerThreadBufferPool *>		m_Array;
    DWORD                           	m_TLSIndex;
	CStrandPool<OVERLAPPEDEX> 			m_GlobalOverlapPool;					// global pool of overlaps shared by m_PTBPCollection
	CStrandPool<MSG_BUF> 				m_GlobalMessagePool;					// global pool of message buffers shared by m_PTBPCollection
	
public:
	BOOL Init(
		MEMORYPOOL * pool)
	{
		m_CS.Init();
		ArrayInit(m_Array, pool, 1);
		if ((m_TLSIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES) 
		{
			TraceError("Could not get Tls index: %d\n", GetLastError());
			return FALSE;
		}
		m_GlobalOverlapPool.Init();
		m_GlobalMessagePool.Init();
		return TRUE;
	}

	void Free(
		MEMORYPOOL * pool)
	{
		{
			CSAutoLock autolock(&m_CS);
			for (unsigned int ii = 0; ii < m_Array.Count(); ++ii)
			{
				NetPerThreadBufferPool * ptbp = m_Array[ii];
				ASSERT_CONTINUE(ptbp);
				ptbp->Free();
				FREE(pool, ptbp);
			}
			m_Array.Destroy();
			m_GlobalMessagePool.Free(pool);
			m_GlobalOverlapPool.Free(pool);
			m_TLSIndex = (DWORD)-1;
		}
		m_CS.Free();
	}

	NetPerThreadBufferPool * Add(
		NETCOM * net);

	inline NetPerThreadBufferPool * Get(
		NETCOM * net)
	{
	    NetPerThreadBufferPool * ptbp = (NetPerThreadBufferPool *)TlsGetValue(m_TLSIndex);
		if (!ptbp)
		{
			CSAutoLock autolock(&m_CS);
			ptbp = Add(net);
			ASSERT(ptbp);
		}
		return ptbp;
	}
};


// ---------------------------------------------------------------------------
// per-worker data held by netcom
// ---------------------------------------------------------------------------
struct NET_WORKER_DATA
{
	NETCOM *							m_Net;
	unsigned int						m_Idx;
	HANDLE								m_hHandle;
	HANDLE								m_hWorkerCompletionPort;
	long                            	m_PoolInitialized;
};


// ---------------------------------------------------------------------------
// DESC:	main structure for network
// LOCK:	generally, it's assumed throughout this module that NETCOM won't
//			get freed except by the main thread and so it's safe to use it
//			everywhere, as long as everything is considered volatile, or
//			locks individually, or is only changed from the main thread
// ---------------------------------------------------------------------------
struct NETCOM
{
	// this section is only written to on init/free 
	// and is assumed non-volatile
	MEMORYPOOL *						mempool;								// memory pool

	char								m_szName[NET_NAME_SIZE];				// debug name

	int									m_nThreadPoolMin;						// min # of worker threads
	int									m_nThreadPoolMax;						// max # of worker threads

	WSOCK_EXTENDED						m_WsockFn[2];							// winsock extended functions, one for ipv4 & one for ipv6

	HANDLE *							m_hWorkerCompletionPorts;				// io completion port to actually start IO
    DWORD                               m_nNumberOfWorkerCompletionPorts;
	HANDLE								m_hFakeConnectExCompletionPort;			// Fake port for Connectex/DisconnectEx

	int									m_nUserDataSize;						// size of user data block per NETCLT

	NPTBPCollection						m_PTBPCollection;						// per thread buffer pools
	DWORD								m_TLSIndex;

	NET_WORKER_DATA						m_WorkerData[MAX_WORKER_THREADS];		// worker thread data

	// this section is volatile
	volatile LONG						m_eState;								// state of net

	volatile long						m_nThreadPoolCount;						// # of attempted worker threads
	volatile LONG						m_nCurrentThreads;						// cur # of worker threads
	volatile LONG						m_nBusyThreads;							// active # of worker threads

	CClientTable						m_ClientTable;							// clients connections

	NETSRV								m_Servers[MAX_SERVERS];					// servers
	volatile LONG						m_nActiveServers;						// # of active servers
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static inline MEMORYPOOL * NetGetMemPool(
	NETCOM * net)
{
	return net->mempool;
}


// ---------------------------------------------------------------------------
// message field types
// ---------------------------------------------------------------------------
enum NET_FIELD_TYPE
{
	NET_FIELD_BYTE,
	NET_FIELD_CHAR,
	NET_FIELD_WORD,
	NET_FIELD_SHORT,
	NET_FIELD_DWORD,
	NET_FIELD_ULONGLONG,
	NET_FIELD_INT,
	NET_FIELD_FLOAT,
	NET_FIELD_UNITID,
	NET_FIELD_ROOMID,
	NET_FIELD_WCHAR,
	NET_FIELD_BLOBB,
	NET_FIELD_BLOBW,
	NET_FIELD_TASKID,
	NET_FIELD_STRUCT,
	NET_FIELD_PGUID,
	NET_FIELD_GAMEID,
	NET_FIELD_CHANNELID,
	NET_FIELD_GAMECLIENTID,
	NET_FIELD_INT64,

	LAST_NET_FIELD
};


struct NET_FIELD_TYPE_DESC
{
	NET_FIELD_TYPE						m_eType;
	const char *						m_szStr;
	unsigned int						m_nSize;
};


const NET_FIELD_TYPE_DESC g_NetFieldTypes[] =
{
	{	NET_FIELD_BYTE,			"BYTE",			sizeof(BYTE)		},
	{	NET_FIELD_CHAR,			"char",			sizeof(char)		},
	{	NET_FIELD_WORD,			"WORD",			sizeof(WORD)		},
	{	NET_FIELD_SHORT,		"short",		sizeof(short)		},
	{	NET_FIELD_DWORD,		"DWORD",		sizeof(DWORD)		},
	{	NET_FIELD_ULONGLONG,	"ULONGLONG",	sizeof(ULONGLONG)	},
	{	NET_FIELD_INT,			"int",			sizeof(int)			},
	{	NET_FIELD_FLOAT,		"float",		sizeof(float)		},
	{	NET_FIELD_UNITID,		"UNITID",		sizeof(UNITID)		},
	{	NET_FIELD_ROOMID,		"ROOMID",		sizeof(ROOMID)		},
	{	NET_FIELD_WCHAR,		"WCHAR",		sizeof(WCHAR)		},
	{	NET_FIELD_BLOBB,		"BLOBB",		sizeof(BYTE)		},
	{	NET_FIELD_BLOBW,		"BLOBW",		sizeof(BYTE)		},
	{   NET_FIELD_TASKID,		"GAMETASKID",		sizeof(DWORD)		},
	{	NET_FIELD_STRUCT,		NULL,			0					},
	{	NET_FIELD_PGUID,		"PGUID",		sizeof(PGUID)		},
	{	NET_FIELD_GAMEID,		"GAMEID",		sizeof(GAMEID)		},
	{	NET_FIELD_CHANNELID,	"CHANNELID",	sizeof(CHANNELID)	},
	{	NET_FIELD_GAMECLIENTID,	"GAMECLIENTID",	sizeof(GAMECLIENTID)},
	{	NET_FIELD_INT64,		"INT64",		sizeof(__int64)		},
};


// ---------------------------------------------------------------------------
// message field declaration
// ---------------------------------------------------------------------------
struct MSG_FIELD
{
#if !ISVERSION(RETAIL_VERSION)
	const char *						m_szName;
#endif
	NET_FIELD_TYPE						m_eType;
	unsigned int						m_nOffset;								// offset into unpacked structure
	unsigned int						m_nCount;
	unsigned int						m_nSize;
	struct MSG_STRUCT_DECL *			m_pStruct;
};


// ---------------------------------------------------------------------------
// message declaration
// ---------------------------------------------------------------------------
struct MSG_STRUCT_DECL
{
	const char **						m_szName;
	MSG_FIELD *							m_pFields;
	unsigned int						m_nFieldCount;
	unsigned int						m_nSize;								// packed size 

	BOOL								m_bInitialized;
	BOOL								m_bVarSize;								// variable sized
};


// ---------------------------------------------------------------------------
// all message declarations
// ---------------------------------------------------------------------------
struct MSG_STRUCT_TABLE
{
	unsigned int						m_nFieldCount;
	unsigned int						m_nStructCount;
	MSG_STRUCT_DECL *					m_pDefCur[MAX_MSG_STRUCT_STACK];		// currently being defined struct
	unsigned int						m_nDefCurTop;

	MSG_FIELD							m_Fields[MAX_MSG_FIELDS_TOTAL];
	MSG_STRUCT_DECL						m_Structs[MAX_MSG_STRUCTS];
};


// ---------------------------------------------------------------------------
// MACROS
// ---------------------------------------------------------------------------
#define CALL_CMD_PRINT(fp, m, s)	(((static_cast<BASE_MSG_STRUCT*>(m))->*(fp)))(s)
#define CALL_CMD_PUSH(fp, m, b)		(((static_cast<BASE_MSG_STRUCT*>(m))->*(fp)))(b)
#define CALL_CMD_POP(fp, m, b)		(((static_cast<BASE_MSG_STRUCT*>(m))->*(fp)))(b)


void (*m_pfImmCallback)(NETCOM *, NETCLIENTID, BYTE * data, unsigned int size);	// immediate callback function


// ---------------------------------------------------------------------------
// command definition
//
// IF YOU CHANGE THIS structure, YOU MUST CHANGE the copy in UserServer.h 
// as well !!!!!!!!!!
// ---------------------------------------------------------------------------
struct NET_COMMAND
{
	unsigned int						cmd;									// command
	unsigned int						maxsize;								// max size
	BOOL								varsize;								// variable sized message?
	MSG_STRUCT_DECL *					msg;									// struct decl
	BOOL								bLogOnSend;
	BOOL								bLogOnRecv;
	MFP_PRINT							mfp_Print;
	MFP_PUSHPOP							mfp_Push;
	MFP_PUSHPOP							mfp_Pop;
	FP_NET_IMM_CALLBACK					mfp_ImmCallback;
};


// ---------------------------------------------------------------------------
// track message counts
// ---------------------------------------------------------------------------
#if NET_MSG_COUNT
struct NET_MSG_TRACKER_NODE
{
#ifndef _WIN64
	CCritSectLite						m_CS;
#endif

	volatile LONG						m_lSendCount;
	volatile LONG						m_lRecvCount;
	volatile LONGLONG					m_llSendBytes;
	volatile LONGLONG					m_llRecvBytes;

	void Zero(
		void)
	{
		m_lSendCount = 0;
		m_llSendBytes = 0;
		m_lRecvCount = 0;
		m_llRecvBytes = 0;
	}

	void Clear(
		void)
	{
		m_lSendCount = 0;
		m_lRecvCount = 0;
#ifndef _WIN64
		m_CS.Enter();
#endif
		m_llSendBytes = 0;
		m_llRecvBytes = 0;
#ifndef _WIN64
		m_CS.Leave();
#endif
	}

	void Init(
		void)
	{
#ifndef _WIN64
		m_CS.Init();
#endif
		Zero();
	}

	void Free(
		void)
	{
#ifndef _WIN64
		m_CS.Free();
#endif
		Zero();
	}

	void IncrementSendCount(
		void)
	{
		InterlockedIncrement(&m_lSendCount);
	}

	void IncrementRecvCount(
		void)
	{
		InterlockedIncrement(&m_lRecvCount);
	}

	void AddSendBytes(
		LONGLONG bytes)
	{
#ifdef _WIN64
		InterlockedAdd64(&m_llSendBytes, bytes);
#else
		m_CS.Enter();
		m_llSendBytes += bytes;
		m_CS.Leave();
#endif
	}

	void AddRecvBytes(
		LONGLONG bytes)
	{
#ifdef _WIN64
		InterlockedAdd64(&m_llRecvBytes, bytes);
#else
		m_CS.Enter();
		m_llRecvBytes += bytes;
		m_CS.Leave();
#endif
	}

	LONG GetSendCount(
		void)
	{
		return m_lSendCount;
	}

	LONG GetRecvCount(
		void)
	{
		return m_lRecvCount;
	}

	LONGLONG GetSendBytes(
		void)
	{
#ifdef _WIN64
		return m_llSendBytes;
#else
		LONGLONG bytes;
		m_CS.Enter();
		bytes = m_llSendBytes;
		m_CS.Leave();
		return bytes;
#endif
	}

	LONGLONG GetRecvBytes(
		void)
	{
#ifdef _WIN64
		return m_llRecvBytes;
#else
		LONGLONG bytes;
		m_CS.Enter();
		bytes = m_llRecvBytes;
		m_CS.Leave();
		return bytes;
#endif
	}
};
#endif


// ---------------------------------------------------------------------------
// track message counts
// ---------------------------------------------------------------------------
#if NET_MSG_COUNT
struct NET_MSG_TRACKER
{
	NET_MSG_TRACKER_NODE *				m_Data;
	unsigned int						m_nCmdCount;
	TIME								m_StartTime;

	void Init(
		unsigned int count)
	{
		m_nCmdCount = count;
		m_Data = (NET_MSG_TRACKER_NODE *)MALLOCZ(NULL, sizeof(NET_MSG_TRACKER_NODE) * m_nCmdCount);
		m_StartTime = 0;
	}

	void Free(
		void)
	{
		if (m_Data)
		{
			FREE(NULL, m_Data);
			m_Data = NULL;
		}
		m_StartTime = 0;
	}

	NET_MSG_TRACKER_NODE & operator[](
		unsigned int index)
	{
		return m_Data[index];
	}

	NET_MSG_TRACKER_NODE & operator[](
		int index)
	{
		return m_Data[index];
	}
};
#endif


// ---------------------------------------------------------------------------
// command table
// ---------------------------------------------------------------------------
struct NET_COMMAND_TABLE
{
	MSG_STRUCT_TABLE					m_MsgStruct;
	NET_COMMAND	*						m_Cmd;
	unsigned int						m_nCmdCount;
	CRefCount							m_RefCount;

#if NET_MSG_COUNT
	NET_MSG_TRACKER						m_Tracker;
#endif
};


// ---------------------------------------------------------------------------
// FORWARD DECLARATION (FUNCTIONS)
// ---------------------------------------------------------------------------
static HANDLE NetSpawnWorkerThread(
	NETCOM * net);

static void NetRecycleSocket(
	NETCOM * net,
	CSocketPool & pool,
	SOCKET & sock);

static void NetRecycleMsgBuf(
	NETCOM * net,
	MSG_BUF * & msg);

static BOOL NetSendParallel(
	NETCOM * net,
	NETCLT * clt,
	const void * data,
	unsigned int size);

static BOOL NetSendSerial(
	NETCOM * net,
	NETCLT * clt,
	const void * data,
	unsigned int size);

static BOOL NetSendParallelNagle(
	NETCOM * net,
	NETCLT * clt,
	const void * data,
	unsigned int size);

static BOOL NetSendLocal(
	NETCOM * net,
	NETCLT * clt,
	const void * data,
	unsigned int size);

// ---------------------------------------------------------------------------
// STATIC FUNCTIONS
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// DESC:	trace error message given error code
// ---------------------------------------------------------------------------
static inline void NetErrorMessage(
	int errorcode,
	BOOL bAssert = TRUE)
{
#if !ISVERSION(SERVER_VERSION)
	enum
	{
		MAX_ERRMSG_LEN = 1024,
	};
	char msg[MAX_ERRMSG_LEN];

    int len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
                  FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, errorcode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPSTR)msg, MAX_ERRMSG_LEN, NULL);
	REF(len);

	LogMessage(NETWORK_LOG,msg);
#else
	TraceError("Net error : %!WINERROR!\n",errorcode);
#endif //!SERVER_VERSION

	if (bAssert)
	{
		ASSERT(0);
	}
}


// ---------------------------------------------------------------------------
// DESC:	singleton, initializes windows sockets
// ---------------------------------------------------------------------------
class CNetCommon : public CSingleton<CNetCommon>
{
public:
	BOOL							m_bInitialized;

public:
	CNetCommon(
		void) : m_bInitialized(NULL)
	{
		WORD wVersionRequested = MAKEWORD(2, 2);
		WSADATA wsaData; 
		int err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0)
		{
			NetErrorMessage(err);
			return;						// couldn't find a usable winsock dll
		}
		if (LOBYTE(wsaData.wVersion) != 2 ||
			HIBYTE(wsaData.wVersion) != 2)
		{
			WSACleanup();
			return;
		}

		m_bInitialized = TRUE;
	}

	~CNetCommon(
		void)
	{
		WSACleanup();
	}
};


// ---------------------------------------------------------------------------
// DESC:	initialize winsock
// ---------------------------------------------------------------------------
static BOOL NetWinsockInit(
	void)
{
	CNetCommon * ptr = CNetCommon::GetInstance();

	return ptr->m_bInitialized;
}


// ---------------------------------------------------------------------------
// DESC:	clean up winsock
// ---------------------------------------------------------------------------
static void NetWinsockFree(
	void)
{
	return;
}


// ---------------------------------------------------------------------------
// DESC:	get address as string
// ---------------------------------------------------------------------------
BOOL NetGetAddrString(
	sockaddr_storage & addr,
	char * str,
	int len)
{
	if (getnameinfo((LPSOCKADDR)&addr, sizeof(addr), str, len, NULL, 0, NI_NUMERICHOST) != 0)
	{
        PStrCopy(str, "<unknown>", len);
		return FALSE;
    }
	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	get sock_addr for a given string addr
// ---------------------------------------------------------------------------
static BOOL NetGetAddr(
	const char * address,
	unsigned int addrfamily,
	unsigned short port,
	DWORD flags,
	sockaddr_storage * addr,
	unsigned int * addrlen,
	unsigned int * family,
	unsigned int * socktype,
	unsigned int * protocol)
{
	// get the addrinfo for my ipaddr
	ADDRINFO Hints, * AddrInfo;
	memclear(&Hints, sizeof(Hints));
	Hints.ai_family = addrfamily;
	Hints.ai_socktype = DEFAULT_SOCKTYPE;
	Hints.ai_flags = flags;

	char szPort[12];
	PIntToStr(szPort, 12, port);

	int err = getaddrinfo(address, szPort, &Hints, &AddrInfo);
	if (err != 0)
	{
		NetErrorMessage(err);
		return FALSE;
	}

	BOOL result = FALSE;

    for (ADDRINFO * AI = AddrInfo; AI != NULL; AI = AI->ai_next) 
	{
        // The correct way to use IPV6 is to try connecting over it first. For now, ignore IPV6 
        //
		if ((Hints.ai_family == PF_UNSPEC && AI->ai_family != PF_INET /*&& AI->ai_family != PF_INET6*/) ||
			Hints.ai_family != DEFAULT_FAMILY)
		{
			continue;
		}

		memcpy(addr, AI->ai_addr, AI->ai_addrlen);
		*addrlen = (unsigned int)AI->ai_addrlen;

		if (family)
		{
			*family = AI->ai_family;
		}
		if (socktype)
		{
			*socktype = AI->ai_socktype;
		}
		if (protocol)
		{
			*protocol = AI->ai_protocol;
		}

		result = TRUE;
		break;
    }
	freeaddrinfo(AddrInfo);

	return result;
}


// ---------------------------------------------------------------------------
// DESC:	get (string) local address of a socket
// ---------------------------------------------------------------------------
static unsigned short NetGetPortFromAddr(
	const sockaddr_storage & addr)
{
	return ntohs(SS_PORT(&addr));
}


// ---------------------------------------------------------------------------
// DESC:	get (string) local address of a socket
// ---------------------------------------------------------------------------
static BOOL NetGetSocketLocalAddr(
	SOCKET sock,
	char * str,
	unsigned int size,
	unsigned short * port)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(size > 9);
	if (sock == INVALID_SOCKET)
	{
		return FALSE;
	}

	struct sockaddr_storage addr;		// generic sockaddr for IPv4 or IPv6
	int addrlen = (int)sizeof(addr);
	if (getsockname(sock, (LPSOCKADDR)&addr, &addrlen) == SOCKET_ERROR) 
	{
		int error = WSAGetLastError();
		switch (error)
		{
		case WSANOTINITIALISED:			// need to call WSAStartup()
		case WSAENETDOWN:				// network susbystem is down
		case WSAEFAULT:					// the name or namelen parameter were invalid
		case WSAEINPROGRESS:			// a blocking winsock call is in progress
		case WSAENOTSOCK:				// the given socket isn't a socket
		case WSAEINVAL:					// the socket hasn't been bound to an address
		default:
			NetErrorMessage(error);
			break;
		}
        PStrCopy(str, "<unknown>", size);
		return FALSE;
    } 
	
	if (!NetGetAddrString(addr, str, size))		// translate name to string
	{
		return FALSE;
	}

	if (port)							// get port if requested
	{
		*port = NetGetPortFromAddr(addr);
	}
	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	get (string) remote address of a socket
// ---------------------------------------------------------------------------
static BOOL NetGetSocketRemoteAddr(
	SOCKET sock,
	char * str,
	unsigned int size,
	unsigned short * port)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(size > 9);
	if (sock == INVALID_SOCKET)
	{
		return FALSE;
	}

	struct sockaddr_storage addr;		// generic sockaddr for IPv4 or IPv6
	int addrlen = (int)sizeof(addr);
	if (getpeername(sock, (LPSOCKADDR)&addr, &addrlen) == SOCKET_ERROR) 
	{
		int error = WSAGetLastError();
		switch (error)
		{
		case WSAEFAULT:					// the name or namelen parameter were invalid
		case WSAEINPROGRESS:			// a blocking winsock call is in progress
		case WSAENOTSOCK:				// the given socket isn't a socket
		case WSAENETDOWN:				// network susbystem is down
		case WSANOTINITIALISED:			// need to call WSAStartup()
		case WSAENOTCONN:				// not connected
		default:
			NetErrorMessage(error);
			break;
		}
        PStrCopy(str, "<unknown>", size);
		return FALSE;
    } 
	
	// translate name to string
	if (!NetGetAddrString(addr, str, size))
	{
		return FALSE;
	}

	// get port if requested
	if (port)
	{
		*port = NetGetPortFromAddr(addr);
	}
	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	wrapper for setsockopt
// ---------------------------------------------------------------------------
static BOOL NetSetSockopt(
	SOCKET sock,
	int level,
	int optname,
	const char * optval,
	int optlen,
	BOOL bAssert = FALSE)
{
	if (setsockopt(sock, level, optname, optval, optlen) == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		switch (error)
		{
		case WSAENETDOWN:				// network susbystem is down
		case WSAEFAULT:					// the name or namelen parameter were invalid
		case WSAENOTSOCK:				// the given socket isn't a socket
		case WSAEINVAL:					// level or optval isn't valid
		case WSAENETRESET:				// connection reset
		case WSAENOPROTOOPT:			// the option is unknown for the provider or socket
		case WSAENOTCONN:				// connection timed out
		case WSANOTINITIALISED:			// need to call WSAStartup()
		default:
			NetErrorMessage(error, bAssert);
			return FALSE;
		}
	}
	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	wrapper for setsockopt
// ---------------------------------------------------------------------------
static BOOL NetSetSockopt(
	SOCKET sock,
	int level,
	int optname,
	BOOL optval,
	BOOL bAssert = FALSE)
{
	return (NetSetSockopt(sock, level, optname, (const char *)&optval, sizeof(BOOL), bAssert));
}


// ---------------------------------------------------------------------------
// DESC: Make sure we're not nagling.
// ---------------------------------------------------------------------------
#if ENFORCE_NO_NAGLE
static void sEnforceNonNagle(
	SOCKET sock)
{
	char sockoptbuffer[256];
	int nLength = 256;

	ASSERT(getsockopt(sock, IPPROTO_TCP, TCP_NODELAY, sockoptbuffer, &nLength) == 0); 
	ASSERT_DO(sockoptbuffer[0] == 1)
	{
		NetSetSockopt(sock, IPPROTO_TCP, TCP_NODELAY, TRUE);
		nLength = 256;
		ASSERT(getsockopt(sock, IPPROTO_TCP, TCP_NODELAY, sockoptbuffer, &nLength) == 0); 
		ASSERT(sockoptbuffer[0] == 1);
	}
}
#endif


// ---------------------------------------------------------------------------
// DESC:	create socket
//			note WSASocket() takes a long time, should call from a different
//			thread if we call this while the server is running
// LOCK:	call from anywhere
// ---------------------------------------------------------------------------
static SOCKET NetWSASocket(
	unsigned int family,
	unsigned int socktype,
	unsigned int protocol)
{
	SOCKET sock = WSASocket(family, socktype, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		switch (error)
		{
		case WSANOTINITIALISED:			// need to call WSAStartup()
		case WSAENETDOWN:				// network susbystem is down
		case WSAEAFNOSUPPORT:			// the specified address family isn't supported
		case WSAEINPROGRESS:			// a blocking winsock call is in progress
		case WSAEMFILE:					// no more socket descriptors are available
		case WSAENOBUFS:				// no buffer space is available
		case WSAEPROTONOSUPPORT:		// the specified protocol isn't supported
		case WSAEPROTOTYPE:				// the specified protocol is the wrong type for this socket
		case WSAESOCKTNOSUPPORT:		// the specified address type isn't supported in this address family
		case WSAEINVAL:					// some bad parameters
		case WSAEFAULT:					// lpProtocol info is invalid
		default:
			NetErrorMessage(error);
			break;
		}
	}
	return sock;
}


// ---------------------------------------------------------------------------
// DESC:	create socket
// LOCK:	call from anywhere
// ---------------------------------------------------------------------------
static BOOL NetBind(
	SOCKET sock,
	const sockaddr * addr,
	int addrlen)
{
	if (bind(sock, addr, addrlen) == SOCKET_ERROR) 
	{
		int error = WSAGetLastError();
		switch (error)
		{
		case WSAEACCES:					// attempt to connect datagram socket to broadcast address failed because SO_BROADCAST isn't enabled
		case WSAEFAULT:					// the name or namelen parameter were invalid
		case WSAEINPROGRESS:			// a blocking winsock call is in progress
		case WSAENOTSOCK:				// not a valid socket
		case WSAEADDRINUSE:				// the socket's local address is already in use
		case WSAEADDRNOTAVAIL:			// the remote address isn't valid
		case WSAENETDOWN:				// network susbystem is down
		case WSAENOBUFS:				// no buffer space is available
		case WSANOTINITIALISED:			// need to call WSAStartup()
		default:
			NetErrorMessage(error);
			break;
		case WSAEINVAL:					// a parameter was invalid or the socket is in an invalid state
			NetErrorMessage(error, FALSE);		// couldn't bind, okay whatever
			break;
		}
		return FALSE;
    }
	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	set socket to listen
// LOCK:	call from anywhere
// ---------------------------------------------------------------------------
static BOOL NetListen(
	SOCKET sock,
	unsigned int backlog = DEFAULT_BACKLOG)
{
	if (listen(sock, backlog) == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		switch (error)
		{
		case WSANOTINITIALISED:			// need to call WSAStartup()
		case WSAENETDOWN:				// network susbystem is down
		case WSAEADDRINUSE:				// the socket's local address is already in use
		case WSAEINVAL:					// a parameter was invalid or the socket is in an invalid state
		case WSAEINPROGRESS:			// a blocking winsock call is in progress
		case WSAEISCONN:				// the socket is already connected
		case WSAEMFILE:					// there are no more socket descriptors available
		case WSAENOBUFS:				// no buffer space is available
		case WSAENOTSOCK:				// not a valid socket
		case WSAEOPNOTSUPP:				// the socket doesn't support the listen operation
		default:
			NetErrorMessage(error);
			break;
		}
		return FALSE;
	}
	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	wrapper for closesocket
// ---------------------------------------------------------------------------
void NetCloseSocket(
	SOCKET & socket)
{
	if (closesocket(socket) == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		switch (error)
		{
		case WSAEWOULDBLOCK:		// the socket is marked as nonblocking and SO_LINGER is set to a nonzero time-out value
		case WSAEINTR:				// the (blocking) windows socket 1.1 call was canceled through WSACancelBlockingCall
		case WSAEINPROGRESS:		// a blocking windows sockets 1.1 call is in progress, or the service provider is still processing a callback function.
		case WSAENOTSOCK:			// the descriptor is not a socket
		case WSAENETDOWN:			// the network subsystem has failed
		case WSANOTINITIALISED:		// a successful WSAStartup call must occur before using this function
		default:
			NetErrorMessage(error);
			break;
		}
	}
	socket = INVALID_SOCKET;
}

	
// ---------------------------------------------------------------------------
// DESC:	get a free socket
//			note WSASocket() takes a long time, should call from a different
//			thread if we call this while the server is running
// LOCK:	call from anywhere
// ---------------------------------------------------------------------------
static SOCKET NetGetNewSocket(
	NETCOM * net,
    DWORD nCPIndex,
	unsigned int family,
	unsigned int socktype,
	unsigned int protocol)
{
    ASSERT_RETVAL(nCPIndex < net->m_nNumberOfWorkerCompletionPorts,INVALID_SOCKET);
	SOCKET sock = NetWSASocket(family, socktype, protocol);

	// associate the socket with the completion port
	if (net->m_hWorkerCompletionPorts[nCPIndex] != CreateIoCompletionPort((HANDLE)sock, 
                                                    net->m_hWorkerCompletionPorts[nCPIndex],
                                                    (ULONG_PTR)0, 0))
	{
		ASSERT(0);
		NetCloseSocket(sock);
		return INVALID_SOCKET;
	}
	return sock;
}


// ---------------------------------------------------------------------------
// DESC:	get a free client socket from the socket pool
// LOCK:	safe from anywhere
// ---------------------------------------------------------------------------
static SOCKET NetGetSocket(
	NETCOM * net,
    DWORD nCPIndex,
	CSocketPool & pool)
{
	return NetGetNewSocket(net, nCPIndex,pool.m_nFamily, pool.m_nSocktype, pool.m_nProtocol);

}


// ---------------------------------------------------------------------------
// DESC:	recycle a used socket
// LOCK:	call from anywhere
// ---------------------------------------------------------------------------
static void NetRecycleSocket(
	NETCOM * net,
	CSocketPool & pool,
	SOCKET & sock)
{
	UNREFERENCED_PARAMETER(pool);
	UNREFERENCED_PARAMETER(net);
	NetCloseSocket( sock );

}


// ---------------------------------------------------------------------------
// create a NetPerThreadBufferPool for calling thread. 
// ---------------------------------------------------------------------------
NetPerThreadBufferPool * NPTBPCollection::Add(
	NETCOM * net)
{
	ASSERT_RETNULL(net);
    NetPerThreadBufferPool * ptbp = (NetPerThreadBufferPool *)MALLOCZ(net->mempool, sizeof(NetPerThreadBufferPool));
	ASSERT_RETNULL(ptbp);

	ptbp->Init(0, net, &m_GlobalOverlapPool, &m_GlobalMessagePool);

	CSAutoLock(m_CS);
	ptbp->m_idx = m_Array.Count();
	ArrayAddItem(m_Array, ptbp);
    ASSERT_GOTO(TlsSetValue(m_TLSIndex, ptbp), _error);
	return ptbp;

_error:
	if (ptbp)
	{
		ptbp->Free();
		FREE(net->mempool, ptbp);
	}
	return NULL;
}


// ---------------------------------------------------------------------------
// Create a NetPerThreadBufferPool for calling thread. 
// ---------------------------------------------------------------------------
BOOL NetCreatePrivatePerThreadBufferPool(
	NETCOM * net)
{
    ASSERT_RETNULL(net);
	return (net->m_PTBPCollection.Add(net) != NULL);
}


// ---------------------------------------------------------------------------
// DESC:	get a NetPerThreadBufferPool from TLS, or a random thread.
// LOCK:	uses the per-thread TLS lock in the pool.
// ---------------------------------------------------------------------------
static NetPerThreadBufferPool * NetGetPerThreadBufferPool(
	NETCOM * net)
{
	ASSERT_RETNULL(net);
	return net->m_PTBPCollection.Get(net);
}


// ---------------------------------------------------------------------------
// DESC:	free the PerThreadBufferPools
// ---------------------------------------------------------------------------
static void NetFreePerThreadBufferPools(
	NETCOM * net)
{
	ASSERT_RETURN(net);

	net->m_PTBPCollection.Free(NetGetMemPool(net));
}


// ---------------------------------------------------------------------------
// DESC:	create some free sockets
// LOCK:	safe anywhere, but only called from main
// ---------------------------------------------------------------------------
BOOL NetPrecreateSockets(
	NETCOM * net,
	long nCPIndex,
	CSocketPool & pool,
	unsigned int count)
{
	for (unsigned int ii = 0; ii < count; ii++)
	{
		SOCKET sock = NetGetNewSocket(net,nCPIndex, pool.m_nFamily, pool.m_nSocktype, pool.m_nProtocol);
		if (sock == INVALID_SOCKET)
		{
			return FALSE;
		}
		CSocketNode * node = (CSocketNode *)MALLOCZ(net->mempool, sizeof(CSocketNode));
		if (!node)
		{
			NetCloseSocket(sock);
			return FALSE;
		}
		InterlockedIncrement(&pool.m_nSocketCount);

		node->m_hSocket = sock;

		// put the node on the socket pool
		pool.m_csSockets.Enter();
		{
			if (!pool.m_pSocketsTail)
			{
				pool.m_pSocketsTail = node;
			}
			node->m_pNext = pool.m_pSocketsHead;
			pool.m_pSocketsHead = node;
		}
		pool.m_csSockets.Leave();
	}

	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	close all sockets in socket pool
// LOCK:	safe anywhere, but only called from main
// TODO:	closesocket() can take an awfully long time!!!
// ---------------------------------------------------------------------------
static void NetCloseSocketPool(
	NETCOM * net,
	CSocketPool & pool)
{
	while (TRUE)
	{
		CSocketNode * node = NULL;

		// get a node from the socket pool
		pool.m_csSockets.Enter();
		{
			node = pool.m_pSocketsHead;
			if (node)
			{
				pool.m_pSocketsHead = node->m_pNext;
			}
		}
		pool.m_csSockets.Leave();

		if (!node)
		{
			break;
		}

		if (node->m_hSocket != INVALID_SOCKET)
		{
			LINGER linger;
			linger.l_onoff = 1;
			linger.l_linger = 0;
			NetSetSockopt(node->m_hSocket, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(linger));

			NetCloseSocket(node->m_hSocket);
		}

		FREE(net->mempool, node);
		InterlockedDecrement(&pool.m_nSocketCount);
	}
	pool.m_pSocketsTail = NULL;
}


// ---------------------------------------------------------------------------
// DESC:	free socket pool
// NOTE:	safe from anywhere, but only called from main
// ---------------------------------------------------------------------------
static void NetFreeSocketPool(
	NETCOM * net, 
	CSocketPool & pool)
{
	NetCloseSocketPool(net, pool);

	// actually NetCloseSocketPool() frees everything, so this shouldn't really happen
	while (TRUE)
	{
		CSocketNode * node = NULL;

		// get a node from the socket pool
		pool.m_csFreeNodes.Enter();
		{
			node = pool.m_pFreeNodes;
			if (node)
			{
				pool.m_pFreeNodes = node->m_pNext;
			}
		}
		pool.m_csFreeNodes.Leave();

		if (!node)
		{
			break;
		}

		FREE(net->mempool, node);
		InterlockedDecrement(&pool.m_nSocketCount);
	}
	ASSERT(pool.m_nSocketCount == 0);
}


// ---------------------------------------------------------------------------
// DESC:	select a WSOCK_EXTENDED object given a socket & family
// ---------------------------------------------------------------------------
static WSOCK_EXTENDED * NetGetWsockEx(
	NETCOM * net,
	SOCKET sock,
	unsigned int family,
	unsigned int socktype,
	unsigned int protocol)
{
	ASSERT_RETNULL(family == PF_INET || family == PF_INET6);
	int index = (family == PF_INET ? 0 : 1);

	BOOL bCloseSocket = FALSE;
	if (sock == INVALID_SOCKET)
	{
		// create a socket
		sock = NetWSASocket(family, socktype, protocol);
		bCloseSocket = TRUE;
	}
	ASSERT_RETNULL(sock != INVALID_SOCKET);

	net->m_WsockFn[index].Initialize(sock, family, socktype, protocol);

	if (bCloseSocket)
	{
		NetCloseSocket(sock);
	}

	return &net->m_WsockFn[index];
}


// ---------------------------------------------------------------------------
// DESC:	return pointer to name
// LOCK:	only called from within a NETCLT softlock
// ---------------------------------------------------------------------------
#ifdef NET_TRACE_0
static const char * NetCltGetName(
	NETCLT * clt)
{
	static const char * unknown = "unknown";
	const char * name = unknown;
	if (clt)
	{
		if (clt->m_szName[0])
		{
			name = clt->m_szName;
		}
		else if (clt->m_pServer && clt->m_pServer->m_szName[0])
		{
			name = clt->m_pServer->m_szName;
		}
	}
	ASSERT(name && name[0] != 0);
	return name;
}
#endif


// ---------------------------------------------------------------------------
// DESC:	increment refcount of client (this is done when posting an 
//			overlapped operation on the client from within a NetGetClientData)
// NOTE:	the ref starts at 1.  every time we get the client data, we add 1
//			every time we release client data we dec 1.  every time we call
//			an overlapped io function we add 1, every time we complete an
//			overlapped io function we dec 1.  if we want to delete the client
//			we dec 1.  if we ever dec to 0, we delete the client.
//			the only time we are every allowed to dec to 0 is on a call to 
//			NetReleaseClientData.  thus calls to NetClientAddRef() and
//			NetClientSubRef() must always occur within a NetGetClientData()
//			block
// ---------------------------------------------------------------------------
static inline LONG NetClientAddRef(CRITSEC_FUNCARGS(
	NETCLT * clt))
{
	LONG val = clt->m_ref.RefAdd(CRITSEC_FUNCNOARGS_PASSFL());
	ASSERT(val > 1);
	return val;
}


// ---------------------------------------------------------------------------
// DESC:	decrement refcount of client (this is done on completion of
//			an overlapped operation from within a NetGetClientData)
// NOTE:	we should never decrement to zero here, because we don't free
//			on zero when decrementing with this function
// ---------------------------------------------------------------------------
static inline void NetClientSubRef(CRITSEC_FUNCARGS(
	NETCLT * clt))
{
	LONG val = clt->m_ref.RefDec(CRITSEC_FUNCNOARGS_PASSFL());
	ASSERT(val > 0);
}


// ---------------------------------------------------------------------------
// DESC:	return the index of a NETCLT
// ---------------------------------------------------------------------------
static inline NETCLIENTID NetGetClientIndex(
	NETCOM * net,
	NETCLT * clt)
{
	CClientTable & tbl = net->m_ClientTable;
	ASSERT_RETINVALID(clt >= tbl.m_pClients);
	unsigned int ii = (unsigned int)(((BYTE *)clt - (BYTE *)tbl.m_pClients) >> tbl.m_nArrayShift);
	ASSERT_RETINVALID(ii < tbl.m_nArraySize);
	return ii;
}


// ---------------------------------------------------------------------------
// DESC:	return the pointer to a NETCLT given index
// ---------------------------------------------------------------------------
static FORCEINLINE NETCLT * NetGetClientByIndex(
	NETCOM * net,
	unsigned int index)
{
	CClientTable & tbl = net->m_ClientTable;
	DBG_ASSERT(tbl.m_pClients);
	DBG_ASSERT(index < tbl.m_nArraySize);
	return (NETCLT *)((BYTE *)tbl.m_pClients + (index << tbl.m_nArrayShift));
}


// ---------------------------------------------------------------------------
// DESC:	create the NETCLIENTID of a NETCLT
// ---------------------------------------------------------------------------
static inline NETCLIENTID NetMakeClientId(
	NETCOM * net,
	NETCLT * clt)
{
	return MAKE_NETCLIENTID(NetGetClientIndex(net, clt), clt->m_wIter);
}


// ---------------------------------------------------------------------------
// DESC:	return the NETCLIENTID of a NETCLT
// ---------------------------------------------------------------------------
static inline NETCLIENTID NetGetClientId(
	NETCLT * clt)
{
	return clt->m_id;
}


// ---------------------------------------------------------------------------
// DESC:	return ptr to user data
// ---------------------------------------------------------------------------
static inline void * NetGetCltUserData(
	NETCLT * clt)
{
	return (void *)((BYTE *)clt + sizeof(NETCLT));
}


// ---------------------------------------------------------------------------
// DESC:	free client table
// LOCK:	call from main thread only
// ---------------------------------------------------------------------------
static void NetFreeClientTable(
	NETCOM * net)
{
	ASSERT_RETURN(net);

	CClientTable & tbl = net->m_ClientTable;

//	ASSERT(tbl.m_nClientCount == 0);
	if (tbl.m_pClients)
	{
		for (unsigned int ii = 0; ii < tbl.m_nArraySize; ++ii)
		{
			NETCLT * clt = NetGetClientByIndex(net, ii);
			clt->m_ref.RefDec(CRITSEC_FUNCNOARGS_FILELINE());
			clt->m_ref.Free();
		}
		FREE(net->mempool, tbl.m_pClients);
	}
	if (tbl.m_pReadBuffer)
	{
		VirtualFree(tbl.m_pReadBuffer, 0, MEM_RELEASE);
	}
	if (tbl.m_pWriteBuffer)
	{
		VirtualFree(tbl.m_pWriteBuffer, 0, MEM_RELEASE);
	}
	memclear(&tbl, sizeof(CClientTable));
}


// ---------------------------------------------------------------------------
// DESC:	initialize client table
// LOCK:	call from main thread only
// ---------------------------------------------------------------------------
static BOOL NetInitClientTable(
	NETCOM * net,
	const net_setup & setup)
{
	NetFreeClientTable(net);

	// TEMP FIX for memory shortage
	unsigned int max_clients =  setup.max_clients;
	ASSERT(max_clients > 0);
	if (max_clients <= 0)
	{
		max_clients = DEFAULT_MAX_CLIENTS;
	}

	CClientTable & tbl = net->m_ClientTable;
	tbl.m_csFreeList.Init();
	tbl.m_nArraySize = max_clients;
	tbl.m_nArrayShift = HIGHBIT(sizeof(NETCLT) + net->m_nUserDataSize) - IsPowerOf2(sizeof(NETCLT) + net->m_nUserDataSize);
	tbl.m_nClientSize = (1 << tbl.m_nArrayShift);

	// use MALLOCBIG to get over 64 MB limit - BAN 8/4/2007
	tbl.m_pClients = MALLOCBIG(net->mempool, max_clients * tbl.m_nClientSize);
	memset(tbl.m_pClients, 0, max_clients * tbl.m_nClientSize);
	ASSERT_RETFALSE(tbl.m_pClients);

	// read buffers are allocated separately because winsock tcp/ip locks the memory page of buffers submitted to it
	// and therefore our buffers should be page-aligned
	tbl.m_pReadBuffer = (BYTE *)VirtualAlloc(NULL, MAX_READ_BUFFER * max_clients, MEM_COMMIT, PAGE_READWRITE);
	ASSERT_RETFALSE(tbl.m_pReadBuffer);

	// each client gets 1 write buffer, for the same reason as they get 1 read buffer.  if the non-buffered send is on
	// then they need to allocate more write buffers individually
	tbl.m_pWriteBuffer = (BYTE *)VirtualAlloc(NULL, MAX_READ_BUFFER * max_clients, MEM_COMMIT, PAGE_READWRITE);
	ASSERT_RETFALSE(tbl.m_pWriteBuffer);
	
	for (unsigned int ii = 0; ii < max_clients; ii++)				// set all client sockets to INVALID_SOCKET
	{
		NETCLT * clt = NetGetClientByIndex(net, ii);
		clt->m_hSocket = INVALID_SOCKET;
	}

	tbl.m_csFreeList.Enter();										// put all clients on free list
	{
		tbl.m_pFreeList = NetGetClientByIndex(net, 0);
		for (unsigned int ii = 1; ii < max_clients; ii++)
		{
			NETCLT * prev = NetGetClientByIndex(net, ii - 1);
			NETCLT * next = NetGetClientByIndex(net, ii);
			prev->m_pNext = next;
		}
	}
	tbl.m_csFreeList.Leave();

	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	clear a NETCLT structure, this is in a separate function
//			in case we want to preserve some members of the struct
// LOCK:	use only when initializing or freeing the object
// ---------------------------------------------------------------------------
static void NetClearClient(
	NETCOM * net,
	NETCLT * clt)
{
	if (clt)
	{
        if(clt->m_bufRead.m_nBufferSize && clt->m_bufRead.m_nBufferSize != MAX_READ_BUFFER)
        {
            VirtualFree(clt->m_bufRead.m_Buffer,0,MEM_RELEASE);
        }
		memclear(clt, net->m_ClientTable.m_nClientSize);
		clt->m_hSocket = INVALID_SOCKET;
		clt->m_id = INVALID_NETCLIENTID;
		clt->m_idLocalConnection = INVALID_NETCLIENTID;
		// point the read buffer to the appropriate memory space
		clt->m_bufRead.m_Buffer = net->m_ClientTable.m_pReadBuffer + NetGetClientIndex(net, clt) * MAX_READ_BUFFER;
        clt->m_bufRead.m_nBufferSize = MAX_READ_BUFFER;
		// point the write buffer
		clt->m_bufWrite.m_Buffer = (MSG_BUF *)(net->m_ClientTable.m_pWriteBuffer + NetGetClientIndex(net, clt) * MAX_READ_BUFFER);
		clt->m_ref.Init();
        for(int i =0; i < IOLast;i++)
        {
            clt->m_nOutstandingOverlaps[i] = 0;
        }
		clt->m_nOutstandingBytes = 0;
	}
}


// ---------------------------------------------------------------------------
// DESC:	get a new iterator
// NOTE:	the iterator is simply a way to tell if we have the right client
// LOCK:	safe to use anywhere
// ---------------------------------------------------------------------------
static WORD NetGetNewIterator(
	NETCOM * net)
{
	CClientTable & tbl = net->m_ClientTable;

	WORD iter;
	
	do 
	{
		iter = (WORD)(InterlockedIncrement(&tbl.m_nCurrentIter) & USHRT_MAX);
	} while (iter == USHRT_MAX);
	
	return iter;
}


// ---------------------------------------------------------------------------
// DESC:	get an unused NETCLT structure
// LOCK:	safe to use anywhere
// ---------------------------------------------------------------------------
static NETCLT * NetGetFreeClient(
	NETCOM * net,
	FP_NETCLIENT_INIT_USERDATA fpInitUserData = NULL)
{
	CClientTable & tbl = net->m_ClientTable;
	NETCLT * clt = NULL;

	tbl.m_csFreeList.Enter();
	{
		if (tbl.m_pFreeList)
		{
			clt = tbl.m_pFreeList;
			tbl.m_pFreeList = clt->m_pNext;
		}
	}
	tbl.m_csFreeList.Leave();
	
	if (clt)
	{
		NetClearClient(net, clt);
		if (fpInitUserData)
		{
			fpInitUserData(NetGetCltUserData(clt));
		}
		clt->m_wIter = NetGetNewIterator(net);
		clt->m_id = NetMakeClientId(net, clt);
		clt->m_ref.Init();
        InterlockedIncrement(&tbl.m_nClientCount);
	}

	return clt;
}


// ---------------------------------------------------------------------------
// DESC:	put NETCLT structure on free list
// LOCK:	safe to use so long as the clt's refcount is zero (or effectively
//			zero ie during the client's initialization & an error occurs)
// ---------------------------------------------------------------------------
static void NetRecycleClient(
	NETCOM * net,
	NETCLT * & clt)
{
	ASSERT(clt->m_ref.GetRef() == 0);

	CClientTable & tbl = net->m_ClientTable;

    ASSERT(clt != tbl.m_pFreeList);
	InterlockedExchangeAdd(&tbl.m_nClientCount, -clt->m_bCounted);

	// delete the client's pending buffer
	clt->m_bufWrite.m_csWriteBuf.Enter();
	{
		MSG_BUF * buf = NULL;
		MSG_BUF * next = clt->m_bufWrite.m_pPendingHead;
		while ((buf = next) != NULL)
		{
			next = buf->next;
			if (buf != clt->m_bufWrite.m_Buffer)
			{
				NetRecycleMsgBuf(net, buf);
			}
		}
	}
	clt->m_bufWrite.m_csWriteBuf.Leave();

	NetClearClient(net, clt);
	clt->m_wIter = (WORD)-1;
	clt->m_id = NetMakeClientId(net, clt);

	tbl.m_csFreeList.Enter();
	{
		clt->m_pNext = tbl.m_pFreeList;
		tbl.m_pFreeList = clt;
	}
	tbl.m_csFreeList.Leave();

	clt = NULL;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static inline NETCLT * NetGetClientById(
	NETCOM * net,
	NETCLIENTID idClient,
	BOOL bAssert)
{
	ASSERT_RETNULL(net);

	CClientTable & tbl = net->m_ClientTable;

	unsigned int idx = NETCLIENTID_GET_INDEX(idClient);

	ASSERT_RETNULL(idx >= 0 && idx < tbl.m_nArraySize);
	NETCLT * clt = NetGetClientByIndex(net, idx);
	if (clt->m_wIter != NETCLIENTID_GET_ITER(idClient))
	{
		ASSERT(bAssert == FALSE);
		return NULL;
	}
	return clt;
}

// ---------------------------------------------------------------------------
// DESC:	return NETCLT given a NETCLIENTID
// NOTE:	increments refcount, doing a "soft lock" ie the object won't get
//			deleted while we have it, but members are still volatile
// WARNING:	always follow NetGetClientData() with NetReleaseClientData()
// ---------------------------------------------------------------------------
static inline NETCLT * NetGetClientData(CRITSEC_FUNCARGS(
	NETCOM * net,
	NETCLIENTID idClient,
	BOOL bAssert))
{
	NETCLT * clt = NetGetClientById(net, idClient, bAssert);
	if (!clt)
	{
		return NULL;
	}

	if (clt->m_ref.RefAdd(CRITSEC_FUNCNOARGS_PASSFL()) > 0)
	{
		return clt;
	}
	else
	{
		return NULL;
	}
}


// ---------------------------------------------------------------------------
// DESC:	return NETCLT given an index...
// NOTE:	increments refcount, doing a "soft lock" ie the object won't get
//			deleted while we have it, but members are still volatile
// WARNING:	always follow NetGetClientData() with NetReleaseClientData()
// ---------------------------------------------------------------------------
static inline NETCLT * NetGetClientDataByIndex(CRITSEC_FUNCARGS(
	NETCOM * net,
	unsigned int idx))
{
	CClientTable & tbl = net->m_ClientTable;

	ASSERT_RETNULL(idx >= 0 && idx < tbl.m_nArraySize);
	NETCLT * clt = NetGetClientByIndex(net, idx);

	if (clt->m_ref.RefAdd(CRITSEC_FUNCNOARGS_PASSFL()) > 0)
	{
		return clt;
	}
	else
	{
		return NULL;
	}
}


// ---------------------------------------------------------------------------
// DESC:	release the data "locked" by NetGetClientData
// NOTE:	decrements refcount.  if the refcount is zero, we free the
//			client
// ---------------------------------------------------------------------------
static inline void NetReleaseClientData(CRITSEC_FUNCARGS(
	NETCOM * net,
	NETCLT * & clt))
{
	if (clt->m_ref.RefDec(CRITSEC_FUNCNOARGS_PASSFL()) == 0)
	{
		// delete the client!!!
		NETSRV * srv = clt->m_pServer;
		if (srv)
		{
			if	(srv->m_fpRemoveCallback && clt->m_bWasAccepted)
			{
				srv->m_fpRemoveCallback(NetGetClientId(clt), NetGetCltUserData(clt));
			}

			if (clt->m_hSocket != INVALID_SOCKET)
			{
				ASSERT(srv->m_WsockFn);
                if(srv->m_WsockFn)
                {
                          NetRecycleSocket(net, srv->m_WsockFn->m_SocketPool, clt->m_hSocket);
                }
			}
			int count = InterlockedExchangeAdd(&srv->m_nClientCount, -clt->m_bCounted);
			ASSERT(count >= 0);
			TraceNetDebug("[%s] client count (%s): %d\n", srv->m_szName, NetCltGetName(clt), count - clt->m_bCounted);
		}
		else if (clt->m_hSocket != INVALID_SOCKET)
		{
			NetCloseSocket(clt->m_hSocket);
		}
		clt->m_hSocket = INVALID_SOCKET;
		clt->m_ref.Free();
		NetRecycleClient(net, clt);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetGetClientAddress(
	NETCOM * net,
	NETCLIENTID idClient,
	sockaddr_storage * addrDest )
{
	ASSERT_RETFALSE( net );
	ASSERT_RETFALSE( addrDest );

	NETCLT * clt = NetGetClientData(CRITSEC_FUNC_FILELINE(
						net,
						idClient,
						FALSE ));

	if( clt )
	{
		ASSERT_RETFALSE(MemoryCopy(
			addrDest,
			sizeof( sockaddr_storage ),
			&clt->m_addr,
			sizeof( sockaddr_storage ) ));

		NetReleaseClientData(CRITSEC_FUNC_FILELINE(
			net,
			clt ));

		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetGetClientServerPort(
		NETCOM *	net,
		NETCLIENTID	idClient,
		UINT16 *	port )
{
	ASSERT_RETFALSE( net );
	ASSERT_RETFALSE( port );

	BOOL toRet   = FALSE;
	NETCLT * clt = NetGetClientData(CRITSEC_FUNC_FILELINE(
						net,
						idClient,
						FALSE ));
	if( clt )
	{
		if( clt->m_pServer )
		{
			(*port) = clt->m_pServer->m_nPort;
			toRet   = TRUE;
		}
		NetReleaseClientData( CRITSEC_FUNC_FILELINE(net, clt ));
	}
	return toRet;
}
// ---------------------------------------------------------------------------
// DESC:	increment a client's refcount
// NOTE:	increments refcount, doing a "soft lock" ie the object won't get
//			deleted while we have it, but members are still volatile
//			this is used to hold a reference to a client (for example, by the
//			game object)
// ---------------------------------------------------------------------------
BOOL NetAddClientRef(CRITSEC_FUNCARGS(
	NETCOM * net,
	NETCLIENTID idClient))
{
	ASSERT_RETFALSE(net);
	if (idClient == INVALID_NETCLIENTID)
	{
		return FALSE;
	}
	NETCLT * clt = NetGetClientData(CRITSEC_FUNC_PASSFL(net, idClient, FALSE));
	if (!clt)
	{
		return FALSE;
	}
	clt->m_ref.RefAdd(CRITSEC_FUNCNOARGS_PASSFL());
	NetReleaseClientData(CRITSEC_FUNC_PASSFL(net, clt));
	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	decrement a client's refcount
// NOTE:	decrements refcount, doing a "soft lock" ie the object won't get
//			deleted while we have it, but members are still volatile
//			this is used to hold a reference to a client (for example, by the
//			game object)
// WARNING:	don't call this w/o a corresponding NetAddClientRef before it!!!
// ---------------------------------------------------------------------------
BOOL NetSubClientRef(CRITSEC_FUNCARGS(
	NETCOM * net,
	NETCLIENTID idClient))
{
	ASSERT_RETFALSE(net);
	if (idClient == INVALID_NETCLIENTID)
	{
		return FALSE;
	}
	NETCLT * clt = NetGetClientData(CRITSEC_FUNC_PASSFL(net, idClient, FALSE));
	if (!clt)
	{
		return FALSE;
	}
	clt->m_ref.RefDec(CRITSEC_FUNCNOARGS_PASSFL());
	NetReleaseClientData(CRITSEC_FUNC_PASSFL(net, clt));
	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	note, these functions don't lock the user data, they simply make 
//			sure that NETCLT isn't deleted while you've got it
//			every call to NetClientGetUserData() must be followed by a call 
//			to NetClientReleaseUserData()
// ---------------------------------------------------------------------------
void * NetGetClientUserData(CRITSEC_FUNCARGS(
	NETCOM * net,
	NETCLIENTID idClient))
{
	ASSERT_RETNULL(net);
	if (idClient == INVALID_NETCLIENTID)
	{
		return NULL;
	}
	NETCLT * clt = NetGetClientData(CRITSEC_FUNC_PASSFL(net, idClient, FALSE));
	if (!clt)
	{
		return NULL;
	}
	return (void *)(NetGetCltUserData(clt));
}


// ---------------------------------------------------------------------------
// DESC:	release the client, by id
// ---------------------------------------------------------------------------
void NetReleaseClientUserData(CRITSEC_FUNCARGS(
	NETCOM * net,
	NETCLIENTID idClient))
{
	NETCLT * clt = NetGetClientById(net, idClient, TRUE);
	if (!clt)
	{
		return;
	}
	NetReleaseClientData(CRITSEC_FUNC_PASSFL(net, clt));
}


// ---------------------------------------------------------------------------
// DESC:	set the client's mailbox, should alread have reflock on client
// ---------------------------------------------------------------------------
void NetSetClientMailbox(
	NETCOM * net,
	NETCLIENTID idClient,
	struct MAILSLOT * mailslot)
{
	CClientTable & tbl = net->m_ClientTable;

	unsigned int idx = NETCLIENTID_GET_INDEX(idClient);

	ASSERT_RETURN(idx >= 0 && idx < tbl.m_nArraySize);
	NETCLT * clt = NetGetClientByIndex(net, idx);
	if (clt->m_wIter != NETCLIENTID_GET_ITER(idClient))
	{
		return;
	}

	clt->m_csMailSlot.Enter();
	{
		clt->m_mailslot = mailslot;
	}
	clt->m_csMailSlot.Leave();
}


// ---------------------------------------------------------------------------
// DESC:	set the client's command table, should already have reflock on client
// ---------------------------------------------------------------------------
void NetSetClientResponseTable(
	NETCOM * net,
	NETCLIENTID idClient,
	struct NET_COMMAND_TABLE * newTbl)
{
	CClientTable & tbl = net->m_ClientTable;

	unsigned int idx = NETCLIENTID_GET_INDEX(idClient);

	ASSERT_RETURN(idx >= 0 && idx < tbl.m_nArraySize);
	NETCLT * clt = NetGetClientByIndex(net, idx);
	if (clt->m_wIter != NETCLIENTID_GET_ITER(idClient))
	{
		return;
	}

	clt->m_csCmdTbl.Enter();
	{
		clt->m_CmdTbl = newTbl;
	}
	clt->m_csCmdTbl.Leave();
}


// ---------------------------------------------------------------------------
// DESC: set the client's encryption key, should alread have reflock on client
// ---------------------------------------------------------------------------
void NetInitClientSendEncryption(
	NETCOM * net,
	NETCLIENTID idClient,
	BYTE * key,
	size_t nBytes,
	ENCRYPTION_TYPE eEncryptionType)
{
	ASSERT_RETURN(net);

	CClientTable & tbl = net->m_ClientTable;

	unsigned int idx = NETCLIENTID_GET_INDEX(idClient);

	ASSERT_RETURN(idx >= 0 && idx < tbl.m_nArraySize);
	NETCLT * clt = NetGetClientByIndex(net, idx);
	if (clt->m_wIter != NETCLIENTID_GET_ITER(idClient))
	{
		return;
	}

	clt->m_encoderSend.InitEncryption(key, nBytes, eEncryptionType);
}


// ---------------------------------------------------------------------------
// DESC: set the client's encryption key, should alread have reflock on client
// ---------------------------------------------------------------------------
void NetInitClientReceiveEncryption(
	NETCOM * net,
	NETCLIENTID idClient,
	BYTE * key,
	size_t nBytes,
	ENCRYPTION_TYPE eEncryptionType)
{
	ASSERT_RETURN(net);

	CClientTable & tbl = net->m_ClientTable;

	unsigned int idx = NETCLIENTID_GET_INDEX(idClient);

	ASSERT_RETURN(idx >= 0 && idx < tbl.m_nArraySize);
	NETCLT * clt = NetGetClientByIndex(net, idx);
	if (clt->m_wIter != NETCLIENTID_GET_ITER(idClient))
	{
		return;
	}

	clt->m_encoderReceive.InitEncryption(key, nBytes, eEncryptionType);
}


// ---------------------------------------------------------------------------
// DESC:	get an unused NETSRV
// LOCK:	only safe if called from the main thread
// ---------------------------------------------------------------------------
static NETSRV * NetGetFreeServer(
	NETCOM * net)
{
	for (int ii = 0; ii < MAX_SERVERS; ii++)
	{
		NETSRV * srv = &net->m_Servers[ii];
		if (srv->net == NULL)
		{
			memclear(srv, sizeof(NETSRV));
			srv->net = net;
			srv->m_hSocket = INVALID_SOCKET;
			return srv;
		}
	}
	return NULL;
}


// ---------------------------------------------------------------------------
// DESC:	get an empty NETSRV
// LOCK:	only safe if called from the main thread
// ---------------------------------------------------------------------------
static void NetRecycleServer(
	NETCOM * net,
	NETSRV * & srv)
{
	UNREFERENCED_PARAMETER(net);
	memclear(srv, sizeof(NETSRV));
	srv->m_hSocket = INVALID_SOCKET;
	srv = NULL;
}


// ---------------------------------------------------------------------------
// DESC:	get an empty msgbuf structure
// LOCK:	safe from anywhere
// ---------------------------------------------------------------------------
static MSG_BUF * NetGetFreeMsgBuf(
	NETCOM * net)
{
	MSG_BUF * msg = NULL;
    NetPerThreadBufferPool * pPTBP = net->m_PTBPCollection.Get(net);
    ASSERT_RETNULL(pPTBP);
    {
        CMessagePool & pool = pPTBP->m_MessagePool;

		msg = pool.Get(net->mempool);

		if(pPTBP->m_PerfInstance)
		{
			PERF_ADD64(pPTBP->m_PerfInstance,CommonNetBufferPool,msgBufAllocationRate,1);
		}
    }
	return msg;
}


// ---------------------------------------------------------------------------
// DESC:	recycle a msg structure
// LOCK:	safe from anywhere
// ---------------------------------------------------------------------------
static void NetRecycleMsgBuf(
	NETCOM * net,
	MSG_BUF * & msg)
{
    // Free back to current thread's pool
    NetPerThreadBufferPool * pPTBP = NetGetPerThreadBufferPool(net);
    ASSERT_RETURN(pPTBP);
    {
        CMessagePool & pool = pPTBP->m_MessagePool;

		pool.Recycle(net->mempool, msg);
    }

	msg = NULL;
}


// ---------------------------------------------------------------------------
// DESC:	we sometimes reclassify the iotype of an overlap, in which case
//			we should call NetRetypeOverlap() to properly track it
// ---------------------------------------------------------------------------
static inline OVERLAPPEDEX * NetRetypeOverlap(
	NETCOM * net,
	OVERLAPPEDEX * overlap,
	IOType newtype)
{
	REF(net);
#if NET_DEBUG
	ASSERT(overlap->m_ioType < IOLast);
	//ASSERT(InterlockedDecrement(&net->m_OverlapPool.m_nOverlapCountPer[overlap->m_ioType]) >= 0);
	ASSERT(newtype < IOLast);
	//InterlockedIncrement(&net->m_OverlapPool.m_nOverlapCountPer[newtype]);
#endif

    NetPerThreadBufferPool * oldp = overlap->m_pParentPTBP;
	memclear(overlap, sizeof(OVERLAPPEDEX));
	overlap->m_idClient = INVALID_NETCLIENTID;
	overlap->m_ioType = newtype;
    overlap->m_pParentPTBP = oldp;

#if ISVERSION(DEBUG_VERSION)
	overlap->m_dwTickCount = GetTickCount();
#endif

	return overlap;
}


// ---------------------------------------------------------------------------
// DESC:	get an empty overlap structure
// LOCK:	safe from anywhere
// ---------------------------------------------------------------------------
static OVERLAPPEDEX * NetGetFreeOverlap(
	NETCOM * net,
	IOType iotype)
{
	OVERLAPPEDEX * overlap = NULL;
    NetPerThreadBufferPool* pPTBP = net->m_PTBPCollection.Get(net);
    {
        COverlapPool & pool = pPTBP->m_OverlapPool;
		overlap = pool.Get(net->mempool);
		ASSERT_RETNULL(overlap);

		memclear(overlap, sizeof(OVERLAPPEDEX));
		overlap->m_pParentPTBP = pPTBP;
        overlap->m_ioType = iotype;
        overlap->m_idClient = INVALID_NETCLIENTID;

		if(pPTBP->m_PerfInstance)
		{
			PERF_ADD64(pPTBP->m_PerfInstance,CommonNetBufferPool,overlapAllocationRate,1);
		}

#if ISVERSION(DEBUG_VERSION)
        overlap->m_dwTickCount = GetTickCount();
#endif
    }

	return overlap;
}


// ---------------------------------------------------------------------------
// DESC:	get an empty overlap structure, reuse if we have one
// LOCK:	safe from anywhere
// ---------------------------------------------------------------------------
static OVERLAPPEDEX * NetGetOrRecycleOverlap(
	NETCOM * net,
	IOType iotype,
	OVERLAPPEDEX * overlap)
{
	if (overlap)
	{
		overlap = NetRetypeOverlap(net, overlap, iotype);
	}
	else
	{
		overlap = NetGetFreeOverlap(net, iotype);
	}

	return overlap;
}


// ---------------------------------------------------------------------------
// DESC:	recycle an overlap structure
// ---------------------------------------------------------------------------
static void NetRecycleOverlap(
	NETCOM * net,
	OVERLAPPEDEX * & overlap)
{
    NetPerThreadBufferPool * pPTBP = net->m_PTBPCollection.Get(net);
    ASSERT_RETURN(pPTBP);
	{
        COverlapPool & pool = pPTBP->m_OverlapPool;

        if (overlap->m_buf)
        {
            NetRecycleMsgBuf(net, overlap->m_buf);
        }

		pool.Recycle(net->mempool, overlap);
    }
	overlap = NULL;
}


// ---------------------------------------------------------------------------
// DESC:	call disconnectex for a client
// LOCK:	
// ---------------------------------------------------------------------------
static BOOL NetDisconnectEx(
	NETCOM * net,
	NETCLT * clt,
	OVERLAPPEDEX * & overlap,
	BOOL bGraceful = FALSE)
{
	ASSERT_RETFALSE(clt);

	if (InterlockedCompareExchange(&clt->m_eClientState, NETCLIENT_STATE_DISCONNECTING, NETCLIENT_STATE_CONNECTED) != NETCLIENT_STATE_CONNECTED)
	{
/*
#if NET_TRACE_0
		trace("[%s] NetDisconnectEx() ERROR -- Already disconnecting!  (socket:%d)\n", clt->m_szName, clt->m_hSocket);
#endif
*/
		if (InterlockedCompareExchange(&clt->m_eClientState, NETCLIENT_STATE_DISCONNECTING, NETCLIENT_STATE_CONNECTFAILED) != NETCLIENT_STATE_CONNECTFAILED)
		{
			if (overlap)
			{
				NetRecycleOverlap(net, overlap);
			}
			return TRUE;
		}
	}
	ASSERT(clt->m_eClientState == NETCLIENT_STATE_DISCONNECTING);		// TODO: what about other client states???

	NETSRV * srv = clt->m_pServer;
	if (srv)
	{
		if (srv->m_fpDisconnectCallback)
		{
			srv->m_fpDisconnectCallback(net,NetGetClientId(clt));
		}
	}

	if (clt->m_idLocalConnection != INVALID_NETCLIENTID)			// immediately disconnect local connection clients
	{
		NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));										// this should cause the client to get deleted anon

		//	now close the opposite local side connection
		NetCloseClient(
			net,
			clt->m_idLocalConnection,
			TRUE );
		return TRUE;
	}

	if (clt->m_hSocket == INVALID_SOCKET)
	{
		TraceNetDebug("[%s] NetDisconnectEx() ERROR -- Invalid Socket!  (client:0x%08x)\n", clt->m_szName, clt->m_id);
		ASSERT(0);
		goto _error;
	}

	TraceNetDebug("[%s] NetDisconnectEx() (client: 0x%08x)\n", clt->m_szName, clt->m_id);

	LINGER linger;
	linger.l_onoff = (bGraceful ? 0 : 1);
	linger.l_linger = 0;
	NetSetSockopt(clt->m_hSocket, SOL_SOCKET, SO_LINGER, (const char *)&linger, sizeof(linger));

	// cancel pending io requests
	// NOTE: All I/O operations that are canceled will complete with the error ERROR_OPERATION_ABORTED. All completion notifications for the I/O operations will occur normally.
	if (CancelIo((HANDLE)clt->m_hSocket) == 0)
	{
		ASSERT(0);
		// error = GetLastError();
	}

	// get a new overlapped buffer
	overlap = NetGetOrRecycleOverlap(net, IODisconnectEx, overlap);
	ASSERT_RETFALSE(overlap);
	InterlockedIncrement(&clt->m_nOutstandingOverlaps[IODisconnectEx]);
	overlap->m_idClient = NetGetClientId(clt);
	ASSERT_RETFALSE(overlap->m_idClient != INVALID_NETCLIENTID);

	DWORD flags = 0;


	NetClientAddRef(CRITSEC_FUNC_FILELINE(clt));
	ASSERTV_GOTO(clt->m_WsockFn,_error,"No disconnectex function pointer found\n");
#if !ISVERSION(SERVER_VERSION)
	if (FakeDisconnectEx(net,clt->m_hSocket,overlap,flags) == FALSE)
#else
	if (clt->m_WsockFn->NetDisconnectEx(clt->m_hSocket, overlap, flags) == FALSE)
#endif
	{
		int error = WSAGetLastError();
		switch (error)
		{
		case ERROR_IO_PENDING:
			break;
		default:
			ASSERT(0);
			NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));

			ASSERT(InterlockedDecrement(&clt->m_nOutstandingOverlaps[IODisconnectEx]) >= 0)
			TraceError("[%s] NetDisconnectEx() ERROR! (client:0x%08x)\n", NetCltGetName(clt), clt->m_id);
			NetErrorMessage(error);
			goto _error;
		}
	}
	else
	{
		TraceNetDebug("DisconnectEx() returned immediate success\n");
	}

	
	return TRUE;

_error:
	if (overlap)
	{
		ASSERT(InterlockedDecrement(&clt->m_nOutstandingOverlaps[IODisconnectEx]) >= 0);
		NetRecycleOverlap(net, overlap);
	}
	return FALSE;
}


// ---------------------------------------------------------------------------
// DESC:	choose a send function
// ---------------------------------------------------------------------------
FP_NET_SEND NetSelectSendFunction(
	BOOL local,
	unsigned int nagle,
	BOOL sequential)
{
	if (local)
	{
		return NetSendLocal;
	}
	if (sequential)
	{
		return NetSendSerial;
	}
	else
	{
		if (nagle > NAGLE_OFF)
		{
			return NetSendParallelNagle;
		}
		else
		{
			return NetSendParallel;
		}
	}
}


// ---------------------------------------------------------------------------
// DESC:	when a client is used by a server, it inherits certain server
//			settings
// ---------------------------------------------------------------------------
static void NetInheritServerOptions(
	NETSRV * srv,
	NETCLT * clt)
{
	ASSERT(clt->m_idLocalConnection == INVALID_NETCLIENTID);

	clt->m_pServer = srv;
	clt->m_WsockFn = srv->m_WsockFn;

	clt->m_CmdTbl = srv->m_CmdTbl;
	clt->m_mailslot = srv->m_mailslot;

	clt->m_nNagle = srv->m_nNagle;
	clt->m_bSequentialWrites = srv->m_bSequentialWrites;
	clt->m_bDontLockOnSend = srv->m_bDontLockOnSend;
	clt->m_fpNetSend = srv->m_fpNetSend;
    clt->m_fpDirectReadCallback = srv->m_fpDirectReadCallback;
}


//----------------------------------------------------------------------------
// DESC:	returns true if the given adder is marked as banned from connecting.
//----------------------------------------------------------------------------
static BOOL NetCheckBanList(
	__notnull NETSRV * srv,
	__notnull SOCKADDR_STORAGE * pAddr )
{
	BOOL toRet = FALSE;

	srv->m_banListLock.ReadLock();

	toRet = ( srv->m_banList && srv->m_banList->HasAddr( pAddr[0] ) );

	srv->m_banListLock.EndRead();

	return toRet;
}


// ---------------------------------------------------------------------------
// DESC:	post an acceptex
// LOCK:	this is done from any thread
// TODO:	if we're creating a socket, this should probably be modified
//			to happen on another thread?
// ---------------------------------------------------------------------------
static BOOL NetAcceptEx(
	NETCOM * net,
	NETSRV * srv)
{

	int tryagain = 3;

_tryagain:
	NETCLT * clt = NULL;

	OVERLAPPEDEX * overlap = NetGetFreeOverlap(net, IOAccept);
	if (!overlap)
	{
		tryagain = 0;
		goto _error;
	}

	// get a new client
	clt = NetGetFreeClient(net, srv->m_fpInitUserData);
	if (!clt)
	{
		tryagain = 0;
		goto _error;
	}
    clt->m_bNonBlocking = srv->m_bNonBlocking;

	
	// get a socket for the client
	ASSERT_GOTO(srv->m_WsockFn,_error);
	clt->m_hSocket = NetGetSocket(net, clt->m_nWorkerCompletionPortIndex,srv->m_WsockFn->m_SocketPool);
	if (clt->m_hSocket == INVALID_SOCKET)
	{
		TraceNetDebug("NetAcceptEx() -- ERROR invalid socket\n");
		goto _error;
	}

	NetInheritServerOptions(srv, clt);

	// turn off nagle buffering if m_nNagle isn't NAGLE_DEFAULT
	NetSetSockopt(clt->m_hSocket, IPPROTO_TCP, TCP_NODELAY, clt->m_nNagle != NAGLE_DEFAULT);

	clt->m_eClientState = NETCLIENT_STATE_LISTENING;

	overlap->m_idClient = NetGetClientId(clt);
	overlap->m_wsabuf.buf = (char *)clt->m_bufRead.m_Buffer;
	overlap->m_wsabuf.len = MAX_READ_BUFFER;
	
	NetClientAddRef(CRITSEC_FUNC_FILELINE(clt));
	DWORD dwBytes = 0;
	ASSERTV_GOTO(srv->m_WsockFn,_error,"No function pointer found\n");

	InterlockedIncrement(&srv->m_nOutstandingAccepts);
	InterlockedIncrement(&clt->m_nOutstandingOverlaps[IOAccept]);

	if (srv->m_WsockFn->NetAcceptEx(srv->m_hSocket, 
                                    clt->m_hSocket, 
                                    (PVOID)overlap->m_wsabuf.buf,
                                    0, 
                                    (DWORD)(srv->m_addrlen + 16), 
                                    (DWORD)(srv->m_addrlen + 16), 
                                    &dwBytes, 
                                    overlap) != TRUE)
	{
		int error = WSAGetLastError();
		if(error != WSA_IO_PENDING)
		{
			ASSERT(0);
			NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));
			NetErrorMessage(error);
			tryagain = 0;
            ASSERT(InterlockedDecrement(&clt->m_nOutstandingOverlaps[IOAccept]) >= 0);
            InterlockedDecrement(&srv->m_nOutstandingAccepts);
			goto _error;
		}
	}

	TraceNetDebug("NetAcceptEx() (client:0x%08x)\n", clt->m_id);

	return TRUE;

_error:
	if (overlap)
	{
		NetRecycleOverlap(net, overlap);
	}
	if (clt)
	{
#if !ISVERSION(RETAIL_VERSION)
		TraceError("NetAcceptEx() FAILED!!!\n");
#endif
		ASSERTV_GOTO(srv->m_WsockFn,_error,"No function pointer found\n");
		NetRecycleSocket(net, srv->m_WsockFn->m_SocketPool, clt->m_hSocket);
		clt->m_ref.RefDec(CRITSEC_FUNCNOARGS_FILELINE());
		clt->m_ref.Free();
		NetRecycleClient(net, clt);
	}

	if (--tryagain > 0)
	{
		goto _tryagain;
	}

	return FALSE;
}


// ---------------------------------------------------------------------------
// DESC:	issue a read request
// LOCK:	worker thread only, each client can have at most 1 recv
// NOTE:	we should always have 1 and only 1 outstanding read request per 
//			client.  if not, we end up with message ordering issues
// ---------------------------------------------------------------------------
BOOL NetRead(
	NETCOM * net,
	NETCLT * clt,
	OVERLAPPEDEX * & overlap)
{
#if ENFORCE_NO_NAGLE
	sEnforceNonNagle(clt->m_hSocket);
#endif
	ASSERT(clt->m_idLocalConnection == INVALID_NETCLIENTID);

	// get a new overlapped buffer
	overlap = NetGetOrRecycleOverlap(net, IORead, overlap);
	ASSERT_RETFALSE(overlap);
	overlap->m_idClient = NetGetClientId(clt);
	ASSERT_RETFALSE(overlap->m_idClient != INVALID_NETCLIENTID);
	ASSERT_RETFALSE(clt->m_bufRead.m_nLen < MAX_READ_BUFFER);
	overlap->m_wsabuf.buf = (char *)clt->m_bufRead.m_Buffer + clt->m_bufRead.m_nLen;
	overlap->m_wsabuf.len = MAX_READ_BUFFER - clt->m_bufRead.m_nLen;

	if (clt->m_eClientState != NETCLIENT_STATE_CONNECTED)			// not determinate, due to soft lock
	{
		goto _error;
	}

	InterlockedIncrement(&clt->m_nOutstandingOverlaps[IORead]);

	NetClientAddRef(CRITSEC_FUNC_FILELINE(clt));										// always done within a soft lock, so never returns 0
	DWORD bytes = 0;
	DWORD flags = MSG_PARTIAL;
	if (WSARecv(clt->m_hSocket, &overlap->m_wsabuf, 1, &bytes, &flags, (LPWSAOVERLAPPED)overlap, NULL) == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		switch (error)
		{
		case WSA_IO_PENDING:
			break;

		case WSAEINTR:
		case WSAEFAULT:				// the lpBuffers parameter invalid
		case WSAEINVAL:				// the socket hasn't been bound
		case WSAEWOULDBLOCK:
		case WSAEINPROGRESS:
		case WSAENOTSOCK:
		case WSAEMSGSIZE:
		case WSAEOPNOTSUPP:
		case WSAENETDOWN:
		case WSAENOTCONN:
		case WSAESHUTDOWN:
		case WSAETIMEDOUT:
		case WSANOTINITIALISED:
		default:
			NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));
			NetErrorMessage(error);
			goto _error;

		case WSAENETRESET:
		case WSAECONNABORTED:
		case WSAECONNRESET:
			NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));
			TraceNetDebug("WSARecv() -- connection reset (client:0x%08x)\n", clt->m_id);
			goto _error;
		}
	}
	else
	{
		// assume completion status gets posted automagically
		return TRUE;
	}

	return TRUE;

_error:
	NetDisconnectEx(net, clt, overlap);
	return FALSE;
}


// ---------------------------------------------------------------------------
// DESC:	peek into read buffer and return current message size
// LOCK:	the readbuf should be either locked or thread-safe
// RETURN:	0 if not enough info to get message size, -1 if error
// TODO:	look into using scatter/gatter to avoid having to do
//			a memmove
// ---------------------------------------------------------------------------
static int NetGetMessageSize(
	const NET_COMMAND_TABLE * cmd_tbl,
	NET_READ_BUF & buf,
	unsigned int start,
	NET_CMD * cmd)
{
	if (buf.m_nCurMessageSize > 0)
	{
		*cmd = buf.m_curCmd;
		return buf.m_nCurMessageSize;
	}

	buf.m_nCurMessageSize = 0;

	int len = buf.m_nLen - start;
	if (len < sizeof(NET_CMD))
	{
		return 0;
	}

	BYTE * cur = buf.m_Buffer + start;

	*cmd = *(NET_CMD*)cur;
	if (*cmd >= cmd_tbl->m_nCmdCount)
	{
		*cmd = 0;
		buf.m_curCmd = (NET_CMD)-1;
		buf.m_nCurMessageSize = (unsigned int)-1;
		return -1;
	}

	buf.m_curCmd = *cmd;
	NET_COMMAND * cmd_def = cmd_tbl->m_Cmd + *cmd;
	unsigned int size = cmd_def->maxsize;
	if (cmd_def->varsize)
	{
		if (len < MSG_HDR_SIZE)
		{
			return 0;
		}
		size = (int)(*(MSG_SIZE *)(cur + MSG_HEADER::GetSizeOffset()));
		buf.m_nCurMessageSize = size;
		return size;
	}
	else if (size > 0)
	{
		buf.m_nCurMessageSize = size;
		return size;
	}
	else
	{
		ASSERT(0);
		buf.m_curCmd = (NET_CMD)-1;
		buf.m_nCurMessageSize = (unsigned int)-1;
		return -1;		// invalid command
	}
}


// ---------------------------------------------------------------------------
// DESC:	process read buffer (get complete messages off and post them)
// LOCK:	the readbuf should be either locked or thread-safe
// RETURN:	FALSE if connection is killed, TRUE if done
// TODO:	look into using scatter/gatter to avoid having to do
//			a memmove
// ---------------------------------------------------------------------------
static BOOL NetProcessReadBuffer(
	NETCOM * net,
	NETCLT * clt)
{
    NET_COMMAND_TABLE * cmd_tbl = NULL;
	clt->m_csCmdTbl.Enter();
    {
        cmd_tbl = clt->m_CmdTbl;
    }
	clt->m_csCmdTbl.Leave();

	NET_READ_BUF & read_buf = clt->m_bufRead;
	unsigned int start = 0;
	unsigned int len = read_buf.m_nLen;
	BYTE * buf = read_buf.m_Buffer;
	int result = FALSE;
	NETCLIENTID idClient = NetGetClientId(clt);

	ONCE
	{
		ASSERT_BREAK(clt->m_bBytesUnencrypted <= len);
		clt->m_encoderReceive.Decode( buf + clt->m_bBytesUnencrypted, len - clt->m_bBytesUnencrypted );
	}

    if (clt->m_fpDirectReadCallback != NULL)
    {
        DWORD bread = 0;

        if(clt->m_bCloseCalled == FALSE)
        {
            result = clt->m_fpDirectReadCallback(net,clt->m_id,0,buf,len,&bread); 
        }
		else
		{
			bread = len;
		}

        start = bread;

        goto End;

    }
	ASSERT_RETFALSE(cmd_tbl);
	do
	{
		if(clt->m_bCloseCalled == TRUE)
		{	//RJD added, abort on closed clients.
			TraceNetDebug("Reading buffer of closed client %d, aborting.", idClient);
			start = len;
			goto End;
		}
		NET_CMD cmd;
		int msg_size = NetGetMessageSize(cmd_tbl, read_buf, start, &cmd);
		if (msg_size <= 0)
		{
			result = (msg_size == 0);
			break;
		}
        if((unsigned)msg_size > read_buf.m_nBufferSize) 
        {
            BYTE *ptr = (BYTE*)VirtualAlloc(NULL,msg_size,MEM_COMMIT,PAGE_READWRITE);
            ASSERT_RETVAL(ptr,FALSE);
            ASSERT_RETFALSE(MemoryCopy(ptr,msg_size,read_buf.m_Buffer,read_buf.m_nLen));
            read_buf.m_Buffer = ptr;
            read_buf.m_nBufferSize = msg_size;
        }
		if (len - start < (unsigned int)msg_size)	// don't have enough in buffer to read off a message yet
		{
			result = TRUE;
			break;
		}

		ASSERT_RETVAL(cmd >= 0 && cmd < cmd_tbl->m_nCmdCount, FALSE);

#if NET_TRACE_MSG
		char txt[1024];
		DbgPrintBuf(txt, 1024, buf, msg_size);
		trace("[%s] Recv: %s\n", clt->m_szName, txt);
#endif

#if NET_MSG_COUNTER
		DWORD msg_counter = *(DWORD *)(buf + sizeof(NET_CMD) + sizeof(MSG_SIZE));
		ASSERT(msg_counter == clt->m_nMsgRecvCounter++);
		trace("[%s] msg_recv_counter: %d  cmd: %d  size: %d  checksum: %08x left: %d\n", NetCltGetName(clt), msg_counter, cmd, msg_size, CRC(0, buf, msg_size), len - msg_size - start);
#endif

#if NET_MSG_COUNT
		NET_MSG_TRACKER_NODE & tracker = cmd_tbl->m_Tracker[cmd];
		tracker.IncrementRecvCount();
		tracker.AddRecvBytes((LONGLONG)msg_size);
#endif

		read_buf.m_nCurMessageSize = 0;
		if (cmd_tbl->m_Cmd[cmd].mfp_ImmCallback)			// process immediate
		{
			cmd_tbl->m_Cmd[cmd].mfp_ImmCallback(net, NetGetClientId(clt), buf, msg_size);
		}
		else												// put in mail slot
		{
			clt->m_csMailSlot.Enter();
			{
				if (clt->m_mailslot)
				{
					MSG_BUF * msg = MailSlotGetBuf(clt->m_mailslot);
					ASSERT_RETFALSE(msg);

					int saved_size = MIN<int>(msg_size, clt->m_mailslot->msg_size);
					msg->size = saved_size;
					msg->source = MSG_SOURCE_CLIENT;
					msg->idClient = idClient;
					memcpy(msg->msg, buf, saved_size);
					MailSlotAddMsg(clt->m_mailslot, msg);
				}
			}
			clt->m_csMailSlot.Leave();
		}
		start += msg_size;
		buf += msg_size;
	} while (TRUE);

End:
	ASSERT_RETFALSE(start <= MAX_READ_BUFFER);
	ASSERT_RETFALSE(read_buf.m_nLen >= start);
	read_buf.m_nLen -= start;
	ASSERT_RETFALSE(MemoryMove(read_buf.m_Buffer, MAX_READ_BUFFER, read_buf.m_Buffer + start, read_buf.m_nLen));
	clt->m_bBytesUnencrypted = read_buf.m_nLen;
	return result;
}


// ---------------------------------------------------------------------------
// DESC:	call WSASend
// ---------------------------------------------------------------------------
static BOOL NetWSASend(
	NETCOM * net,
	NETCLT * clt,
	OVERLAPPEDEX * & overlap)
{
#if ENFORCE_NO_NAGLE
	sEnforceNonNagle(clt->m_hSocket);
#endif
	ASSERT(clt->m_idLocalConnection == INVALID_NETCLIENTID);

	BOOL result = FALSE;
	DWORD dwBytesSent = 0;

	InterlockedIncrement(&clt->m_nOutstandingOverlaps[IOWrite]);

	NetClientAddRef(CRITSEC_FUNC_FILELINE(clt));

    if(clt->m_pServer && clt->m_pServer->m_PerfInstance)
    {
        PERF_ADD64(clt->m_pServer->m_PerfInstance,CommonNetServer,numBytesOutstanding,overlap->m_wsabuf.len);
        PERF_ADD64(clt->m_pServer->m_PerfInstance,CommonNetServer,numSendsPending,1);
    }

	InterlockedExchangeAdd(&clt->m_nOutstandingBytes,overlap->m_wsabuf.len);

	if (WSASend(clt->m_hSocket, &overlap->m_wsabuf, 1, &dwBytesSent, 0, (LPWSAOVERLAPPED)overlap, NULL) == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		switch (error)
		{
		case ERROR_IO_PENDING:
			result = TRUE;
			break;

		default:
			InterlockedExchangeAdd(&clt->m_nOutstandingBytes, 0 - overlap->m_wsabuf.len);
            if(clt->m_pServer && clt->m_pServer->m_PerfInstance)
            {
                PERF_ADD64(clt->m_pServer->m_PerfInstance,CommonNetServer,numBytesOutstanding,-(int)overlap->m_wsabuf.len);
                PERF_ADD64(clt->m_pServer->m_PerfInstance,CommonNetServer,numSendsPending,-1);
            }
			NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));
			ASSERT(InterlockedDecrement(&clt->m_nOutstandingOverlaps[IOWrite]) >= 0);
			TraceNetDebug("send error! -- overlap: %p\n", overlap);
			NetErrorMessage(error, FALSE);
			NetRecycleOverlap(net, overlap);
			break;

		}
	}
	else
	{
		// assume completion status gets posted automagically
		result = TRUE;
	}
	return result;
}


// ---------------------------------------------------------------------------
// DESC:	send a message (serially)
// LOCK:	NETCLT soft lock
// ---------------------------------------------------------------------------
static MSG_BUF * NetGetMsgBufForSend(
	NETCOM * net,
	NETCLT * clt,
	MSG_BUF * & buf)
{
	MSG_BUF * msg = NULL;
	buf = NULL;

	clt->m_bufWrite.m_csWriteBuf.Enter();
	{
		if (!clt->m_bufWrite.m_bDefaultBufferInUse)
		{
			msg = clt->m_bufWrite.m_Buffer;
			clt->m_bufWrite.m_bDefaultBufferInUse = TRUE;
		}
	}
	clt->m_bufWrite.m_csWriteBuf.Leave();
	if (!msg)
	{
		msg = NetGetFreeMsgBuf(net);
		buf = msg;
	}
	return msg;
}


// ---------------------------------------------------------------------------
// DESC:	send a message (serially)
// LOCK:	NETCLT soft lock
// ---------------------------------------------------------------------------
static BOOL NetSendSerial(
	NETCOM * net,
	NETCLT * clt,
	const void * data,
	unsigned int size)
{
	OVERLAPPEDEX * overlap = NULL;
	MSG_BUF * buf = NULL;	// == msg if we allocated it, otherwise NULL if we're using the default buf
	MSG_BUF * msg = NetGetMsgBufForSend(net, clt, buf);
	ASSERT_GOTO(msg, _error);

	DBG_ASSERT(size <= MAX_MESSAGE_SIZE);
	memcpy(msg->msg, data, size);
	msg->size = size;

	BOOL bEnqueued = FALSE;
	clt->m_bufWrite.m_csWriteBuf.Enter();
	{
		if (clt->m_bufWrite.m_bSending == TRUE)			// currently sending
		{
			if (clt->m_bufWrite.m_pPendingHead)			// put it on the pending list
			{
				ASSERT_GOTO(clt->m_bufWrite.m_pPendingTail,_error);
				clt->m_bufWrite.m_pPendingTail->next = msg;
				clt->m_bufWrite.m_pPendingTail = msg;
			}
			else
			{
				clt->m_bufWrite.m_pPendingHead = msg;
				clt->m_bufWrite.m_pPendingTail = msg;
			}
			bEnqueued = TRUE;
		}
		else
		{
			clt->m_bufWrite.m_bSending = TRUE;
		}
	}
	clt->m_bufWrite.m_csWriteBuf.Leave();

	if (bEnqueued)
	{
		return TRUE;
	}

	// get a new overlapped buffer
	overlap = NetGetFreeOverlap(net, IOWrite);
	ASSERT_GOTO(overlap,_error);
	overlap->m_idClient = NetGetClientId(clt);
	overlap->m_buf = buf;
	overlap->m_wsabuf.buf = (char *)clt->m_bufWrite.m_Buffer->msg;
	overlap->m_wsabuf.len = size;

	if (!NetWSASend(net, clt, overlap))
	{
		goto _error;
	}
	return TRUE;

_error:
	TraceNetDebug("[%s] NetSendSerial() -- WSASend() failed. disconnecting client\n", clt->m_szName);
	NetDisconnectEx(net, clt, overlap);
	return FALSE;
}


// ---------------------------------------------------------------------------
// DESC:	send a pending message
// LOCK:	NETCLT soft lock, WRITE_BUF hard lock
// ---------------------------------------------------------------------------
static BOOL NetSendPending(
	NETCOM * net,
	NETCLT * clt,
	OVERLAPPEDEX * & overlap)
{
	MSG_BUF * msg = NULL;
	clt->m_bufWrite.m_csWriteBuf.Enter();
	{
		ASSERT(clt->m_bufWrite.m_bSending == 1);

		if (overlap->m_buf == NULL)
		{
			clt->m_bufWrite.m_bDefaultBufferInUse = FALSE;
		}

		msg = clt->m_bufWrite.m_pPendingHead;
		if (msg)
		{
			clt->m_bufWrite.m_pPendingHead = msg->next;
			if (!clt->m_bufWrite.m_pPendingHead)
			{
				clt->m_bufWrite.m_pPendingTail = NULL;
			}
		}
		else
		{
			clt->m_bufWrite.m_bSending = FALSE;
		}
	}
	clt->m_bufWrite.m_csWriteBuf.Leave();

	if (!msg)
	{
		NetRecycleOverlap(net, overlap);
		return TRUE;
	}

	if (overlap->m_buf)
	{
		NetRecycleMsgBuf(net, overlap->m_buf);
	}

	msg->next = NULL;
	overlap->m_buf = (msg == clt->m_bufWrite.m_Buffer ? NULL : msg);
	overlap->m_wsabuf.buf = (char *)msg->msg;
	overlap->m_wsabuf.len = msg->size;

	if (!NetWSASend(net, clt, overlap))
	{
		if (overlap && overlap->m_buf)
		{
			NetRecycleMsgBuf(net, overlap->m_buf);
		}
		return FALSE;
	}

	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	send a message (parallel)
// LOCK:	NETCLT soft lock
// ---------------------------------------------------------------------------
static BOOL NetSendParallel(
	NETCOM * net,
	NETCLT * clt,
	const void * data,
	unsigned int size)
{
	OVERLAPPEDEX * overlap = NULL;
	
	MSG_BUF * buf = NULL;	// == msg if we allocated it, otherwise NULL if we're using the default buf
	MSG_BUF * msg = NetGetMsgBufForSend(net, clt, buf);
	ASSERT_GOTO(msg, _error);

	// get a new overlapped buffer
	overlap = NetGetFreeOverlap(net, IOWrite);
	ASSERT_GOTO(overlap, _error);
	overlap->m_idClient = NetGetClientId(clt);
	overlap->m_buf = buf;
	overlap->m_wsabuf.buf = (char *)msg->msg;
	overlap->m_wsabuf.len = size;
	DBG_ASSERT(size <= MAX_MESSAGE_SIZE);
	memcpy(msg->msg, data, size);

	if (!NetWSASend(net, clt, overlap))
	{
		goto _error;
	}

	return TRUE;

_error:
	TraceNetDebug("[%s] NetSendParallel() -- WSASend() failed. disconnecting client\n", clt->m_szName);
	NetDisconnectEx(net, clt, overlap, FALSE);
	
	return FALSE;
}


// ---------------------------------------------------------------------------
// DESC:	send a message (parallel)
// LOCK:	NETCLT soft lock
// ---------------------------------------------------------------------------
static BOOL NetSendParallelNagle(
	NETCOM * net,
	NETCLT * clt,
	const void * data,
	unsigned int size)
{
	OVERLAPPEDEX * overlap = NULL;
	
	MSG_BUF * msg = NULL;
	MSG_BUF * next = NetGetFreeMsgBuf(net);
	ASSERT_GOTO(next, _error);

	clt->m_bufWrite.m_csWriteBuf.Enter();
	{
		MSG_BUF * cur = clt->m_bufWrite.m_pPendingHead;
		if (cur)												// there's a pending message
		{
			if (size == 0 || cur->size + size >= clt->m_nNagle)	// send the pending message
			{
				clt->m_bufWrite.m_pPendingHead = NULL;
				msg = cur;
			}
			else												// append the message
			{
				memcpy(cur->msg + cur->size, data, size);
				cur->size += size;
			}
		}
		else													// no pending message
		{
			if (clt->m_bufWrite.m_bDefaultBufferInUse)
			{
				msg = next;										// make the new msg the pending message
				next = NULL;
			}
			else
			{
				msg = clt->m_bufWrite.m_Buffer;					// make the default buffer the pending message
				clt->m_bufWrite.m_bDefaultBufferInUse = TRUE;
			}
			clt->m_bufWrite.m_pPendingHead = msg;
			memcpy(msg->msg, data, size);						// append the message 
			msg->size = size;
			msg = NULL;
		}
	}
	clt->m_bufWrite.m_csWriteBuf.Leave();
	
	if (next)
	{
		NetRecycleMsgBuf(net, next);
	}

	if (!msg)
	{
		return TRUE;											// added the message to pending
	}

	// get a new overlapped buffer
	overlap = NetGetFreeOverlap(net, IOWrite);
	ASSERT_GOTO(overlap, _error);
	overlap->m_idClient = NetGetClientId(clt);
	overlap->m_buf = (msg == clt->m_bufWrite.m_Buffer ? NULL : msg);
	overlap->m_wsabuf.buf = (char *)msg->msg;
	overlap->m_wsabuf.len = msg->size;

	if (!NetWSASend(net, clt, overlap))
	{
		goto _error;
	}

	return TRUE;

_error:
	TraceNetDebug("[%s] NetSendParallel() -- WSASend() failed. disconnecting client\n", clt->m_szName);
	NetDisconnectEx(net, clt, overlap, FALSE);
	
	return FALSE;
}


// ---------------------------------------------------------------------------
// DESC:	send a message to a local connection
// LOCK:	NETCLT soft lock
// NOTE:	how many memcopies does a local message do?
//			1) we create a BASE_MSG_STRUCT on the stack (initialization)
//			2) we fill it in
//			3) we translate it into a BIT_BUF_NET
//			4) we copy it to to the read buffer
//			5)   (if it's a mailbox message, we copy it to a MSG_BUF
//			6) we translate it back into a MSG_STRUCT
//			potentially, we could get rid of everything except 2 and 5
//			(except on servers & mac/xbox, which need to do 3 & 6)
//
//			how many memcopies does a remote message do?
//			1) we create a BASE_MSG_STRUCT on the stack (initialization)
//			2) we fill it in
//			3) we translate it into a BIT_BUF_NET
//			4) we copy it into a MSG_BUF
//			5) we read it into the read buffer on the remote machine
//			6)   (if it's a mailbox message, we copy it to a MSG_BUF)
//			7) we translate it back into a MSG_STRUCT
//			potentially, we could get rid of everything except 2 and 5
//			(except on servers & mac/xbox, which need to do 3 & 7)
//
//			the translation steps are the least necessary to get rid of
//			because they're required on the server (well not really,
//			but we'll have to declare all MSG_STRUCT members as 
//			_unaligned and them pack them)
//
//			but, if we do encryption, then we need to do the translation
//			anyway (note there's no reason to do encryption for local,
//			but then again, we're not concerned with optimization for
//			local)
//		
//			TODO:
//			given this, the main point where we could make an optimization
//			is in step 4 of remote, where we could the translation
//			directly into a MSG_BUF!  this should be fairly easy to do,
//			saving it for later.
// ---------------------------------------------------------------------------
static BOOL NetSendLocal(
	NETCOM * net,
	NETCLT * clt,
	const void * data,
	unsigned int size)
{
	if (!size)
	{
		return TRUE;
	}
	ASSERT_RETFALSE(data);

	NETCLT * dest = NetGetClientData(CRITSEC_FUNC_FILELINE(net, clt->m_idLocalConnection, TRUE));
	ASSERT_RETFALSE(dest);
	
	// copy the data to dest's read buffer
	ASSERT_GOTO(dest->m_bufRead.m_nLen == 0, _error);
	memcpy(dest->m_bufRead.m_Buffer, data, size);
	dest->m_bufRead.m_nLen = size;

	NetProcessReadBuffer(net, dest);
	NetReleaseClientData(CRITSEC_FUNC_FILELINE(net, dest));
	return TRUE;

_error:
	NetReleaseClientData(CRITSEC_FUNC_FILELINE(net, dest));
	return FALSE;
}


// ---------------------------------------------------------------------------
// DESC:	return pointer to overlap type label
// LOCK:	safe anywhere
// ---------------------------------------------------------------------------
#ifdef NET_TRACE_0
static const char * NetGetOverlapLabel(
	OVERLAPPEDEX * overlap)
{
	static const char * iolabel[] =
	{
		"unknown",
		"ioconnect",
		"ioaccept",
		"ioread",
		"iowrite",
		"iomulipointwrite",
		"iodisconnectex",
	};
	ASSERT(arrsize(iolabel) == IOLast);
	
	const char * label = iolabel[0];
	if (overlap && overlap->m_ioType > 0 && overlap->m_ioType < IOLast)
	{
		label = iolabel[overlap->m_ioType];
	}
	return label;
}
#endif


// ---------------------------------------------------------------------------
// DESC:	completion packet: IOConnect
// LOCK:	call from worker thread, NETCLT soft lock
// ---------------------------------------------------------------------------
static void WorkerProcessIOConnect(
	NETCOM * net,
	NETCLT * clt,
	OVERLAPPEDEX * & overlap)
{
	// TODO: setting the client state would be dangerous, except at this point
	// IOConnect is the only outstanding overlap request
	// HOWEVER what happens if we change the client state from the 
	// main thread? possible race condition?
	// still, possible race condition
	if (InterlockedCompareExchange(&clt->m_eClientState, NETCLIENT_STATE_CONNECTED, NETCLIENT_STATE_CONNECTING) != NETCLIENT_STATE_CONNECTING)
	{
		ASSERT(0);
	}

	TraceNetDebug("NetId 0x%08x [%s] connected to server\n", clt->m_id,clt->m_szName);

	// net read removes the client if it fails
	
	if(!clt->m_bCloseCalled && clt->m_fpConnectCallback) 
	{
		int addrlen = sizeof(struct sockaddr_storage);
		getsockname(clt->m_hSocket,(struct sockaddr*)&clt->m_addr,&addrlen);
		clt->m_fpConnectCallback( clt->m_id, NetGetCltUserData( clt ), CONNECT_STATUS_CONNECTED );
	}
	
	NetRead(net, clt, overlap);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void NetRefuseAccept(
	NETCOM * net,
	NETSRV * srv,
	NETCLT * clt )
{
	// post another accept
	NetAcceptEx(net, srv);
	// we decrement here because we never decremented for the IOAccept completion
	// (a successful NetAcceptEx incremented by one)
	ASSERT(InterlockedDecrement(&srv->m_nOutstandingAccepts) >= 0);

	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	NetSetSockopt(clt->m_hSocket, SOL_SOCKET, SO_LINGER, (const char *)&linger, sizeof(linger));

	closesocket(clt->m_hSocket);

	//	free the client
	clt->m_bWasAccepted = FALSE;
	clt->m_hSocket = INVALID_SOCKET;
	clt->m_ref.RefDec(CRITSEC_FUNCNOARGS_FILELINE());
}


// ---------------------------------------------------------------------------
// DESC:	completion packet: IOAccept
// LOCK:	call from worker thread, NETCLT soft lock
// ---------------------------------------------------------------------------
static void WorkerProcessIOAccept(
	NETCOM * net,
	NETCLT * clt,
	OVERLAPPEDEX * & overlap)
{
#if ENFORCE_NO_NAGLE
	sEnforceNonNagle(clt->m_hSocket);
#endif
	NETSRV * srv = clt->m_pServer;
    struct tcp_keepalive kv = {1,60*1000,10*1000}; //Keepalive time of 1 minute, with 10 second intervals
    DWORD bwrote;

	ASSERT(srv);
	if (!srv)
	{
#if !ISVERSION(RETAIL_VERSION)
		TraceError("WorkerProcessIOAccept() -- NULL server\n");
#endif
		goto _error;
	}
    if(srv->m_PerfInstance)
    {
        PERF_ADD64(srv->m_PerfInstance,CommonNetServer,connectionRate,1);
    }

	sockaddr_storage * plocal = 0, * premote = 0;
	int localsz = (int)srv->m_addrlen, remotesz = (int)srv->m_addrlen;
	ASSERT_GOTO(srv->m_WsockFn,_error);

	if (srv->m_WsockFn->NetGetAcceptExSockaddrs((PVOID)clt->m_bufRead.m_Buffer,
                                                 0,
                                                 (DWORD)(srv->m_addrlen + 16),
                                                 (DWORD)(srv->m_addrlen + 16), 

		(sockaddr**)&plocal, &localsz, (sockaddr**)&premote, &remotesz))
	{
		memcpy(&clt->m_addr, premote, remotesz);
	}

	if( NetCheckBanList( srv, (SOCKADDR_STORAGE *)premote ) )
	{
#if ISVERSION(SERVER_VERSION)
        TraceNetWarn("[%s] Refusing connection from: %!IPADDR!", 
                     srv->m_szName, 
                     ((struct sockaddr_in*)(premote))->sin_addr.s_addr);
#endif // ISVERSION(SERVER_VERSION)

		NetRefuseAccept( net, srv, clt );
		goto _error;
	}
#if ISVERSION(SERVER_VERSION)
    TraceNetDebug("Client connection from addr %!IPADDR! assigned netclientid 0x%08x\n", ((struct sockaddr_in*)(premote))->sin_addr.s_addr,clt->m_id);
#endif ISVERSION(SERVER_VERSION)

	if (!NetSetSockopt(clt->m_hSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&srv->m_hSocket, sizeof(SOCKET), TRUE))
	{
		ASSERT(0);
		goto _error;
	}
    if(WSAIoctl(clt->m_hSocket,SIO_KEEPALIVE_VALS,&kv,sizeof(kv),NULL,0,&bwrote,NULL,NULL) == SOCKET_ERROR)
    {
        TraceError("Could not set keepalive vals on netclientid 0x%08x\n",clt->m_id);
    }

	ASSERT(InterlockedCompareExchange(&clt->m_bCounted, 1, 0) == 0);
	InterlockedIncrement(&net->m_ClientTable.m_nClientCount);
	int count = InterlockedIncrement(&srv->m_nClientCount);
	REF(count);

	// still, possible race condition
	if (InterlockedCompareExchange(&clt->m_eClientState, NETCLIENT_STATE_CONNECTED, NETCLIENT_STATE_LISTENING) != NETCLIENT_STATE_LISTENING)
	{
		ASSERT(0);
	}

	TraceNetDebug("[%s] client count: %d.\n", srv->m_szName, count);
	PStrPrintf(clt->m_szName, NET_NAME_SIZE, "SVC_%02d", count);

    if(srv->m_PerfInstance)
    {
        PERF_ADD64(srv->m_PerfInstance,CommonNetServer,numClientsConnected,1);
    }

	if (srv->m_fpAcceptCallback)
	{
		srv->m_fpAcceptCallback(srv->m_pAcceptContext,NetGetClientId(clt),plocal,premote);
	}

	// post another accept
	NetAcceptEx(net, srv);
	// we decrement here because we never decremented for the IOAccept completion
	// (a successful NetAcceptEx incremented by one)
	ASSERT(InterlockedDecrement(&srv->m_nOutstandingAccepts) >= 0);

	// NetRead() disconnects the client on failure
	clt->m_bWasAccepted = TRUE;
	NetRead(net, clt, overlap);
 
	return;

_error:
	NetRecycleOverlap(net, overlap);
}


// ---------------------------------------------------------------------------
// DESC:	completion packet: IORead
//			this event occurs when we're done reading the data
// LOCK:	call from worker thread, NETCLT soft lock
// ---------------------------------------------------------------------------
static void WorkerProcessIORead(
	NETCOM * net,
	NETCLT * clt,
	OVERLAPPEDEX * & overlap,
	DWORD bytes)
{
	if (bytes == 0)
	{
		TraceNetDebug("[%s] WorkerProcessIORead() -- 0 bytes read, removing client (client:0x%08x)\n", clt->m_szName, clt->m_id);
		goto _error;
	}

	InterlockedIncrement(&clt->m_nMsgIn);
	InterlockedExchangeAdd(&clt->m_bBytesIn, bytes);

	clt->m_bufRead.m_nLen += bytes;
#if ISVERSION(SERVER_VERSION)
    if(clt->m_bNonBlocking && clt->m_bufRead.m_nLen < MAX_READ_BUFFER /2) 
    {
        int bytesRead = 0;
        int bytesToRead = MAX_READ_BUFFER - clt->m_bufRead.m_nLen;
        while(bytesRead != SOCKET_ERROR && bytesToRead > 0 )
        {
            bytesRead = recv(clt->m_hSocket,(char*)clt->m_bufRead.m_Buffer + clt->m_bufRead.m_nLen,bytesToRead,0);
            if(bytesRead == SOCKET_ERROR)
            {
                ASSERT_GOTO(WSAGetLastError() == WSAEWOULDBLOCK,_error);
                break;
            }
            if(bytesRead == 0)
            {
                break;
            }
            bytesToRead -= bytesRead;
            clt->m_bufRead.m_nLen += bytesRead;
            TraceExtraVerbose(TRACE_FLAG_COMMON_NET,"client %d added %d more bytes to buffer\n",clt->m_id,bytesRead);
        }

    }
#endif //SERVER_VERSION

	// process recv buffer
	if (!NetProcessReadBuffer(net, clt))
	{
#if !ISVERSION(RETAIL_VERSION)
		TraceError("[%s] WorkerProcessIORead() -- error processing read buffer, removing client (client:0x%08x)\n", clt->m_szName, clt->m_id);
#endif
		goto _error;
	}

	// NetRead() disconnects the client on failure
	NetRead(net, clt, overlap);
	return;

_error:
	NetDisconnectEx(net, clt, overlap);
}


// ---------------------------------------------------------------------------
// DESC:	handle overlap iotype == IOWrite
//			this event occurs when we're done writing the data
// LOCK:	call from worker thread, NETCLT soft lock
// ---------------------------------------------------------------------------
static void WorkerProcessIOWrite(
	NETCOM * net,
	NETCLT * clt,
	OVERLAPPEDEX * & overlap,
	DWORD bytes)
{
	if (bytes == 0)
	{
		TraceNetDebug("[%s] WorkerProcessIOWrite() -- 0 bytes written, removing client (client:0x%08x)\n", clt->m_szName, clt->m_id);
		goto _error;
	}

	ASSERT(bytes == overlap->m_wsabuf.len);

	InterlockedIncrement(&clt->m_nMsgOut);
	InterlockedExchangeAdd(&clt->m_bBytesOut, bytes);

#if NET_TRACE_MSG
	{
		char txt[1024];
		DbgPrintBuf(txt, 1024, (BYTE *)overlap->m_wsabuf.buf, overlap->m_wsabuf.len);
		trace("[%s] Send: %s\n", clt->m_szName, txt);
	}
#endif
	
	// change the client's send state
	if (clt->m_bSequentialWrites)
	{
		if (!NetSendPending(net, clt, overlap))
		{
			goto _error;
		}
	}
	else
	{
		if (overlap->m_buf == NULL)
		{
			clt->m_bufWrite.m_csWriteBuf.Enter();
			{
				ASSERT(clt->m_bufWrite.m_bDefaultBufferInUse == TRUE);		// free up my default write buffer
				clt->m_bufWrite.m_bDefaultBufferInUse = FALSE;
			}
			clt->m_bufWrite.m_csWriteBuf.Leave();
		}
		NetRecycleOverlap(net, overlap);
	}
	return;

_error:
	NetDisconnectEx(net, clt, overlap);
}



// ---------------------------------------------------------------------------
// DESC:	completion packet: IODisconnectEx
// LOCK:	call from worker thread, NETCLT soft lock
// ---------------------------------------------------------------------------
static void WorkerProcessIODisconnectEx(
	NETCOM * net,
	NETCLT * clt,
	OVERLAPPEDEX * & overlap)
{
	NetRecycleOverlap(net, overlap);

	TraceNetDebug("[%s] WorkerProcessIODisconnectEx() -- Closed connection from 0x%08x\n", clt->m_szName, clt->m_id);

	// allow decrement to 0, though this won't actually decrement to 0 because we're in a NETCLT softlock
	NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));

	// TODO: we may want to simply signal another thread to do this stuff
	NETSRV * srv = clt->m_pServer;
	if (srv)
	{
		// post another acceptex if there aren't enough
		// we increment to tell any other threads that we INTEND to increase the acceptex count
		LONG outstanding_accepts = InterlockedIncrement(&srv->m_nOutstandingAccepts);
		if (outstanding_accepts <= srv->m_nAcceptBacklog)
		{
			if (!NetAcceptEx(net, srv))
			{
				TraceNetDebug("WorkerProcessIODisconnectEx() -- post new AcceptEx failed!\n");
			}
		}
		// undo the previous increment since NetAcceptEx() will have done the actual incrementing on success
		ASSERT(InterlockedDecrement(&srv->m_nOutstandingAccepts) >= 0);
        if(srv->m_PerfInstance)
        {
            PERF_ADD64(srv->m_PerfInstance,CommonNetServer,numClientsConnected,-1);
        }
	}
	else if(!clt->m_bCloseCalled && clt->m_fpConnectCallback)//TODO change this to DisconnectCallback
	{
		clt->m_fpConnectCallback( clt->m_id, NetGetCltUserData( clt ), CONNECT_STATUS_DISCONNECTED );
	}
}


// ---------------------------------------------------------------------------
// DESC:	worker thread process completion packet GetQueuedCompletionStatus
// LOCK:	call from worker thread obviously, NETCLT soft lock
// ---------------------------------------------------------------------------
static BOOL WorkerThreadProcess(
	NETCOM * net,
	NETCLT * clt,
	DWORD bytes,
	OVERLAPPEDEX * & overlap)
{
	NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));

	switch (overlap->m_ioType)
	{
	case IOConnect:
		WorkerProcessIOConnect(net, clt, overlap);
		break;
	case IOAccept:
		WorkerProcessIOAccept(net, clt, overlap);
		break;
	case IORead:
		WorkerProcessIORead(net, clt, overlap, bytes);
		break;
	case IOWrite:
		InterlockedExchangeAdd(&clt->m_nOutstandingBytes, 0 - overlap->m_wsabuf.len);
        if(clt->m_pServer && clt->m_pServer->m_PerfInstance)
        {
            PERF_ADD64(clt->m_pServer->m_PerfInstance,CommonNetServer,numBytesOutstanding,(signed) (0 - overlap->m_wsabuf.len));
            PERF_ADD64(clt->m_pServer->m_PerfInstance,CommonNetServer,numSendsPending,-1);
        }
		WorkerProcessIOWrite(net, clt, overlap, bytes);
		break;
	case IODisconnectEx:
		WorkerProcessIODisconnectEx(net, clt, overlap);
		break;
	default:
#if NET_TRACE_ERR
		TraceError("GetQueuedCompletionStatus() -- invalid iotype in overlap\n");
#endif
		ASSERT(0);
		return FALSE;
	}

	return TRUE;
}




// ---------------------------------------------------------------------------
// DESC:	worker thread process error from GetQueuedCompletionStatus
// LOCK:	call from worker thread obviously, NETCLT soft lock
// ---------------------------------------------------------------------------
static BOOL WorkerThreadError(
	NETCOM * net,
	NETCLT * clt,
	DWORD io_error,
	OVERLAPPEDEX * & overlap)
{
	BOOL retval = TRUE;

#if NET_TRACE_0
	static volatile long lErrorOperationAbortedCount = 0;
	if (io_error == ERROR_OPERATION_ABORTED)
	{
		static const long lErrorOperationAbortedFactor = 200;
		long count = InterlockedIncrement(&lErrorOperationAbortedCount);
		if (count % lErrorOperationAbortedFactor == 1)
		{
#if ISVERSION(DEBUG_VERSION)
			TraceNetDebug("[%s] %s -- ERROR: %d  COUNT: %d  TICK: %d", NetCltGetName(clt), NetGetOverlapLabel(overlap), io_error, count, overlap->m_dwTickCount);
#else
			TraceNetDebug("[%s] %s -- ERROR: %d  COUNT: %d", NetCltGetName(clt), NetGetOverlapLabel(overlap), io_error, count);
#endif
            
#if ISVERSION(SERVER_VERSION)
            TraceNetDebug(" Address : %!IPADDR!\n",((struct sockaddr_in*)&clt->m_addr)->sin_addr.s_addr);
#else
            TraceNetDebug("\n");
#endif //!SERVER_VERSION
		}
	}
	else
	{
		char error_str[2048];
		TraceExtraVerbose(TRACE_FLAG_COMMON_NET, "[%s] %s -- ERROR: %s (%d)", NetCltGetName(clt), NetGetOverlapLabel(overlap),(((GetErrorString(io_error, error_str, 2048)))), io_error);
#if ISVERSION(SERVER_VERSION)
        TraceExtraVerbose(TRACE_FLAG_COMMON_NET, " Address : %!IPADDR!\n",((struct sockaddr_in*)&clt->m_addr)->sin_addr.s_addr);
#else
        TraceError("\n");
#endif //!SERVER_VERSION
	}
#endif //NET_0

	switch (io_error)
	{
	case WAIT_TIMEOUT:
		NetRecycleOverlap(net, overlap);
		ASSERT(0);						// we don't currently call GetQueuedCompletionStatus() with a timeout, so should never get here

	case ERROR_OPERATION_ABORTED:		// probably socket associated with completion port closed
	case ERROR_NETNAME_DELETED:
		break;

	case ERROR_DUP_NAME:				// this usuall happens on connect, if the address is in use
	default:
		{
#if !ISVERSION(RETAIL_VERSION)
			char error_str[2048];
			TraceExtraVerbose(TRACE_FLAG_COMMON_NET, 
                    "GetQueuedCompletionStatus() -- error: %s iostatus %p", 
                    GetErrorString(io_error, error_str, 2048),
                    (void*)(overlap->m_Overlapped.Internal)
                    );
#endif
		}
		break;
	}

	if (!clt)
	{
		NetRecycleOverlap(net, overlap);
		ASSERT_RETFALSE(0);
	}

	NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));

	switch (overlap->m_ioType)
	{
	case IOConnect:
		NetRecycleOverlap(net, overlap);
		if (InterlockedCompareExchange(&clt->m_eClientState, NETCLIENT_STATE_CONNECTFAILED, NETCLIENT_STATE_CONNECTING) != NETCLIENT_STATE_CONNECTING)
		{
			ASSERT(0);
		}
		if (clt->m_pServer)
		{
			NetDisconnectEx(net, clt, overlap);
		}
		else
        {
            if(!clt->m_bCloseCalled && clt->m_fpConnectCallback )
            {
                clt->m_fpConnectCallback( clt->m_id, NetGetCltUserData( clt ), CONNECT_STATUS_CONNECTFAILED );
            }
            //NetClientSubRef(CRITSEC_FUNC_FILELINE(clt)); not needed here, I don't think. -amol 10/18/06
        }
		break;

	case IOAccept:
		{
			NetRecycleOverlap(net, overlap);
			NETSRV * srv = clt->m_pServer;
			ASSERT_BREAK(srv);
            /*
			ASSERT(srv->m_eServerState == SERVER_STATE_SHUTDOWN);
            */
			ASSERT(InterlockedDecrement(&srv->m_nOutstandingAccepts) >= 0);
			NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));
		}
		break;

	case IODisconnectEx:
		WorkerProcessIODisconnectEx(net, clt, overlap);		// call anyway
		break;
	case IOWrite:
		InterlockedExchangeAdd(&clt->m_nOutstandingBytes,0 - overlap->m_wsabuf.len);
        if(clt->m_pServer && clt->m_pServer->m_PerfInstance)
        {
            PERF_ADD64(clt->m_pServer->m_PerfInstance,CommonNetServer,numBytesOutstanding,(signed) (0 - overlap->m_wsabuf.len));
            PERF_ADD64(clt->m_pServer->m_PerfInstance,CommonNetServer,numSendsPending,-1);
        }
		//FALLTHROUGH
    default:
		NetDisconnectEx(net, clt, overlap);
		break;
	}

	return retval;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void PreCreateBuffers(
	NETCOM * net)
{
	MSG_BUF * arrayMessages[PER_THREAD_PRECREATE_COUNT];
	OVERLAPPEDEX * arrayOverlaps[PER_THREAD_PRECREATE_COUNT];

    for (unsigned int ii = 0; ii < PER_THREAD_PRECREATE_COUNT; ++ii) 
	{
        arrayMessages[ii] = NetGetFreeMsgBuf(net);
        arrayOverlaps[ii] = NetGetFreeOverlap(net, IOWrite);
	}

    for (unsigned int ii = 0; ii < PER_THREAD_PRECREATE_COUNT; ++ii) 
	{
        NetRecycleMsgBuf(net, arrayMessages[ii]);
        NetRecycleOverlap(net, arrayOverlaps[ii]);
	}
}


// ---------------------------------------------------------------------------
// DESC:	worker thread
// ---------------------------------------------------------------------------
DWORD WINAPI WorkerThread(
	LPVOID param)    
{
	NET_WORKER_DATA * workerdata = (NET_WORKER_DATA *)param;
	ASSERT_RETZERO(workerdata);

	NETCOM * net = workerdata->m_Net;

	HANDLE hCompletionPort = workerdata->m_hWorkerCompletionPort;

    TraceNetDebug("WorkerThread %d assigned completion port %p\n", workerdata->m_Idx, hCompletionPort);

	InterlockedIncrement(&net->m_nBusyThreads);

	char thread_name[256];
	WCHAR uthread_name[256];
	int thread_id = InterlockedIncrement(&net->m_nCurrentThreads);
	PStrPrintf(thread_name, 256, "net_%02d", thread_id);
	PStrPrintf(uthread_name, 256, L"net_%02d", thread_id);

	PreCreateBuffers(net);

	InterlockedIncrement(&workerdata->m_PoolInitialized);

#if NET_DEBUG
	SetThreadName(thread_name);
#endif
#if NET_TRACE_1
	TraceNetDebug("begin worker thread [%s]\n", thread_name);
#endif
	PERF_INSTANCE(CommonNetWorker) *pWorkerThreadPerfInstance = PERF_GET_INSTANCE(CommonNetWorker,uthread_name);

	while (TRUE)
	{
		InterlockedDecrement(&net->m_nBusyThreads);					// thread is block waiting for io completion

		ULONG_PTR completion_key;										// get a completed io request
		OVERLAPPEDEX * overlap = NULL;
		DWORD dwIOSize;
		DWORD dwIOError = 0;
		BOOL bIORet = GetQueuedCompletionStatus(hCompletionPort, &dwIOSize, (PULONG_PTR)&completion_key, (LPOVERLAPPED *)&overlap, INFINITE);
		if (!bIORet)
		{
			dwIOError = GetLastError();
		}
		if(pWorkerThreadPerfInstance)
		{
			PERF_ADD64(pWorkerThreadPerfInstance,CommonNetWorker,schedulingRate,1);
			if(overlap)
			{
				switch( overlap->m_ioType)
				{
					case IOWrite:
						PERF_ADD64(pWorkerThreadPerfInstance,CommonNetWorker,sendCompletionRate,1);
						break;
					case IORead:
						PERF_ADD64(pWorkerThreadPerfInstance,CommonNetWorker,recvCompletionRate,1);
						break;
					default:
						break;
				}
			}
		}
 		//int nBusyThreads = InterlockedIncrement(&net->m_nBusyThreads);

		if (overlap == NULL)
		{
			ASSERT(net->m_eState == SERVER_STATE_SHUTDOWN);
			break;													// exit thread
		}

		ASSERT(overlap->m_idClient != INVALID_NETCLIENTID);
		NETCLT * clt = NetGetClientData(CRITSEC_FUNC_FILELINE(net, overlap->m_idClient, TRUE));
		if (!clt)
		{
			if (WorkerThreadError(net, clt, dwIOError,  overlap) == FALSE)
			{
				break;												// exit thread
			}
			continue;
		}

        if (overlap->m_ioType != IOMultipointWrite)
        {
            ASSERT(InterlockedDecrement(&clt->m_nOutstandingOverlaps[overlap->m_ioType]) >= 0);
            ASSERT(clt->m_idLocalConnection == INVALID_NETCLIENTID);
        }

		BOOL bExit = FALSE;
		ONCE
		{
			if (!bIORet)
			{
				if (WorkerThreadError(net, clt, dwIOError,  overlap) == FALSE)
				{
					bExit = TRUE;									// exit thread
				}
				break;												
			}


			if (WorkerThreadProcess(net, clt, dwIOSize,  overlap) == FALSE)
			{
				bExit = TRUE;										// exit thread
				break;												
			}
		}
		NetReleaseClientData(CRITSEC_FUNC_FILELINE(net, clt));

		if (bExit)
		{
			break;													// exit thread
		}
	}
	PERF_FREE_INSTANCE(pWorkerThreadPerfInstance);

#if NET_TRACE_1
	TraceNetDebug("exit worker thread [%s]\n", thread_name);
#endif

	InterlockedDecrement(&net->m_nCurrentThreads);
	InterlockedDecrement(&net->m_nBusyThreads);

	return 0;
}


// ---------------------------------------------------------------------------
// DESC:	spawn a worker thread
// LOCK:	the state check isn't determinate, and would be a race condition
//			except that this function is only called from two places,
//			either initialization or from within a NETCLT softlock
// ---------------------------------------------------------------------------
static HANDLE NetSpawnWorkerThread(
	NET_WORKER_DATA * workerdata)
{
	ASSERT_RETNULL(workerdata);
    NETCOM * net = workerdata->m_Net;
	ASSERT_RETNULL(net);
	if (net->m_eState == SERVER_STATE_SHUTDOWN)		
	{
		return FALSE;
	}

	InterlockedIncrement(&net->m_nThreadPoolCount);
	DWORD dwThreadId;
	HANDLE hWorker = (HANDLE)CreateThread(NULL,	0, WorkerThread, workerdata, 0, &dwThreadId);
    if (hWorker == NULL)
	{
		ASSERT_RETNULL(0);
    }
	SetThreadPriority(hWorker, WORKER_PRIORITY);
	return hWorker;
}
// ---------------------------------------------------------------------------
// DESC:	Fake ConnecteEx threadproc
// ---------------------------------------------------------------------------
DWORD WINAPI FakeConnectExThread(
	LPVOID param)    
{
	ASSERT_RETVAL(param,1);
	NETCOM *net = (NETCOM*)param;

	BOOL bDone = FALSE;

	while(!bDone)
	{
		ULONG_PTR key;
		DWORD bytesRead;
		OVERLAPPEDEX *pOverEx;
		BOOL bRet = GetQueuedCompletionStatus(net->m_hFakeConnectExCompletionPort,&bytesRead,&key,(OVERLAPPED**)&pOverEx,INFINITE);

		if(bRet == FALSE)
		{
			TraceError("GetQueuedCompletionStatus failed on fake connect: %d\n",GetLastError());
			break;
		}
		NETCLT * clt = NetGetClientData(CRITSEC_FUNC_FILELINE(net, pOverEx->m_idClient, TRUE));
		HANDLE hEvent = pOverEx->m_Overlapped.hEvent;
		ONCE
		{
			if(WaitForSingleObject(hEvent,INFINITE) != WAIT_OBJECT_0)
			{
				DWORD err = GetLastError();
				TraceError("WaitForSingleObject failed. :%d\n",err);
				ASSERT(WorkerThreadError(net, clt, err,  pOverEx) == TRUE);
				break;
			}
			WSANETWORKEVENTS netevents;
			if(WSAEnumNetworkEvents(clt->m_hSocket,pOverEx->m_Overlapped.hEvent,&netevents) == SOCKET_ERROR)
			{
				DWORD err = GetLastError();
				TraceError("WSAEnumNetworkEvents failed. :%d\n",err);
				ASSERT(WorkerThreadError(net, clt, err,  pOverEx) == TRUE);
				break;
			}
			if(netevents.iErrorCode[FD_CONNECT_BIT] != 0)
			{
				TraceError("connect failed. :%d\n",netevents.iErrorCode[FD_CONNECT_BIT]);
				ASSERT(WorkerThreadError(net, clt, netevents.iErrorCode[FD_CONNECT_BIT],  pOverEx) == TRUE);
				break;
			}
			WorkerThreadProcess(net,clt,0,pOverEx);
		}
		CloseHandle(hEvent);
		NetReleaseClientData(CRITSEC_FUNC_FILELINE(net, clt));
	}

	return 0;
}

// ---------------------------------------------------------------------------
// DESC:	spawn a fake connectex thread
// ---------------------------------------------------------------------------
static BOOL sNetSpawnFakeConnectExThread(NETCOM *net)
{
	ASSERT_RETNULL(net);
	if (net->m_eState == SERVER_STATE_SHUTDOWN)		
	{
		return FALSE;
	}
	DWORD dwThreadId;
	HANDLE hWorker = (HANDLE)CreateThread(NULL,	0, FakeConnectExThread, net, 0, &dwThreadId);
    ASSERT_RETFALSE(hWorker);

	return TRUE;
	
}

		
// ---------------------------------------------------------------------------
// DESC:	free net memory
// LOCK:	call from main thread only
// ---------------------------------------------------------------------------
static void sNetFree(
	NETCOM * & net)
{
	if (!net)
	{
		return;
	}
	if(net->m_hWorkerCompletionPorts) 
		for (DWORD di = 0; di < net->m_nNumberOfWorkerCompletionPorts; di++)
	{
		if (net->m_hWorkerCompletionPorts[di] != NULL)
		{
			CloseHandle(net->m_hWorkerCompletionPorts[di]);
			net->m_hWorkerCompletionPorts[di] = NULL;
		}
	}

	NetFreeClientTable(net);
    NetFreePerThreadBufferPools(net);

	for (int ii = 0; ii < 2; ii++)
	{
		NetFreeSocketPool(net, net->m_WsockFn[ii].m_SocketPool);
	}


#if NET_TRACE_1
	TraceNetDebug("[%s] Exit\n", net->m_szName);
#endif

	FREE(net->mempool, net);
	net = NULL;
}


// ---------------------------------------------------------------------------
// EXPORTED FUNCTIONS
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// DESC:	initialize network
// LOCK:	call from main thread only
// ---------------------------------------------------------------------------
NETCOM * NetInit(
	const net_setup & setup)
{
	ASSERT_RETNULL(NetWinsockInit());

	NETCOM * net = (NETCOM *)MALLOCZ(setup.mempool, sizeof(NETCOM));
	ASSERT_RETNULL(net);
	net->mempool = setup.mempool;

	if (setup.name)
	{
		PStrCopy(net->m_szName, setup.name, NET_NAME_SIZE);
	}
	else
	{
		PStrCopy(net->m_szName, "NET", NET_NAME_SIZE);
	}

    // setup cpu metrics
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
	net->m_nThreadPoolMin = systemInfo.dwNumberOfProcessors * HUERISTIC_VALUE;
	net->m_nThreadPoolMax = net->m_nThreadPoolMin * 2;
	ASSERT(net->m_nThreadPoolMax <= MAX_WORKER_THREADS);

	if (setup.num_completion_ports)
	{
	    net->m_nNumberOfWorkerCompletionPorts = min((long)systemInfo.dwNumberOfProcessors, setup.num_completion_ports);
	}
	else
	{
	    net->m_nNumberOfWorkerCompletionPorts = 1;
	}
	TraceNetDebug("NET: Creating %d completion ports\n", net->m_nNumberOfWorkerCompletionPorts);

	net->m_nUserDataSize = setup.user_data_size;

	// initialize server array
	for (int ii = 0; ii < MAX_SERVERS; ii++)
	{
		net->m_Servers[ii].m_hSocket = INVALID_SOCKET;
	}

	// initialize client array
	ASSERT_GOTO(NetInitClientTable(net, setup), _error);

	ASSERT_RETFALSE((net->m_hWorkerCompletionPorts = 
					(HANDLE*)MALLOCZ(net->mempool,net->m_nNumberOfWorkerCompletionPorts*sizeof(HANDLE)))!= NULL);

    for (DWORD di = 0; di < net->m_nNumberOfWorkerCompletionPorts;di++)
    {
        // create the completion port
        net->m_hWorkerCompletionPorts[di] = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)0, 0);
        if (net->m_hWorkerCompletionPorts[di] == NULL) 
        {
            TraceError("CreateIOCompletionPort %d failed: %d\n",di,GetLastError());
            goto _error;
        }

    }
	net->m_hFakeConnectExCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,NULL,0);
	if(net->m_hFakeConnectExCompletionPort == NULL)
	{
		TraceError("CreateIOCompletionPort failed for fake connectex port: %d\n",GetLastError());
		goto _error;
	}
#if !ISVERSION(SERVER_VERSION)
	if(!sNetSpawnFakeConnectExThread(net))
	{
		TraceError(" sNetSpawnFakeConnectExThread for fake connectex port\n");
		goto _error;
	}
#endif

	ASSERT_GOTO(net->m_PTBPCollection.Init(net->mempool), _error);

	// set up worker threads
    for (unsigned int ii = 0; ii < (unsigned int)net->m_nThreadPoolMin; ii++) 
	{
		net->m_WorkerData[ii].m_Net = net;
		net->m_WorkerData[ii].m_Idx = ii;
		net->m_WorkerData[ii].m_hWorkerCompletionPort = net->m_hWorkerCompletionPorts[ii % net->m_nNumberOfWorkerCompletionPorts];
		net->m_WorkerData[ii].m_PoolInitialized = 0;
		net->m_WorkerData[ii].m_hHandle = NetSpawnWorkerThread(&net->m_WorkerData[ii]);

		ASSERT_GOTO(net->m_WorkerData[ii].m_hHandle, _error);
        while(InterlockedCompareExchange(&net->m_WorkerData[ii].m_PoolInitialized, 0, 0) == 0)
        {
            Sleep(100);
        }
    }

	net->m_eState = SERVER_STATE_READY;

#if NET_TRACE_1
	TraceNetDebug("[%s] NetSrvInit(): complete\n", net->m_szName);
#endif

	return net;

_error:
	sNetFree(net);
    return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void NetFreeBanList(
	IPADDRDEPOT * list )
{
	if(!list)
		return;

	MEMORYPOOL * pool = list->GetPool();
	list->Free();
	FREE(pool,list);
}


// ---------------------------------------------------------------------------
// DESC:	free a server
// LOCK:	call from main thread only
// ---------------------------------------------------------------------------
static void NetFreeServer(
	NETCOM * net,
	NETSRV * & srv)
{
	if (!srv)
	{
		return;
	}
	if (srv->m_hSocket != INVALID_SOCKET)
	{
		NetCloseSocket(srv->m_hSocket);
	}
    if(srv->m_PerfInstance)
    {
        PERF_FREE_INSTANCE(srv->m_PerfInstance);
    }

	srv->m_banListLock.Free();
	NetFreeBanList(srv->m_banList);

	NetRecycleServer(net, srv);
}


// ---------------------------------------------------------------------------
// DESC:	startup a server
// LOCK:	call from main thread only
// ---------------------------------------------------------------------------
static BOOL NetInitServer(
	NETCOM * net,
	NETSRV * srv,
	const net_srv_setup & setup)
{ 
	if (!setup.local)
	{
		PStrCopy(srv->m_szAddr, setup.local_addr, MAX_NET_ADDRESS_LEN);
		srv->m_nPort = setup.local_port;

		// create a listener for new connections
		srv->m_hSocket = NetWSASocket(srv->m_nFamily, srv->m_nSockType, srv->m_nProtocol);
		ASSERT_RETFALSE(srv->m_hSocket != INVALID_SOCKET);

		// link to a WSOCK_EXTENDED
		srv->m_WsockFn = NetGetWsockEx(net, srv->m_hSocket, srv->m_nFamily, srv->m_nSockType, srv->m_nProtocol);
		ASSERT_RETFALSE(srv->m_WsockFn);

        if((unsigned)setup.completion_port_index < (unsigned)net->m_nNumberOfWorkerCompletionPorts)
        {
            srv->m_nWorkerCompletionPortIndex = setup.completion_port_index;
        }
        else
        {
            srv->m_nWorkerCompletionPortIndex = 0;
        }
        TraceNetDebug("SRV %p assigned completion port index :srv->m_nWorkerCompletionPortIndex %d\n",
                        srv, srv->m_nWorkerCompletionPortIndex );

		// associate the new socket with send completion port
		ASSERT_RETFALSE(net->m_hWorkerCompletionPorts[srv->m_nWorkerCompletionPortIndex]  == 
                            CreateIoCompletionPort((HANDLE)srv->m_hSocket, 
                                    net->m_hWorkerCompletionPorts[srv->m_nWorkerCompletionPortIndex], 
                                    (ULONG_PTR)0, 0));

		// bind the socket to the local address
		ASSERTX_RETFALSE(
			NetBind(srv->m_hSocket, (const sockaddr *)&srv->m_addr, srv->m_addrlen),
			"Unable to bind networking socket to IP address specified as the local ip. Possibly wrong IP givin as local IP." );

		// set up to listen
		if (srv->m_nSockType == SOCK_STREAM)
		{
            ASSERT_RETFALSE(NetSetSockopt(srv->m_hSocket, IPPROTO_TCP, TCP_NODELAY, TRUE));
			ASSERT_RETFALSE(NetListen(srv->m_hSocket, setup.listen_backlog));
		}

		// precreate sockets
		ASSERT_RETFALSE(NetPrecreateSockets(net, srv->m_nWorkerCompletionPortIndex,srv->m_WsockFn->m_SocketPool, setup.socket_precreate_count));
	}

    srv->m_pAcceptContext = setup.accept_context;
	srv->m_fpAcceptCallback = setup.fp_AcceptCallback;

	srv->m_fpDisconnectCallback = setup.fp_DisconnectCallback;
	srv->m_fpRemoveCallback = setup.fp_RemoveCallback;

    srv->m_fpDirectReadCallback = setup.fp_DirectReadCallback;

	InterlockedIncrement(&net->m_nActiveServers);
	if (InterlockedCompareExchange(&srv->m_eServerState, SERVER_STATE_READY, SERVER_STATE_NONE) != SERVER_STATE_NONE)
	{
		InterlockedDecrement(&net->m_nActiveServers);
		ASSERT_RETFALSE(0);
	}
	if(setup.disable_send_buffer)
	{
		int bufsize = 0;
		(void)setsockopt(srv->m_hSocket,SOL_SOCKET,SO_SNDBUF,(char*)&bufsize,sizeof(bufsize));
	}
	if(setup.non_blocking_socket)
	{
		u_long nonblocking = 1;
		ASSERT_RETFALSE(ioctlsocket(srv->m_hSocket,FIONBIO,&nonblocking) != SOCKET_ERROR);
        srv->m_bNonBlocking = TRUE;
	}
    else
    {
        srv->m_bNonBlocking = FALSE;
    }
	if(setup.max_pending_sends)
	{
		srv->m_nMaxOutstandingSends = min(setup.max_pending_sends,MAX_SENDS_PERMITTED);
	}
	if(setup.max_pending_bytes)
	{
		srv->m_nMaxOutstandingBytes = min(setup.max_pending_bytes,MAX_SEND_BYTES_PERMITTED);
	}

	if (setup.local)
	{
#if NET_TRACE_1
		TraceNetDebug("Server [%s] Initialized.  Size of NETCLT = %d.  LOCAL SERVER ONLY\n", srv->m_szName, sizeof(NETCLT));
#endif
	}
	else	// set up accepts
	{
		srv->m_nAcceptBacklog = MAX((LONG)setup.accept_backlog, (LONG)1);
		for (unsigned int ii = 0; ii < (unsigned int)srv->m_nAcceptBacklog; ii++)
		{
			ASSERT_RETFALSE(NetAcceptEx(net, srv));
		}

#if NET_TRACE_1
		TraceNetDebug("Server [%s] Initialized.  Size of NETCLT = %d.  Listening on: %s port %d (socket:%d)\n", 
			srv->m_szName, sizeof(NETCLT), srv->m_szAddr, srv->m_nPort, (int)srv->m_hSocket);
#endif
	}

	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	create & initialize a server
// LOCK:	only call this from the main thread of the app!
// ---------------------------------------------------------------------------
NETSRV * NetCreateServer(
	NETCOM * net,
	const net_srv_setup & setup)
{
	ASSERT_RETFALSE(setup.local || setup.local_addr);

	NETSRV * srv = NetGetFreeServer(net);
	ASSERT_RETNULL(srv);

	PStrCopy(srv->m_szName, setup.name, NET_NAME_SIZE);

	srv->m_bLocal = setup.local;
	if (!setup.local)
	{
		ASSERT_GOTO(NetGetAddr(setup.local_addr, DEFAULT_FAMILY, setup.local_port, AI_NUMERICHOST | AI_PASSIVE, &srv->m_addr, &srv->m_addrlen, &srv->m_nFamily, &srv->m_nSockType, &srv->m_nProtocol), _error);
	}

	// setup command table & mail slot
    if(setup.fp_DirectReadCallback == NULL)
    {
        ASSERT(setup.cmd_tbl);
    }
	srv->m_CmdTbl = setup.cmd_tbl;
	srv->m_mailslot = setup.mailslot;
	srv->m_nNagle = setup.nagle_value;						// these are ignored if local, but don't matter
	srv->m_bSequentialWrites = setup.sequential_writes;		// these are ignored if local, but don't matter
    srv->m_bDontLockOnSend = setup.dont_enforce_thread_safe_writes;
	srv->m_fpNetSend = NetSelectSendFunction(setup.local, setup.nagle_value, setup.sequential_writes);
	srv->m_fpInitUserData = setup.fp_InitUserData;
	srv->m_banList = NULL;
	srv->m_banListLock.Init();

    WCHAR unicode_name[NET_NAME_SIZE+ 1];
    ASSERT_RETNULL(PStrCvt(unicode_name,setup.name,ARRAYSIZE(unicode_name)));
    srv->m_PerfInstance = PERF_GET_INSTANCE(CommonNetServer,unicode_name);

	ASSERT_GOTO(NetInitServer(net, srv, setup), _error);

	return srv;

_error:
	NetFreeServer(net, srv);
	return NULL;
}


// ---------------------------------------------------------------------------
// DESC:	iterate clients connected to a given server & disconnect them
// LOCK:	call from anywhere!
// ---------------------------------------------------------------------------
static void NetDisconnectAll(
	NETCOM * net,
	NETSRV * srv)
{
	CClientTable & tbl = net->m_ClientTable;

	for (unsigned int ii = 0; ii < tbl.m_nArraySize; ii++)
	{
		NETCLT * clt = NetGetClientDataByIndex(CRITSEC_FUNC_FILELINE(net, ii));
		if (clt)
		{
			if (clt->m_pServer == srv)
			{
				NetCloseClient(net, NetGetClientId(clt), FALSE);
			}
			NetReleaseClientData(CRITSEC_FUNC_FILELINE(net, clt));
		}
	}
}


// ---------------------------------------------------------------------------
// DESC:	iterate clients connected to a given server & disconnect them
// LOCK:	call from anywhere!
// ---------------------------------------------------------------------------
static void NetDebugClose(
	NETCOM * net,
	NETSRV * srv)
{
	CClientTable & tbl = net->m_ClientTable;

	for (unsigned int ii = 0; ii < tbl.m_nArraySize; ii++)
	{
		NETCLT * clt = NetGetClientDataByIndex(CRITSEC_FUNC_FILELINE(net, ii));
		if (clt)
		{
			if (clt->m_pServer == srv && clt->m_eClientState != NETCLIENT_STATE_NONE)
			{
				trace("whoa!\n");
			}
			NetReleaseClientData(CRITSEC_FUNC_FILELINE(net, clt));
		}
	}
}


// ---------------------------------------------------------------------------
// DESC:	close & release a server (blocks)
// LOCK:	call from main thread only
// ---------------------------------------------------------------------------
void NetCloseServer(
	NETCOM * net,
	NETSRV * srv)
{
	ASSERT_RETURN(net);
	if (!srv || srv->net == NULL)
	{
		return;
	}
	if (InterlockedCompareExchange(&srv->m_eServerState, SERVER_STATE_SHUTDOWN, SERVER_STATE_READY) != SERVER_STATE_READY)
	{
		return;
	}
	if (srv->m_hSocket == INVALID_SOCKET)
	{
		NetFreeServer(net, srv);
		return;
	}
	srv->m_nAcceptBacklog = 0;

	// close the listening socket
	NetCloseSocket(srv->m_hSocket);
	
	// wait until all listeners exit
	// TODO: make it an event?
	{
		unsigned int wait_count = 0;
		while (srv->m_nOutstandingAccepts > 0 && wait_count < 1200)		// wait no more than 600 seconds
		{
			Sleep(50);
			wait_count++;
		}
		ASSERT(wait_count < 1200);
	}

	// close all active client sockets
	NetDisconnectAll(net, srv);

	// wait until all clients exit
	while (srv->m_nClientCount > 0)
	{
 		Sleep(50);
	}

	NetFreeServer(net, srv);
	InterlockedDecrement(&net->m_nActiveServers);
}


// ---------------------------------------------------------------------------
// DESC:	free a client
// LOCK:	call from NetInitClient() only
// ---------------------------------------------------------------------------
static void NetFreeClient(
	NETCOM * net,
	NETCLT * & clt)
{
	if (!clt)
	{
		return;
	}
	if (clt->m_hSocket != INVALID_SOCKET)
	{
		if (clt->m_WsockFn)
		{
			NetRecycleSocket(net, clt->m_WsockFn->m_SocketPool, clt->m_hSocket);
		}
		else
		{
			NetCloseSocket(clt->m_hSocket);
		}
	}
	clt->m_ref.RefDec(CRITSEC_FUNCNOARGS_FILELINE());
	clt->m_ref.Free();
	NetRecycleClient(net, clt);
}


// ---------------------------------------------------------------------------
// DESC:	startup a client
// LOCK:	call from NetCreateClient() only
// ---------------------------------------------------------------------------
static BOOL NetInitClient(
	NETCOM * net,
	NETCLT * clt,
	const net_clt_setup & setup,
	const sockaddr_storage & local_addr,
	unsigned int local_addrlen,
	const sockaddr_storage & remote_addr,
	unsigned int remote_addrlen,
	unsigned int family,
	unsigned int socktype,
	unsigned int protocol)
{
	OVERLAPPEDEX * overlap = NULL;

	// link to a WSOCK_EXTENDED
	clt->m_WsockFn = NetGetWsockEx(net, INVALID_SOCKET, family, socktype, protocol);
	ASSERT_RETFALSE(clt->m_WsockFn);

	clt->m_hSocket = NetGetSocket(net, setup.completion_port_index,clt->m_WsockFn->m_SocketPool);
	ASSERT_RETFALSE(clt->m_hSocket != INVALID_SOCKET);

	if (setup.socket_precreate_count > 0)
	{
		ASSERT_GOTO(NetPrecreateSockets(net,setup.completion_port_index, clt->m_WsockFn->m_SocketPool, setup.socket_precreate_count), _error);
	}

	// bind the socket to the local port
	if (!NetBind(clt->m_hSocket, (const sockaddr *)&local_addr, local_addrlen))
	{
		goto _error;
	}
	if(setup.disable_send_buffer)
	{
		int bufsize = 0;
		(void)setsockopt(clt->m_hSocket,SOL_SOCKET,SO_SNDBUF,(char*)&bufsize,sizeof(bufsize));
	}
	if(setup.non_blocking_socket)
	{
		u_long nonblocking = 1;
		ASSERT_GOTO(ioctlsocket(clt->m_hSocket,FIONBIO,&nonblocking) != SOCKET_ERROR,_error)
        clt->m_bNonBlocking = TRUE;
	}
    else
    {
        clt->m_bNonBlocking = FALSE;
    }
    

	// turn off nagle buffering if m_nNagle isn't NAGLE_DEFAULT
	NetSetSockopt(clt->m_hSocket, IPPROTO_TCP, TCP_NODELAY, clt->m_nNagle != NAGLE_DEFAULT);

	TraceNetDebug("Attempt to connect: %s port %d\n", setup.remote_addr, setup.remote_port);

	overlap = NetGetFreeOverlap(net, IOConnect);
	ASSERT_GOTO(overlap, _error);
	overlap->m_idClient = NetGetClientId(clt);

	InterlockedIncrement(&clt->m_nOutstandingOverlaps[IOConnect]);

	NetClientAddRef(CRITSEC_FUNC_FILELINE(clt));
#if !ISVERSION(SERVER_VERSION)
	if(FakeConnectEx(net,clt->m_hSocket, (const sockaddr *)&remote_addr, remote_addrlen, overlap) == FALSE)
#else
	if (clt->m_WsockFn->ConnectEx(clt->m_hSocket, (const sockaddr *)&remote_addr, remote_addrlen, NULL, 0, NULL, overlap) == FALSE)
#endif
	{
		int error = WSAGetLastError();
		switch (error)
		{
		case ERROR_IO_PENDING:
			break;

		case WSAEFAULT:					// the name or namelen parameter were invalid
		case WSAEINVAL:					// a parameter was invalid or the socket is in an invalid state
		case WSAEALREADY:				// a connect call is already in progress on the socket
		case WSAENOTSOCK:				// not a valid socket
		case WSAEAFNOSUPPORT:			// an address of the specified family can't be used with this socket
		case WSAEADDRINUSE:				// the socket's local address is already in use
		case WSAEADDRNOTAVAIL:			// the remote address isn't valid
		case WSAENETDOWN:				// network susbystem is down
		case WSAENETUNREACH:			// network can't be reached at this time
		case WSAENOBUFS:				// no buffer space is available
		case WSANOTINITIALISED:			// need to call WSAStartup()
		default:
			NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));
			NetErrorMessage(error);
			goto _error;

		case WSAETIMEDOUT:				// attempt timed out without establishing a connection
		case WSAECONNREFUSED:			// connection was refused
			NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));
			NetErrorMessage(error);
			goto _error;
		}
	}

	return TRUE;

_error:
	if (overlap)
	{
		InterlockedDecrement(&clt->m_nOutstandingOverlaps[IOConnect]);
		NetRecycleOverlap(net, overlap);
	}
	NetRecycleSocket(net, clt->m_WsockFn->m_SocketPool, clt->m_hSocket);
	clt->m_eClientState = CONNECT_STATUS_CONNECTFAILED;
	return FALSE;
}


// ---------------------------------------------------------------------------
// DESC:	create & initialize a client
// LOCK:	call from main thread only
// ---------------------------------------------------------------------------
NETCLIENTID NetCreateClient(
	NETCOM * net,
	const net_clt_setup & setup)
{
	ASSERT_RETVAL(net, INVALID_NETCLIENTID);
	ASSERT_RETVAL(setup.remote_addr, INVALID_NETCLIENTID);
    
    if(setup.fp_InitUserData || setup.user_data)
    {
        ASSERT_RETVAL(((setup.fp_InitUserData == NULL) ^ (setup.user_data == NULL)),INVALID_NETCLIENTID);
    }

	NETCLT * clt = NetGetFreeClient(net, setup.fp_InitUserData);
	if (!clt)
	{
		return INVALID_NETCLIENTID;
	}
    if(setup.user_data)
    {
        memcpy(NetGetCltUserData(clt),setup.user_data,net->m_nUserDataSize);
    }

	PStrCopy(clt->m_szName, setup.name, NET_NAME_SIZE);

	sockaddr_storage remote_addr;
	unsigned int remote_addrlen;
	unsigned int remote_family, remote_socktype, remote_protocol;
	if (!NetGetAddr(setup.remote_addr, DEFAULT_FAMILY, setup.remote_port, 0, &remote_addr, &remote_addrlen, &remote_family, &remote_socktype, &remote_protocol))
	{
		ASSERT_GOTO(0, _error);
	}

	sockaddr_storage local_addr;
	unsigned int local_addrlen;
	unsigned int local_family, local_socktype, local_protocol;
	if (!NetGetAddr(setup.local_addr, DEFAULT_FAMILY, setup.local_port, AI_PASSIVE, &local_addr, &local_addrlen, &local_family, &local_socktype, &local_protocol))
	{
		ASSERT_GOTO(0, _error);
	}

	if (local_socktype != remote_socktype || local_protocol != remote_protocol)
	{
		ASSERT_GOTO(0, _error);
	}

	// setup command table & mail slot
    if(setup.fp_DirectReadCallback == NULL)
    {
        ASSERT_GOTO(setup.cmd_tbl, _error);
    }
	clt->m_CmdTbl = setup.cmd_tbl;
	clt->m_mailslot = setup.mailslot;
	clt->m_nNagle = setup.nagle_value;
	clt->m_bSequentialWrites = setup.sequential_writes;
    clt->m_bDontLockOnSend = setup.dont_enforce_thread_safe_writes;
	clt->m_fpNetSend = NetSelectSendFunction(FALSE, setup.nagle_value, setup.sequential_writes);

	clt->m_eClientState = NETCLIENT_STATE_CONNECTING;

	NETCLIENTID idClient = NetGetClientId(clt);

    clt->m_fpDirectReadCallback = setup.fp_DirectReadCallback;
	clt->m_fpConnectCallback    = setup.fp_ConnectCallback;
	if (!NetInitClient(net, clt, setup, local_addr, local_addrlen, remote_addr, remote_addrlen, local_family, local_socktype, local_protocol))
	{
		goto _error;
	}
	if(setup.max_pending_sends)
	{
		clt->m_nMaxOutstandingSends = min(setup.max_pending_sends,MAX_SENDS_PERMITTED);
	}
	if(setup.max_pending_bytes)
	{
		clt->m_nMaxOutstandingBytes = min(setup.max_pending_bytes,MAX_SEND_BYTES_PERMITTED);
	}

	
	return idClient;

_error:
	NetFreeClient(net, clt);
	return INVALID_NETCLIENTID;
}


// ---------------------------------------------------------------------------
// DESC:	create & initialize a local connection
// LOCK:	call from main thread only
// ---------------------------------------------------------------------------
BOOL NetCreateLocalConnection(
	NETCOM * net,
	NETSRV * srv,
	const net_loc_setup & setup,
	NETCLIENTID * idClient,
	NETCLIENTID * idServer)
{
	ASSERT_RETFALSE(net);
	ASSERT_RETFALSE(srv);

	NETCLT * clt_clt = NULL, * srv_clt = NULL;

	clt_clt = NetGetFreeClient(net, setup.clt_fpInitUserData);
	ASSERT_GOTO(clt_clt, _error);

	srv_clt = NetGetFreeClient(net, setup.srv_fpInitUserData);
	ASSERT_GOTO(srv_clt, _error);

	PStrCopy(clt_clt->m_szName, setup.clt_name, NET_NAME_SIZE);
	PStrCopy(srv_clt->m_szName, setup.srv_name, NET_NAME_SIZE);

	// setup command table & mail slot
	ASSERT(setup.clt_cmd_tbl);
	clt_clt->m_CmdTbl = setup.clt_cmd_tbl;
	clt_clt->m_mailslot = setup.clt_mailslot;
	clt_clt->m_idLocalConnection = NetGetClientId(srv_clt);
	clt_clt->m_fpNetSend = NetSelectSendFunction(TRUE, NAGLE_OFF, FALSE);

	srv_clt->m_pServer = srv;
	srv_clt->m_CmdTbl = srv->m_CmdTbl;
	srv_clt->m_mailslot = srv->m_mailslot;
	srv_clt->m_idLocalConnection = NetGetClientId(clt_clt);
	srv_clt->m_fpNetSend = NetSelectSendFunction(TRUE, NAGLE_OFF, FALSE);
	srv_clt->m_bCounted = 1;
	srv_clt->m_fpDirectReadCallback = srv->m_fpDirectReadCallback;
	InterlockedIncrement(&net->m_ClientTable.m_nClientCount);	// add one 

	clt_clt->m_eClientState = NETCLIENT_STATE_CONNECTED;
	srv_clt->m_eClientState = NETCLIENT_STATE_CONNECTED;

	*idClient = NetGetClientId(clt_clt);
	*idServer = NetGetClientId(srv_clt);
	
	srv->m_nClientCount = 1;
	if (srv->m_fpAcceptCallback)
	{
		sockaddr_storage fake = {0};
		srv->m_fpAcceptCallback(NULL,NetGetClientId(srv_clt),&fake,&fake);
	}

	return TRUE;

_error:
	NetFreeClient(net, clt_clt);
	NetFreeClient(net, srv_clt);

	*idClient = INVALID_NETCLIENTID;
	*idServer = INVALID_NETCLIENTID;

	return FALSE;
}


// ---------------------------------------------------------------------------
// DESC:	close a client connection & release the client
// LOCK:	call from anywhere
// ---------------------------------------------------------------------------
BOOL NetCloseClient(
	NETCOM * net,
	NETCLIENTID idClient,
	BOOL bGraceful)
{
	BOOL retval = FALSE;
	if (idClient == INVALID_NETCLIENTID)
	{
		return TRUE;
	}
	NETCLT * clt = NetGetClientData(CRITSEC_FUNC_FILELINE(net, idClient, FALSE));
	if (!clt)
	{
		return TRUE;
	}
	if(clt->m_bCloseCalled)
	{
		goto done;
	}
    clt->m_bCloseCalled = TRUE;


	int client_state = clt->m_eClientState;
	switch (client_state)
	{
	case NETCLIENT_STATE_CONNECTED:
		{
			OVERLAPPEDEX * overlap = NULL;
			NetDisconnectEx(net, clt, overlap, bGraceful);
		}
		retval = TRUE;
		break;

	case NETCLIENT_STATE_CONNECTING:
		{
			OVERLAPPEDEX * overlap = NULL;
			NetDisconnectEx(net, clt, overlap, bGraceful);
		}
		retval = TRUE;
		break;

	case NETCLIENT_STATE_CONNECTFAILED:
		NetClientSubRef(CRITSEC_FUNC_FILELINE(clt));
		retval = TRUE;
		break;

	case NETCLIENT_STATE_LISTENING:				// close listening client?
		ASSERT(clt->m_pServer);
		ASSERT(clt->m_idLocalConnection == INVALID_NETCLIENTID);
		break;

	case NETCLIENT_STATE_DISCONNECTING:
		break;

	default:
		ASSERT(0);
		break;
	}

done:
	NetReleaseClientData(CRITSEC_FUNC_FILELINE(net, clt));

	return retval;
}


// ---------------------------------------------------------------------------
// DESC:	return TRUE if client is connected
// LOCK:	call from anywhere
// NOTE:	of course, the moment we return, the state could change!
// ---------------------------------------------------------------------------
BOOL NetClientIsConnected(
	NETCOM * net,
	NETCLIENTID idClient)
{
	BOOL result = FALSE;

	if (idClient == INVALID_NETCLIENTID)
	{
		return result;
	}

	NETCLT * clt = NetGetClientData(CRITSEC_FUNC_FILELINE(net, idClient, FALSE));
	if (clt)
	{
		result = (clt->m_eClientState == NETCLIENT_STATE_CONNECTED);
		NetReleaseClientData(CRITSEC_FUNC_FILELINE(net, clt));
	}
	return result;
}


// ---------------------------------------------------------------------------
// DESC:	return status of client
// LOCK:	call from anywhere
// NOTE:	of course, the moment we return, the state could change!
// ---------------------------------------------------------------------------
CONNECT_STATUS NetClientGetConnectionStatus(
	NETCOM * net,
	NETCLIENTID idClient)
{
	CONNECT_STATUS result = CONNECT_STATUS_DISCONNECTED;

	if (idClient == INVALID_NETCLIENTID)
	{
		return result;
	}

	NETCLT * clt = NetGetClientData(CRITSEC_FUNC_FILELINE(net, idClient, FALSE));
	if (clt)
	{
		switch (clt->m_eClientState)
		{
		default:
		case NETCLIENT_STATE_NONE:
			break;
		case NETCLIENT_STATE_CONNECTING:
			result = CONNECT_STATUS_CONNECTING;
			break;
		case NETCLIENT_STATE_CONNECTED:
			result = CONNECT_STATUS_CONNECTED;
			break;
		case NETCLIENT_STATE_DISCONNECTING:
			result = CONNECT_STATUS_DISCONNECTING;
			break;
		case NETCLIENT_STATE_CONNECTFAILED:
			result = CONNECT_STATUS_CONNECTFAILED;
			break;
		}
		NetReleaseClientData(CRITSEC_FUNC_FILELINE(net, clt));
	}
	return result;
}
///
///  ****************** CHECK THROTTLING LIMITS ***********************************
// shouldn't need to interlockedcompareexchange these.
static BOOL sCheckThrottleLimits(NETCLT *clt)
{

	if(clt->m_nMaxOutstandingSends > 0)
	{
		if(clt->m_nOutstandingOverlaps[IOWrite] >= clt->m_nMaxOutstandingSends)
		{
			TraceWarn(TRACE_FLAG_COMMON_NET,
				"Warning: Netclient id 0x%08x Already have %d outstanding sends and limit is %d\n",
                clt->m_id,
				clt->m_nOutstandingOverlaps[IOWrite],
				clt->m_nMaxOutstandingSends);
			return FALSE;
		}
	}
	if(clt->m_nMaxOutstandingBytes > 0)
	{
		if(clt->m_nOutstandingBytes >= clt->m_nMaxOutstandingBytes)
		{
			TraceWarn(TRACE_FLAG_COMMON_NET,
				"Warning: Netclient id 0x%08x Already have %d outstanding sends and limit is %d\n",
                clt->m_id,
				clt->m_nOutstandingBytes,
				clt->m_nMaxOutstandingBytes);
			return FALSE;
		}
	}
	if(clt->m_pServer)
	{
		if((clt->m_pServer->m_nMaxOutstandingSends > 0) && 
			(clt->m_nOutstandingOverlaps[IOWrite] >= 
			clt->m_pServer->m_nMaxOutstandingSends))
		{
			if(clt->m_pServer->m_PerfInstance)
			{
				PERF_ADD64(clt->m_pServer->m_PerfInstance,CommonNetServer,connectionsThrottledBySendCount,1);
			}
			TraceWarn(TRACE_FLAG_COMMON_NET,
				"Warning: Netclient id 0x%08x Already have %d outstanding sends and limit is %d\n",
                clt->m_id,
				clt->m_nOutstandingOverlaps[IOWrite],
				clt->m_pServer->m_nMaxOutstandingSends);
			return FALSE ;
		}
		if((clt->m_pServer->m_nMaxOutstandingBytes > 0) && 
			(clt->m_nOutstandingBytes >= clt->m_pServer->m_nMaxOutstandingBytes))
		{
			if(clt->m_pServer->m_PerfInstance)
			{
				PERF_ADD64(clt->m_pServer->m_PerfInstance,CommonNetServer,connectionsThrottledByByteCount,1);
			}
			TraceWarn(TRACE_FLAG_COMMON_NET,
				"Warning: Netclient id 0x%08x Already have %d outstanding bytes and limit is %d\n",
                clt->m_id,
				clt->m_nOutstandingBytes,
				clt->m_pServer->m_nMaxOutstandingBytes);
			return FALSE;
		}
	}
	return TRUE;
}
///  ****************** END CHECK THROTTLING LIMITS ***********************************


// ---------------------------------------------------------------------------
// DESC:	send a message
// LOCK:	call from anywhere
// WARNING:	if we're calling NetSend() from the main thread, there're no
//			ordering issues, but if we call NetSend() from a worker thread
//			there's no guarantee of what order the client is going to get
//			these messages...
//			currently, we don't send a message from a non-main thread
//			except in response to a previous message, so it doesn't break
// ---------------------------------------------------------------------------
int NetSend(
	NETCOM * net,
	NETCLIENTID idClient,
	const void * data,
	unsigned int size)
{
	if (!net)
	{
		return -1;
	}
	if(idClient == INVALID_NETCLIENTID)
    {
#if !ISVERSION(RETAIL_VERSION)
        TraceNetWarn("Trying to send to INVALID_NETCLIENTID\n");
#endif
		return -1;
	}
	ASSERT_RETVAL(size <= (MESSAGE_BUFFER_ALLOC_SIZE - OFFSET(MSG_BUF, msg)), -1);

	// return false if the server is closed/closing
	if (net->m_eState == SERVER_STATE_SHUTDOWN)
	{
		return -1;
	}


	NETCLT * clt = NetGetClientData(CRITSEC_FUNC_FILELINE(net, idClient, FALSE));
	if (!clt)
	{
		return -1;
	}
	int result = -1;
	ONCE
	{
		if (clt->m_pServer && clt->m_pServer->m_eServerState != SERVER_STATE_READY)
		{
			break;
		}
		if (clt->m_eClientState != NETCLIENT_STATE_CONNECTED)
		{
			if (!clt->m_pServer)
			{	
				TraceNetDebug("NETWORK ERROR: not connected!");
			}
			break;
		}
		if (clt->m_hSocket == INVALID_SOCKET && clt->m_idLocalConnection == INVALID_NETCLIENTID)
		{
			break;
		}
		if(!sCheckThrottleLimits(clt))
		{
			result = 0;
			break;
		}

#if NET_MSG_COUNTER
		ASSERT(size >= MSG_HDR_SIZE);
		*(DWORD *)((BYTE *)data + sizeof(NET_CMD) + sizeof(MSG_SIZE)) = clt->m_nMsgSendCounter;
		trace("[%s] msg_send_counter: %d  cmd: %d  size: %d  checksum: %08x\n", NetCltGetName(clt), clt->m_nMsgSendCounter, *(NET_CMD *)data, size, CRC(0, (BYTE *)data, size));
		++clt->m_nMsgSendCounter;
#endif 

		ASSERT_BREAK(clt->m_fpNetSend);

        {
            if(!clt->m_bDontLockOnSend)
            {
                CLT_VERSION_ONLY(clt->m_csSendLock.Enter());
            }
            ASSERT_BREAK(clt->m_encoderSend.Encode( (BYTE *)data, size) );
            //Note: encryption.
            //May be necessary to move deeper into the code, since compression 
            //has to occur after buffering, and encryption after compression.

            result = (clt->m_fpNetSend(net, clt, data, size) == TRUE ? size: -1);

            if(!clt->m_bDontLockOnSend)
            {
                CLT_VERSION_ONLY(clt->m_csSendLock.Leave());
            }
        }
	}
	NetReleaseClientData(CRITSEC_FUNC_FILELINE(net, clt));

	return result;
}


// ---------------------------------------------------------------------------
// DESC:	shutdown network
// LOCK:	call from main thread only
// TODO:	well, closing all those sockets takes awhile, maybe put this on
//			a different thread?
// ---------------------------------------------------------------------------
void NetClose(
	NETCOM * net)
{
	if (!net)
	{
		return;
	}

	net->m_eState = SERVER_STATE_SHUTDOWN;

	// free all servers
	for (unsigned int ii = 0; ii < MAX_SERVERS; ii++)
	{
		NetCloseServer(net, &net->m_Servers[ii]);
	}

	// wait for all threads to finish creating
	for (unsigned int ii = 0; ii < 1000; ++ii)
	{
		if (net->m_nCurrentThreads == net->m_nThreadPoolCount)
		{
			break;
		}
		Sleep(10);
	}

	// shut down worker threads
	unsigned int thread_count = net->m_nCurrentThreads;

	// fill in handle array before issuing stop!
	HANDLE arrayWorkerHandles[MAX_WORKER_THREADS];
	for (unsigned int ii = 0; ii < thread_count; ii++)
	{
		arrayWorkerHandles[ii] = net->m_WorkerData[ii].m_hHandle;
	}	

	for (unsigned int ii = 0; ii < thread_count; ii++)
	{
		PostQueuedCompletionStatus(net->m_WorkerData[ii].m_hWorkerCompletionPort, 0, (DWORD_PTR)INVALID_NETCLIENTID, NULL);
	}

	// wait until all worker threads exit
	ASSERT(WaitForMultipleObjects((DWORD)thread_count, arrayWorkerHandles, TRUE, INFINITE) == WAIT_OBJECT_0);

	sNetFree(net);

	NetWinsockFree();
}


//----------------------------------------------------------------------------
// DESC: returns the previously set list.
// LOCK: call from anywhere except NetCheckBanList and self.
//----------------------------------------------------------------------------
BOOL NetSetBanList(
	NETCOM * net,
	NETSRV * srv,
	IPADDRDEPOT * list )
{
	ASSERT_GOTO(net,_ERROR);
	ASSERT_GOTO(srv,_ERROR);
	ASSERT_GOTO(list,_ERROR);
	ASSERT_GOTO(net->m_eState!=SERVER_STATE_SHUTDOWN,_ERROR);

	srv->m_banListLock.WriteLock();
	NetFreeBanList(srv->m_banList);
	srv->m_banList = list;
	srv->m_banListLock.EndWrite();

	return TRUE;
_ERROR:
	NetFreeBanList(list);
	return FALSE;
}


// ---------------------------------------------------------------------------
// DESC:	set the name of a thread
// ---------------------------------------------------------------------------
#pragma warning(push) // /analyze hates this function
#pragma warning(disable : 6312 6322)
void SetThreadName(
	const char * name,
	DWORD dwThreadID)
{
	struct THREADNAME_INFO
	{
		DWORD	dwType;				// must be 0x1000
		LPCSTR	szName;				// pointer to name (in user addr space)
		DWORD	dwThreadID;			// thread ID (-1=caller thread)
		DWORD	dwFlags;			// reserved for future use, must be zero
	};

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException(0x406D1388, 0, sizeof(info)/sizeof(DWORD), (ULONG_PTR*)&info);
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}
#pragma warning(pop)


// ---------------------------------------------------------------------------
// DESC:	get local ip addresses of a particular family & fills out
//			family for each address
// ---------------------------------------------------------------------------
static unsigned int sNetGetLocalIpAddr(
	ULONG familytoget, 
	FP_GETADAPTERSADDRESSES fpGetAdaptersAddresses,
	char * str,
	ULONG * family,
	unsigned int size,
	unsigned int count)
{
	unsigned int addrcount = 0;
	IP_ADAPTER_ADDRESSES * ipaddrs = NULL;
	ONCE
	{
		ipaddrs = (IP_ADAPTER_ADDRESSES *)MALLOC(NULL, sizeof(IP_ADAPTER_ADDRESSES));
		ULONG buflen = 0;
		if (fpGetAdaptersAddresses(familytoget, 0, NULL, ipaddrs, &buflen) == ERROR_BUFFER_OVERFLOW)
		{
			ipaddrs = (IP_ADAPTER_ADDRESSES *)REALLOC(NULL, ipaddrs, buflen);
		}

		DWORD error = fpGetAdaptersAddresses(familytoget, 0, NULL, ipaddrs, &buflen);
		if (error != ERROR_SUCCESS)
		{
			break;
		}

		IP_ADAPTER_ADDRESSES * ai = NULL;
		IP_ADAPTER_ADDRESSES * ai_next = ipaddrs;
		while ((ai = ai_next) != NULL)
		{
			ai_next = ai->Next;

			DWORD strlen = size;
			if (!ai->FirstUnicastAddress)
			{
				continue;
			}
			if (ai->OperStatus != IfOperStatusUp)
			{
				continue;
			}
			if (ai->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
			{
				continue;
			}
			if (WSAAddressToString(ai->FirstUnicastAddress->Address.lpSockaddr, 
                                   ai->FirstUnicastAddress->Address.iSockaddrLength, 
                                   NULL, 
                                   str, 
                                   &strlen) == 0)
			{
				trace("Detected ip address: %s\n", str);
				str += size;
				addrcount++;
				if (family)
				{
					*family = familytoget;
					family++;
				}
				if (addrcount >= count)
				{
					break;
				}
			}
			else
			{
				int nLastError = WSAGetLastError();
				trace( "Error with WSAAddressToString: returned %d\n", nLastError );
			}
		}

	}

	if (ipaddrs)
	{
		FREE(NULL, ipaddrs);
	}

	return addrcount;
}


// ---------------------------------------------------------------------------
// DESC:	get local ip addresses & family for each address
// ---------------------------------------------------------------------------
unsigned int NetGetLocalIpAddr(
	char * str,
	ULONG * family,
	unsigned int size,
	unsigned int count)
{
	HMODULE iphlp = LoadLibrary(TEXT("Iphlpapi.dll"));
	if (!iphlp)
	{
		return 0;
	}

	unsigned int addrcount = 0;
	ONCE
	{
		FP_GETADAPTERSADDRESSES fpGetAdaptersAddresses = (FP_GETADAPTERSADDRESSES)GetProcAddress(iphlp, TEXT("GetAdaptersAddresses"));
		if (!fpGetAdaptersAddresses)
		{
			break;
		}

		addrcount = sNetGetLocalIpAddr(AF_INET, fpGetAdaptersAddresses, str, family, size, count);
		addrcount += sNetGetLocalIpAddr(AF_INET6, fpGetAdaptersAddresses, str + (size * addrcount), family ? family + addrcount : 0, size, count - addrcount);
	}
	FreeLibrary(iphlp);

	return addrcount;
}


// ---------------------------------------------------------------------------
// DESC:	get ip address for server (first ipv4 address & first ipv6 addr)
// ---------------------------------------------------------------------------
unsigned int NetGetServerIpToUse(
	char * str,
	unsigned int size,
	unsigned int count)
{
	HMODULE iphlp = LoadLibrary(TEXT("Iphlpapi.dll"));
	if (!iphlp)
	{
		return 0;
	}

	unsigned int addrcount = 0;
	ONCE
	{
		FP_GETADAPTERSADDRESSES fpGetAdaptersAddresses = (FP_GETADAPTERSADDRESSES)GetProcAddress(iphlp, TEXT("GetAdaptersAddresses"));
		if (!fpGetAdaptersAddresses)
		{
			break;
		}

		addrcount = sNetGetLocalIpAddr(AF_INET, fpGetAdaptersAddresses, str, NULL, size, 1);
		if (addrcount < count)
		{
			addrcount += sNetGetLocalIpAddr(AF_INET6, fpGetAdaptersAddresses, str + (size * addrcount), NULL, size, 1);
		}
	}
	FreeLibrary(iphlp);

	return addrcount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGetBestInterfaceForAddress(
	__notnull FP_GETBESTINTERFACE fpGetBestInterface,
	sockaddr_storage & remote_addr,
	__notnull DWORD * bestInterfaceIndex )
{
	sockaddr_in remote_in_addr = ((sockaddr_in*)&remote_addr)[0];

	IPAddr remote_ipv4_addr = ((IPAddr*)&remote_in_addr.sin_addr)[0];

	DWORD interfaceResult = fpGetBestInterface(remote_ipv4_addr, bestInterfaceIndex);

	ASSERT_RETFALSE(interfaceResult == NO_ERROR);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sMacBytesToString(
	char * szDest,
	DWORD destLen,
	const BYTE * pBuff,
	DWORD buffLen )
{
	ASSERT_RETFALSE(szDest);
	ASSERT_RETFALSE(pBuff);
	
	szDest[0] = 0;
	if (buffLen == 0)
		return TRUE;
	
							//	hex bytes		dashes			null terminator
	ASSERT_RETFALSE(destLen >= ((buffLen * 2) + (buffLen - 1) + 1));

	static const char sHexChars[] = {
		'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

	DWORD strIndex = 0;
	for (DWORD ii = 0; ii < buffLen; ++ii)
	{
		szDest[ strIndex++ ] = sHexChars[pBuff[ii] >> 4];
		szDest[ strIndex++ ] = sHexChars[pBuff[ii] & 0xF];
		szDest[ strIndex++ ] = '-';
	}
	szDest[ --strIndex ] = 0;

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGetInterfaceMacAddress(
	__notnull FP_GETIFTABLE fpGetIfTable,
	          DWORD interfaceIndex,
	__notnull char * outMacAddress,
	          DWORD outMacAddressLen)
{
	PMIB_IFTABLE pTbl = NULL;
	BOOL success = FALSE;

	ONCE
	{
		ULONG buffSize = sizeof(MIB_IFTABLE);
		pTbl = (PMIB_IFTABLE)MALLOC(NULL, buffSize);

		DWORD getIfTblResult = fpGetIfTable(pTbl, &buffSize, FALSE);
		if (getIfTblResult == ERROR_INSUFFICIENT_BUFFER)
		{
			pTbl = (PMIB_IFTABLE)REALLOC(NULL, pTbl, buffSize);
			getIfTblResult = fpGetIfTable(pTbl, &buffSize, FALSE);
		}

		ASSERT_BREAK(getIfTblResult == NO_ERROR);

		for (DWORD ii = 0; ii < pTbl->dwNumEntries; ++ii)
		{
			if (pTbl->table[ii].dwIndex == interfaceIndex)
			{
				sMacBytesToString(outMacAddress, outMacAddressLen, pTbl->table[ii].bPhysAddr, pTbl->table[ii].dwPhysAddrLen);
				success = TRUE;
				break;
			}
		}
	}

	if (pTbl)
		FREE(NULL, pTbl);

	return success;
}

//----------------------------------------------------------------------------
// DECS:	return the mac address of the adapter to be used given
//			a target ip or url
// NOTE:	currently only for IPv4, there are good methods for IPv6, but
//			only on XP+. I decided to stick to IPv4 since just about everything
//			else in this file is limited to IPv4 in one way or another.
//----------------------------------------------------------------------------
BOOL NetGetClientMacAddress(
	const char * targetUrlOrIPv4Address,
	char * outMacAddress,
	DWORD outMacAddressLen )
{
	ASSERT_RETFALSE(targetUrlOrIPv4Address && targetUrlOrIPv4Address[0]);
	ASSERT_RETFALSE(outMacAddress);

	BOOL success = FALSE;
	outMacAddress[0] = 0;

	//	resolve the target address
	sockaddr_storage remote_addr;
	unsigned int remote_addrlen, remote_family, remote_socktype, remote_protocol;
	ASSERT_RETFALSE(NetGetAddr(targetUrlOrIPv4Address, DEFAULT_FAMILY, 443, 0, &remote_addr, &remote_addrlen, &remote_family, &remote_socktype, &remote_protocol));
	ASSERT_RETFALSE(remote_family == AF_INET);

	HMODULE iphlp = LoadLibrary(TEXT("Iphlpapi.dll"));
	ASSERT_RETFALSE(iphlp);

	ONCE
	{
		//	get the target methods
		FP_GETBESTINTERFACE fpGetBestInterface = (FP_GETBESTINTERFACE)GetProcAddress(iphlp, TEXT("GetBestInterface"));
		FP_GETIFTABLE       fpGetIfTable       = (FP_GETIFTABLE)GetProcAddress(iphlp, TEXT("GetIfTable"));
		ASSERT_BREAK(fpGetBestInterface && fpGetIfTable);

		//	resolve which interface will be used
		DWORD bestInterfaceIndex = 0;
		ASSERT_BREAK(sGetBestInterfaceForAddress(fpGetBestInterface, remote_addr, &bestInterfaceIndex));

		//	look up the current mac address for that interface
		ASSERT_BREAK(sGetInterfaceMacAddress(fpGetIfTable, bestInterfaceIndex, outMacAddress, outMacAddressLen));

		success = TRUE;
	}

	FreeLibrary(iphlp);

	return success;
}


// ---------------------------------------------------------------------------
// DESC:	output a dbg to a string
// ---------------------------------------------------------------------------
void DbgPrintBuf(
	char * str,
	int strlen,
	const BYTE * buf,
	int buflen)
{
	ASSERT(strlen > buflen);
	if (strlen <= buflen)
	{
		buflen = strlen - 1;
	}

	const BYTE * end = buf + buflen;
	while (buf < end)
	{
		char c = *(const char *)buf;
		*str = (c >= 32 && c < 128) ? c : '.';
		buf++;
		str++;
	}
	*str = 0;
}


// ---------------------------------------------------------------------------
// find a message struct by name
// ---------------------------------------------------------------------------
static MSG_STRUCT_DECL * NetFindMsgStructByName(
	NET_COMMAND_TABLE * cmd_tbl,
	const char ** name)
{
	ASSERT_RETNULL(cmd_tbl);
	for (unsigned int ii = 0; ii < cmd_tbl->m_MsgStruct.m_nStructCount; ii++)
	{
		if (cmd_tbl->m_MsgStruct.m_Structs[ii].m_szName == name)
		{
			return cmd_tbl->m_MsgStruct.m_Structs + ii;
		}
	}
	return NULL;
}


// ---------------------------------------------------------------------------
// get message type from string
// ---------------------------------------------------------------------------
static NET_FIELD_TYPE NetMessageGetTypeFromString(
	NET_COMMAND_TABLE * cmd_tbl,
	const char * typestr,
	const char ** structTypeStr,
	MSG_STRUCT_DECL ** structptr)
{
	ASSERT(arrsize(g_NetFieldTypes) == LAST_NET_FIELD);
	
	if (typestr)
	{
		for (unsigned int ii = 0; ii < LAST_NET_FIELD; ii++)
		{
			if (g_NetFieldTypes[ii].m_szStr && PStrCmp(typestr, g_NetFieldTypes[ii].m_szStr) == 0)
			{
				return (NET_FIELD_TYPE)ii;
			}
		}
	}
	*structptr = NetFindMsgStructByName(cmd_tbl, structTypeStr);
	if (*structptr != NULL)
	{
		return NET_FIELD_STRUCT;
	}
	return LAST_NET_FIELD;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL NetRegisterMsgStructPush(
	MSG_STRUCT_TABLE & msg_tbl,
	MSG_STRUCT_DECL * cur)
{
	ASSERT_RETFALSE(msg_tbl.m_nDefCurTop < MAX_MSG_STRUCT_STACK);
	msg_tbl.m_pDefCur[msg_tbl.m_nDefCurTop++] = cur;
	return TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
MSG_STRUCT_DECL * NetRegisterMsgStructPeek(
	MSG_STRUCT_TABLE & msg_tbl)
{
	ASSERT_RETNULL(msg_tbl.m_nDefCurTop > 0);
	return msg_tbl.m_pDefCur[msg_tbl.m_nDefCurTop - 1];
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void NetRegisterMsgStructPop(
	MSG_STRUCT_TABLE & msg_tbl)
{
	ASSERT_RETURN(msg_tbl.m_nDefCurTop > 0);
	msg_tbl.m_nDefCurTop--;
}


// ---------------------------------------------------------------------------
// register message structure - begin
// ---------------------------------------------------------------------------
MSG_STRUCT_DECL * NetRegisterMsgStructStart(
	NET_COMMAND_TABLE * cmd_tbl,
	const char ** name,
	BOOL isheader,
	BOOL & bAlreadyDefined,
	BOOL isStreamed )
{
	bAlreadyDefined = FALSE;

	MSG_STRUCT_TABLE & msg_tbl = cmd_tbl->m_MsgStruct;

	ASSERT_RETNULL(cmd_tbl);
	ASSERT_RETNULL(msg_tbl.m_nStructCount < MAX_MSG_STRUCTS);
	MSG_STRUCT_DECL * prev = NetFindMsgStructByName(cmd_tbl, name);
	if (prev != NULL)	// already defined
	{
		bAlreadyDefined = TRUE;
		return prev;
	}

	MSG_STRUCT_DECL * cur = msg_tbl.m_Structs + msg_tbl.m_nStructCount;
	NetRegisterMsgStructPush(msg_tbl, cur);
	memclear(cur, sizeof(MSG_STRUCT_DECL));
	cur->m_szName = name;
	if (isheader)
	{
		cur->m_nSize = MSG_HDR_SIZE;
	}
	cur->m_bVarSize = isStreamed;	//	if user-defined streaming, ensure correct sizing.

	msg_tbl.m_nStructCount++;
	return cur;
}


// ---------------------------------------------------------------------------
// register a message field
// ---------------------------------------------------------------------------
void NetRegisterMsgField(
	NET_COMMAND_TABLE * cmd_tbl,
	unsigned int fieldno,
#if !ISVERSION(RETAIL_VERSION)
	const char * name, 
#endif
	const char * typestr,
	const char ** structTypeStr,
	unsigned int offset,
	unsigned int count)
{
	ASSERT_RETURN(cmd_tbl);
	ASSERT(fieldno < MAX_MSG_FIELDS);

	MSG_STRUCT_TABLE & msg_tbl = cmd_tbl->m_MsgStruct;
	if (!msg_tbl.m_pDefCur)
	{
		return;
	}

#if !ISVERSION(RETAIL_VERSION)
	ASSERT_RETURN(name);
#endif
	ASSERT_RETURN(typestr || structTypeStr);
	ASSERT_RETURN(msg_tbl.m_nFieldCount < MAX_MSG_FIELDS_TOTAL);

	MSG_STRUCT_DECL * defcur = NetRegisterMsgStructPeek(msg_tbl);
	ASSERT_RETURN(defcur);

	ASSERT_RETURN(defcur->m_nFieldCount < MAX_MSG_FIELDS);
	ASSERT_RETURN(defcur->m_nFieldCount == fieldno);

	MSG_STRUCT_DECL * structptr = NULL;
	NET_FIELD_TYPE type = NetMessageGetTypeFromString(cmd_tbl, typestr, structTypeStr, &structptr);
	ASSERTX_RETURN(type < LAST_NET_FIELD, "Unknown message field type in net message.");
	ASSERT_RETURN(!structptr || structptr != defcur);	// prevent recursive definition

	MSG_FIELD & msg_field = msg_tbl.m_Fields[msg_tbl.m_nFieldCount];
	msg_tbl.m_nFieldCount++;

	memclear(&msg_field, sizeof(MSG_FIELD));
#if !ISVERSION(RETAIL_VERSION)
	msg_field.m_szName = name;
#endif
	msg_field.m_eType = type;
	msg_field.m_nOffset = offset;
	msg_field.m_nCount = count;
	msg_field.m_nSize = structptr ? structptr->m_nSize : g_NetFieldTypes[type].m_nSize;
	msg_field.m_pStruct = structptr;

	if (!defcur->m_pFields)
	{
		defcur->m_pFields = &msg_field;
	}
	defcur->m_nFieldCount++;
	defcur->m_nSize += msg_field.m_nSize * msg_field.m_nCount;

	switch (type)
	{
	case NET_FIELD_STRUCT:
		ASSERT_RETURN(structptr);
		if (structptr->m_bVarSize)
		{
			defcur->m_bVarSize = TRUE;
		}
		break;
	case NET_FIELD_WCHAR:
	case NET_FIELD_CHAR:
		defcur->m_bVarSize = TRUE;
		break;
	case NET_FIELD_BLOBB:
		ASSERT(count <= UCHAR_MAX);
		defcur->m_bVarSize = TRUE;
		break;
	case NET_FIELD_BLOBW:
		ASSERT(count <= USHRT_MAX);
		defcur->m_bVarSize = TRUE;
		break;
	}
}


// ---------------------------------------------------------------------------
// finish registering a message struct
// ---------------------------------------------------------------------------
void NetRegisterMsgStructEnd(
	NET_COMMAND_TABLE * cmd_tbl)
{
	ASSERT_RETURN(cmd_tbl);
	MSG_STRUCT_TABLE & msg_tbl = cmd_tbl->m_MsgStruct;

	NetRegisterMsgStructPop(msg_tbl);
}


// ---------------------------------------------------------------------------
// register a command in the command table
// ---------------------------------------------------------------------------
void NetRegisterCommand(
	NET_COMMAND_TABLE * cmd_tbl,
	unsigned int cmd,
	MSG_STRUCT_DECL * msg,
	BOOL bLogOnSend,
	BOOL bLogOnRecv,
	MFP_PRINT mfp_print,
	MFP_PUSHPOP mfp_push,
	MFP_PUSHPOP mfp_pop)
{
	ASSERT_RETURN(cmd_tbl);
	ASSERT_RETURN(msg);
	ASSERT_RETURN(cmd < cmd_tbl->m_nCmdCount);
	msg->m_bInitialized = TRUE;

	NET_COMMAND * net_cmd = cmd_tbl->m_Cmd + cmd;
	net_cmd->cmd = cmd;
	net_cmd->msg = msg;
	net_cmd->bLogOnSend = bLogOnSend;
	net_cmd->bLogOnRecv = bLogOnRecv;
	net_cmd->mfp_Print = mfp_print;
	net_cmd->mfp_Push = mfp_push;
	net_cmd->mfp_Pop = mfp_pop;
	net_cmd->varsize = msg->m_bVarSize;
	net_cmd->maxsize = msg->m_nSize;
}


// ---------------------------------------------------------------------------
// create a new command table
// ---------------------------------------------------------------------------
NET_COMMAND_TABLE * NetCommandTableInit(
	unsigned int num_commands)
{
	for (unsigned int ii = 0; ii < LAST_NET_FIELD; ii++)
	{
		ASSERT(g_NetFieldTypes[ii].m_eType == (NET_FIELD_TYPE)ii);
	}

	NET_COMMAND_TABLE * cmd_tbl = (NET_COMMAND_TABLE *)MALLOCZ(NULL, sizeof(NET_COMMAND_TABLE));
	ASSERT_RETNULL(cmd_tbl);
	cmd_tbl->m_nCmdCount = num_commands;
	cmd_tbl->m_Cmd = (NET_COMMAND *)MALLOCZ(NULL, sizeof(NET_COMMAND) * cmd_tbl->m_nCmdCount);
	cmd_tbl->m_RefCount.Init();

#if NET_MSG_COUNT
	cmd_tbl->m_Tracker.Init(num_commands);
#endif

	return cmd_tbl;
}


// ---------------------------------------------------------------------------
// refcount a command table
// ---------------------------------------------------------------------------
void NetCommandTableRef(
	NET_COMMAND_TABLE * cmd_tbl)
{
	ASSERT_RETURN(cmd_tbl);
	cmd_tbl->m_RefCount.RefAdd(CRITSEC_FUNCNOARGS_FILELINE());
}


// ---------------------------------------------------------------------------
// free a new command table
// ---------------------------------------------------------------------------
void NetCommandTableFree(
	NET_COMMAND_TABLE * cmd_tbl)
{
	if (!cmd_tbl)
	{
		return;
	}

	if (cmd_tbl->m_RefCount.RefDec(CRITSEC_FUNCNOARGS_FILELINE()) != 0)
	{
		return;
	}

#if NET_MSG_COUNT
	NetCommandTraceMessageCounts(cmd_tbl);
	cmd_tbl->m_Tracker.Free();
#endif

	FREE(NULL, cmd_tbl->m_Cmd);
	FREE(NULL, cmd_tbl);
}


// ---------------------------------------------------------------------------
// validate
// ---------------------------------------------------------------------------
BOOL NetCommandTableValidate(
	NET_COMMAND_TABLE * cmd_tbl)
{
	ASSERT_RETFALSE(cmd_tbl);

	BOOL result = TRUE;
	for (unsigned int ii = 0; ii < cmd_tbl->m_nCmdCount; ii++)
	{
		ASSERT(cmd_tbl->m_Cmd[ii].msg);
		if (!cmd_tbl->m_Cmd[ii].msg)
		{
			result = FALSE;
		}
	}
	return result;
}


// ---------------------------------------------------------------------------
// trace message counts
// ---------------------------------------------------------------------------
void NetCommandTraceMessageCounts(
	NET_COMMAND_TABLE * cmd_tbl)
{
	ASSERT_RETURN(cmd_tbl);

#if NET_MSG_COUNT
	TraceGameInfo("%44s  %6s  %10s   %6s  %10s", "CMD", "SENT#", "SENT-BYTES", "RECV#", "RECV-BYTES");
	for (unsigned int ii = 0; ii < cmd_tbl->m_nCmdCount; ii++)
	{
		NET_COMMAND & cmd = cmd_tbl->m_Cmd[ii];

		if (!cmd.msg)
		{
			continue;
		}
		NET_MSG_TRACKER_NODE & node = cmd_tbl->m_Tracker[ii];
		LONG sendCount = node.GetSendCount();
		LONG recvCount = node.GetRecvCount();
		if (sendCount || recvCount)
		{
			TraceGameInfo("%44s  %6d  %10I64d   %6d  %10I64d", *cmd.msg->m_szName, sendCount, node.GetSendBytes(), recvCount, node.GetRecvBytes());
		}
	}
#endif
}


// ---------------------------------------------------------------------------
// reset message counts
// ---------------------------------------------------------------------------
void NetCommandResetMessageCounts(
	NET_COMMAND_TABLE * cmd_tbl)
{
	REF(cmd_tbl);
#if NET_MSG_COUNT
	for (unsigned int ii = 0; ii < cmd_tbl->m_nCmdCount; ii++)
	{
		NET_COMMAND & cmd = cmd_tbl->m_Cmd[ii];

		if (!cmd.msg)
		{
			continue;
		}
		cmd_tbl->m_Tracker[ii].Clear();
	}	
#endif
}


//----------------------------------------------------------------------------
// retrieve the number of registered commands
//----------------------------------------------------------------------------
DWORD NetCommandTableGetSize(
	NET_COMMAND_TABLE * cmd_tbl)
{
	ASSERT_RETZERO( cmd_tbl );
	return cmd_tbl->m_nCmdCount;
}


// ---------------------------------------------------------------------------
// register immediate callback function
// ---------------------------------------------------------------------------
BOOL NetCommandTableRegisterImmediateMessageCallback(
	NET_COMMAND_TABLE * cmd_tbl,
	NET_CMD command,
	FP_NET_IMM_CALLBACK func)
{
	ASSERT_RETFALSE(cmd_tbl);
	ASSERT_RETFALSE(command >= 0 && command < cmd_tbl->m_nCmdCount);
	cmd_tbl->m_Cmd[command].mfp_ImmCallback = func;
	return TRUE;
}


// ---------------------------------------------------------------------------
// do structure alignment & endian conversion
// return the message in the bbuf
// theoretically, if we didn't have to do any structure alignment & conversion
// we could simply switch the bbuf's data pointer to a ptr to msg
// ---------------------------------------------------------------------------
BOOL NetTranslateMessageForSend(
	NET_COMMAND_TABLE * cmd_tbl,
	NET_CMD cmd,
	MSG_STRUCT * msg,
	unsigned int & size,
	BYTE_BUF_NET & bbuf)
{
	if (!cmd_tbl)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(msg);
	ASSERT_RETFALSE(cmd < cmd_tbl->m_nCmdCount);
	NET_COMMAND * netcmd = cmd_tbl->m_Cmd + cmd;
	ASSERT_RETFALSE(netcmd->msg);

	unsigned int actual_size = netcmd->msg->m_nSize;
	if (!netcmd->msg->m_bVarSize)
	{
		ASSERT_RETFALSE(actual_size < MAX_LARGE_MSG_SIZE);
	}
	msg->hdr.cmd = cmd;
	msg->hdr.size = (MSG_SIZE)actual_size;
	msg->hdr.Push(bbuf);

	ASSERT_RETFALSE(netcmd->mfp_Push);
	CALL_CMD_PUSH(netcmd->mfp_Push, msg, bbuf);

	if (netcmd->msg->m_bVarSize)					// variable size message, write actual size
	{
		unsigned int cur = bbuf.GetCursor();
		bbuf.SetCursor(msg->hdr.GetSizeOffset());
		bbuf.Push((MSG_SIZE)cur);
		bbuf.SetCursor(cur);
		actual_size = cur;
	}

	ASSERT_RETFALSE(actual_size <= netcmd->maxsize);
	size = actual_size;

#if NET_MSG_COUNT
	// assume 1 translate = 1 send
	NET_MSG_TRACKER_NODE & node = cmd_tbl->m_Tracker[cmd];
	node.IncrementSendCount();
	node.AddSendBytes((LONGLONG)size);
#endif

	return TRUE;
}


// ---------------------------------------------------------------------------
// calculate the size of a message
// ---------------------------------------------------------------------------
unsigned int NetCalcMessageSize(
	NET_COMMAND_TABLE * cmd_tbl,
	NET_CMD cmd,
	MSG_STRUCT * msg)
{
	if (!cmd_tbl)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(msg);
	ASSERT_RETFALSE(cmd < cmd_tbl->m_nCmdCount);
	NET_COMMAND * netcmd = cmd_tbl->m_Cmd + cmd;
	ASSERT_RETFALSE(netcmd->msg);

	unsigned int actual_size = netcmd->msg->m_nSize;
	if (!netcmd->msg->m_bVarSize)
	{
		ASSERT_RETFALSE(actual_size < MAX_LARGE_MSG_SIZE);
	}
	else
	{
		BYTE buf_data[MAX_SMALL_MSG_STRUCT_SIZE];
		BYTE_BUF_NET bbuf(buf_data, MAX_SMALL_MSG_STRUCT_SIZE);

		msg->hdr.cmd = cmd;
		msg->hdr.size = (MSG_SIZE)actual_size;
		msg->hdr.Push(bbuf);

		ASSERT_RETFALSE(netcmd->mfp_Push);
		CALL_CMD_PUSH(netcmd->mfp_Push, msg, bbuf);

		actual_size = bbuf.GetCursor();
	}

	ASSERT_RETFALSE(actual_size <= netcmd->maxsize);
	return actual_size;
}


// ---------------------------------------------------------------------------
// do structure alignment & endian conversion
// return the message in msg (should be allocated to be adequate size
// theoretically, if we didn't have to do any structure alignment & conversion
// we could simply point the msg to the data
// ---------------------------------------------------------------------------
BOOL NetTranslateMessageForRecv(
	NET_COMMAND_TABLE * cmd_tbl,
	BYTE * data,
	unsigned int size,
	MSG_STRUCT * msg)
{
	ASSERT_RETFALSE(cmd_tbl);
	ASSERT_RETFALSE(data && size > 0);

	BYTE_BUF_NET bbuf(data, size);

	msg->hdr.Pop(bbuf);

	NET_CMD cmd = msg->hdr.cmd;
	ASSERT_RETFALSE(cmd < cmd_tbl->m_nCmdCount);
	NET_COMMAND * netcmd = cmd_tbl->m_Cmd + cmd;
	ASSERT_RETFALSE(netcmd->msg);
	if (!netcmd->msg->m_bVarSize)
	{
		ASSERT(size >= netcmd->msg->m_nSize);
	}

	ASSERT_RETFALSE(netcmd->mfp_Pop);
	ASSERT_RETFALSE(CALL_CMD_POP(netcmd->mfp_Pop, msg, bbuf));

	return TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL NetTraceSendMessage(
	NET_COMMAND_TABLE * cmd_tbl,
	MSG_STRUCT * msg,
	const char * label)
{
#if NET_TRACE_MSG2
	if (!cmd_tbl)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(msg);

	NET_CMD cmd = msg->hdr.cmd;
	ASSERT_RETFALSE(cmd < cmd_tbl->m_nCmdCount);
	NET_COMMAND * netcmd = cmd_tbl->m_Cmd + cmd;
	ASSERT_RETFALSE(netcmd->msg);

	if (!netcmd->bLogOnSend)
	{
		return TRUE;
	}

	char buf[4096];
	PStrI str(buf, 4096, label);

	ASSERT_RETFALSE(netcmd->mfp_Print);
	CALL_CMD_PRINT(netcmd->mfp_Print, msg, str);

	LogMessage(SEND_LOG, str);
#else
	REF(cmd_tbl);
	REF(msg);
	REF(label);
#endif

	return TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL NetTraceRecvMessage(
	NET_COMMAND_TABLE * cmd_tbl,
	MSG_STRUCT * msg,
	const char * label)
{
#if NET_TRACE_MSG2
	if (!cmd_tbl)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(msg);

	NET_CMD cmd = msg->hdr.cmd;
	ASSERT_RETFALSE(cmd < cmd_tbl->m_nCmdCount);
	NET_COMMAND * netcmd = cmd_tbl->m_Cmd + cmd;
	ASSERT_RETFALSE(netcmd->msg);

	if (!netcmd->bLogOnRecv)
	{
		return TRUE;
	}

	char buf[4096];
	PStrI str(buf, 4096, label);

	ASSERT_RETFALSE(netcmd->mfp_Print);
	CALL_CMD_PRINT(netcmd->mfp_Print, msg, str);

	LogMessage(RECV_LOG, str);
#else
	REF(cmd_tbl);
	REF(msg);
	REF(label);
#endif

	return TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD NetCommandGetMaxSize(NET_COMMAND_TABLE *cmd_tbl, NET_CMD cmd)
{
    return cmd_tbl->m_Cmd[cmd].maxsize;
}


// ---------------------------------------------------------------------------
// DESC:	send a message to multiple NETCLIENTIDs
// LOCK:	call from anywhere
// ---------------------------------------------------------------------------
BOOL NetMultipointSend(
	NETCOM * net,
	NETCLIENTID *idClients,
    DWORD        numClients,
	const void * data,
	unsigned int size)
{
    UINT failcount = 0;
    BYTE buffer[MAX_LARGE_MSG_SIZE];

	ASSERT_RETFALSE(size <= sizeof(buffer));

    for(DWORD di = 0; di < numClients ; di++)
    {
        //Have to copy because it gets encoded per client
        ASSERT_RETFALSE(MemoryCopy(buffer,sizeof(buffer),data,size));
        if(NetSend(net,idClients[di],buffer,size) <= 0 )
        {
            TraceWarn(TRACE_FLAG_COMMON_NET,"MultipointSend: NetSend to client 0x%08x failed\n",idClients[di]);
            failcount++;
        }
    }
    return (failcount < numClients);
}
#if !ISVERSION(SERVER_VERSION)
BOOL FakeConnectEx(
   NETCOM *net,
   SOCKET sock,
   const struct sockaddr * name,
   int namelen,
   OVERLAPPEDEX * lpOverlapped)
{

	lpOverlapped->m_Overlapped.hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	if(lpOverlapped->m_Overlapped.hEvent == NULL)
	{
		TraceError("Could not create event for fake connect: %d\n",GetLastError());
		return FALSE;
	}
	if(WSAEventSelect(sock,lpOverlapped->m_Overlapped.hEvent ,FD_CONNECT) != SOCKET_ERROR)
	{
		DWORD dwRet = connect(sock,name,namelen);
		if(dwRet == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
		{
			ASSERT_GOTO(dwRet != SOCKET_ERROR,_error);
		}
		ASSERT_GOTO(PostQueuedCompletionStatus(net->m_hFakeConnectExCompletionPort,0,NULL,(OVERLAPPED*)lpOverlapped),_error);
	}
	return TRUE;
_error:
	CloseHandle(lpOverlapped->m_Overlapped.hEvent);
	return FALSE;
}

BOOL FakeDisconnectEx(
	    NETCOM *net,
		SOCKET sock,
		OVERLAPPEDEX * lpOverlapped,
		DWORD dwFlags)
{
	UNREFERENCED_PARAMETER(dwFlags);
	shutdown(sock,SD_SEND);
	ASSERT_RETFALSE(PostQueuedCompletionStatus(net->m_hWorkerCompletionPorts[0],0,NULL,(OVERLAPPED*)lpOverlapped));
	return TRUE;
}

#endif
#if 0 // later
// ---------------------------------------------------------------------------
// DESC:	send a message, after serializing it.
// LOCK:	call from anywhere
// ---------------------------------------------------------------------------
BOOL NetSendMessage(
    NETCOM *net,
    NETCLIENTID cid,
	NET_COMMAND_TABLE * cmd_tbl,
	NET_CMD cmd,
	MSG_STRUCT * msg
    )
{
    BOOL result = FALSE;
    REF_COUNTED_BUFFER *pBuffer = NULL;

	ASSERT_RETFALSE(msg);

	if (!cmd_tbl)
	{
		goto _error;
	}
	ASSERT_RETFALSE(cmd < cmd_tbl->m_nCmdCount);
	NET_COMMAND * netcmd = cmd_tbl->m_Cmd + cmd;
	ASSERT_RETFALSE(netcmd->msg);

	unsigned int actual_size = netcmd->msg->m_nSize;
	if(!netcmd->msg->m_bVarSize)
	{
		ASSERT_RETFALSE(actual_size < MAX_LARGE_MSG_SIZE);
	}
    
    pBuffer = (REF_COUNTED_BUFFER*)MALLOCZ(NULL,actual_size + 
                                                sizeof(REF_COUNTED_BUFFER));

	if (!pBuffer)
	{
		return FALSE;
	}

    {
        BYTE_BUF_NET byteBuf(pBuffer,MAX_MSG_SIZE);

        if(!NetTranslateMessageForSend(cmd_tbl,cmd,msg,actual_size,byteBuf))
        {
            TraceError("NetSendMessage:Could not translate message for send\n");
            ASSERT(result);
            goto _error;
        }
        pBuffer->dataLen = byteBuf.GetCursor();
    }

    result = sNetMultipointSend(net,&cid,1,pBuffer);

_error:
    if(!result)
    {
        if(pBuffer)
        {
            FREE(NULL,pBuffer);
        }
    }
    return result;
}
#endif
