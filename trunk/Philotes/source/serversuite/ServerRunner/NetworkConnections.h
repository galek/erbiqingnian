//----------------------------------------------------------------------------
// NetworkConnections.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef	_NETWORKCONNECTIONS_H_
#define _NETWORKCONNECTIONS_H_


//----------------------------------------------------------------------------
// NET_CONS
//----------------------------------------------------------------------------

struct NET_CONS;

//----------------------------------------------------------------------------

NET_CONS *
	NetConsInit(
		struct MEMORYPOOL *,
		CONNECTION_TABLE *,
		SVRTYPE,
		SVRINSTANCE,
		SVRMACHINEINDEX );

void
	NetConsFree(
		NET_CONS * );

void
	NetConsCloseActiveConnections(
		NET_CONS *,
		NETCOM * );

//----------------------------------------------------------------------------

CONNECTION_TABLE *
	NetConsGetConnectionTable(
		NET_CONS * );

SVRTYPE
	NetConsGetSvrType(
		NET_CONS * );

SVRINSTANCE
	NetConsGetSvrInstance(
		NET_CONS * );

SVRMACHINEINDEX
	NetConsGetSvrMachineIndex(
		NET_CONS * );

//----------------------------------------------------------------------------

// NOTE: adds all instances to disconnected list
BOOL
	NetConsRegisterProviderType(
		NET_CONS *,
		SVRTYPE,
		DWORD );
void
	NetConsSetProviderStatusAndNetClient(
		NET_CONS *,
		SVRTYPE,
		SVRINSTANCE,
		CONNECT_STATUS,
		IMuxClient * );

IMuxClient *
	NetConsGetProviderNetClient(
		NET_CONS *,
		SVRTYPE,
		SVRINSTANCE );

CONNECT_STATUS
	NetConsGetProviderStatus(
		NET_CONS *,
		SVRTYPE,
		SVRINSTANCE );

//----------------------------------------------------------------------------

void
	NetConsSetProviderMailbox(
		NET_CONS *,
		SVRTYPE,
		SVRINSTANCE,
		SERVER_MAILBOX * );

SERVER_MAILBOX *
	NetConsGetProviderMailbox(
		NET_CONS *,
		SVRTYPE,
		SVRINSTANCE );

//----------------------------------------------------------------------------

DWORD
	NetConsGetDisconnectedProviderCount(
		NET_CONS * );

void
	NetConsIterateDisconnectedProviders(
		NET_CONS *,
		IMuxClient * (*fpProviderItr)( NET_CONS *, SVRTYPE, SVRINSTANCE, SERVER_MAILBOX * ) );

//----------------------------------------------------------------------------

void
	NetConsSetProvidedServicePtr(
		NET_CONS *,
		SVRTYPE,
		SVRMACHINEINDEX,
		MuxServer * );

MuxServer *
	NetConsGetProvidedServicePtr(
		NET_CONS *,
		SVRTYPE,
		SVRMACHINEINDEX );

BOOL
	NetConsGetClientAddress(
		NET_CONS *,
		MUXCLIENTID,
		SOCKADDR_STORAGE&);

int
	NetConsSendToClient(
		NET_CONS*,
		SVRMACHINEINDEX,
		SERVICEUSERID,
		BYTE*,
		DWORD);

int
	NetConsSendToMultipleClients(
		NET_CONS*,
		SVRTYPE,
		SVRMACHINEINDEX,
		SVRCONNECTIONID*,
		UINT,
		BYTE*,
		DWORD);

BOOL
	NetConsDisconnectClient(
		NET_CONS*,
		SVRMACHINEINDEX,
		SERVICEUSERID);

void *
	NetConsGetClientContext(
		NET_CONS*,
		SVRMACHINEINDEX,
		SERVICEUSERID);

SERVICEUSERID
	NetConsGetClientServiceUserId(
		NET_CONS*,
		SVRID);

void
	NetConsClientAttached(
		NET_CONS*,
		SERVICEUSERID);

void
	NetConsClientDetached(
		NET_CONS*,
		SERVICEUSERID);


class NetConsAutolock
{
	protected:
		NET_CONS* m_pCons;	
	public:
		NetConsAutolock(NET_CONS*);	
		~NetConsAutolock();
};

#endif	//	_NETWORKCONNECTIONS_H_
