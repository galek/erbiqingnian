//----------------------------------------------------------------------------
// NetworkMap.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _NETWORKMAP_H_
#define _NETWORKMAP_H_


//----------------------------------------------------------------------------
// server info structs used with the net map.
//----------------------------------------------------------------------------

//	all information required about a server instance within the cluster
struct SERVER
{
	SVRTYPE				svrType;
	SVRINSTANCE			svrInstance;
	SVRMACHINEINDEX		svrMachineIndex;
	SOCKADDR_STORAGE	svrIpAddress;
};

//	server info for servers on either side of a communication channel
struct CHANNEL_SERVERS
{
	SVRTYPE				OfferingType;
	SVRMACHINEINDEX		OfferingMachineIndex;
	SVRTYPE				UserType;
	SVRMACHINEINDEX		UserMachineIndex;
};

//----------------------------------------------------------------------------
// NETWORK SERVER MAPPINGS
//----------------------------------------------------------------------------

struct NET_MAP;

NET_MAP *
	NetMapNew(
		struct MEMORYPOOL * pool );

void
	NetMapFree(
		NET_MAP *	toFree );

BOOL
	NetMapSetServerInstanceCount(
		NET_MAP *	map,
		SVRTYPE		svrType,
		DWORD		svrCount );

BOOL
	NetMapAddServer(
		NET_MAP *	map,
		SERVER &	svr );

BOOL
	NetMapRemoveServer(
		NET_MAP *	map,
		SERVER &	svr );

BOOL
	NetMapGetServerAddrForUser(
		NET_MAP *			map,
		SVRTYPE				svrType,
		SVRINSTANCE			svrInstance,
		SVRTYPE				userType,
		SVRMACHINEINDEX		userMachineIndex,
		SOCKADDR_STORAGE &	svrAddr);

DWORD
	NetMapGetServiceInstanceCount(
		NET_MAP *	map,
		SVRTYPE		svrType );

SVRMACHINEINDEX
	NetMapGetServerMachineIndex(
		NET_MAP *	map,
		SVRID		server );

//	HACK: returns provider machine index in provider instance slot as that is what the net layer really needs...
SERVICEID
	NetMapGetServiceIdFromUserAddress(
		NET_MAP *	map,
		SOCKADDR_STORAGE&  remoteAddress,
		SVRPORT		port);

//	returns port in network byte order
SVRPORT
	NetMapGetCommunicationPort(
		SVRTYPE			offeringType,
		SVRMACHINEINDEX	offeringMachineIndex,
		SVRTYPE			userType,
		SVRMACHINEINDEX	userMachineIndex );

//	expects port in network byte order
CHANNEL_SERVERS
	NetMapGetPortUsers(
		SVRPORT port );

#endif	//	_NETWORKMAP_H_
