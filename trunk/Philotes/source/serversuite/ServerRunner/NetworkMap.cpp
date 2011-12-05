//----------------------------------------------------------------------------
// NetworkMap.cpp
// (C)Copyright 2007, Ping0 Interactive Limited. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"
#include "hlocks.h"
#include "NetworkMap.cpp.tmh"


//----------------------------------------------------------------------------
// STORAGE TYPES
//----------------------------------------------------------------------------

//	internal ip map key
typedef UINT32		IPMAPID;
inline  IPMAPID		IpMapId(	SVRTYPE			type,
								SVRMACHINEINDEX mIndex ) { return ( ((UINT32)mIndex << 16) | (UINT32)type ); }

//	sockaddr struct compare class
class sockaddr_compare 
{
public:
	bool operator()(const SOCKADDR_STORAGE& lhs, const SOCKADDR_STORAGE& rhs) const;
};

//	internal server instance storage type
struct MAP_SVR : SERVER
{
	IPMAPID						id;
};

//	server instance entry hash
typedef map_mp<IPMAPID, MAP_SVR>::type SVRS_BY_INDEX;

//	top level hash of server instances by hosting machine address
struct MACHINE_SERVERS
{
	SOCKADDR_STORAGE			addr;
	SVRS_BY_INDEX				Servers;
};

//	cluster machine hash
typedef map_mp<SOCKADDR_STORAGE,MACHINE_SERVERS*, sockaddr_compare>::type SVRS_BY_IP;

//	map lock
DEF_HLOCK(NetMapLock,HLReaderWriter)
	HLOCK_LOCK_RESTRICTION(ALL_LOCKS_HLOCK_NAME)
END_HLOCK

//----------------------------------------------------------------------------
// NET_MAP
// THREAD SAFETY: all methods are fully thread safe
// DESC: provides and maintains a map of the cluster network layout.
//		responsible for mapping raw port/ip data to and from server type/instance data.
//		controlled by the server runner but used by the network layer.
//----------------------------------------------------------------------------
struct NET_MAP
{
	//	map server data
	SVRS_BY_IP						ipm_serversByIp;		//	accessed exclusively through private IpMap methods
	MAP_SVR**						m_serversById[ TOTAL_SERVER_COUNT ];
	UINT16							m_serversCounts[ TOTAL_SERVER_COUNT ];

	//	map thread lock
	NetMapLock						m_mapLock;

	//	memory pool
	struct MEMORYPOOL *				m_pool;
};


//----------------------------------------------------------------------------
// PRIVATE INNER NET IP MAP METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void IpMapInit(
	__notnull NET_MAP * map )
{
	new (&map->ipm_serversByIp) SVRS_BY_IP(SVRS_BY_IP::key_compare(), map->m_pool);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void IpMapFree(
	__notnull NET_MAP * map,
	struct MEMORYPOOL * pool )
{
	SVRS_BY_IP::iterator iter;
	for(iter = map->ipm_serversByIp.begin(); iter != map->ipm_serversByIp.end();
			iter++)
	{
		iter->second->Servers.erase(iter->second->Servers.begin());
		FREE(pool,iter->second);
	}
	map->ipm_serversByIp.~SVRS_BY_IP();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL IpMapAdd(
	__notnull NET_MAP *	map,
	MAP_SVR &			toAdd,
	struct MEMORYPOOL * pool,
	SOCKADDR_STORAGE &	ip,
	SVRTYPE				type,
	SVRMACHINEINDEX		mIndex )
{
	MACHINE_SERVERS * pNew = NULL;
	SVRS_BY_IP::iterator iter;
	BOOL bRet = FALSE;
	
	iter = map->ipm_serversByIp.find(ip);
	if( iter == map->ipm_serversByIp.end())
	{
		pNew = (MACHINE_SERVERS*)MALLOCZ(pool,sizeof(*pNew));
		if(!pNew)
		{
			TraceError("Memory allocation failed in %!FUNC!\n");
			goto done;
		}
		new (&pNew->Servers) SVRS_BY_INDEX(SVRS_BY_INDEX::key_compare(), pool);
		if(!map->ipm_serversByIp.insert(std::make_pair(ip,pNew)).second)
		{
			pNew->Servers.~SVRS_BY_INDEX();
			FREE(pool,pNew);
			goto done;
		}
	}
	else
	{
		pNew = iter->second;
	}
	if(pNew)
	{
		//	add the entry
		if(pNew->Servers.insert(std::make_pair(IpMapId( type, mIndex ),toAdd)).second == true)
		{
			bRet = TRUE;
		}
	}

done:
	return bRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void IpMapRemove(
	__notnull NET_MAP *	map,
	SOCKADDR_STORAGE&	ip,
	SVRTYPE				type,
	SVRMACHINEINDEX		mIndex )
{
	MACHINE_SERVERS * entry = NULL;
	SVRS_BY_IP::iterator iter;

	//	get the machine entry
	iter = map->ipm_serversByIp.find(ip);
	ASSERT_RETURN( iter != map->ipm_serversByIp.end() );

	//	remove the server
	entry = iter->second;
	entry->Servers.erase( IpMapId( type, mIndex ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static  BOOL IpMapGet(
	__notnull NET_MAP *	map,
	MAP_SVR &			toGet,
	SOCKADDR_STORAGE &	ip,
	SVRTYPE				type,
	SVRMACHINEINDEX		mIndex )
{
	MACHINE_SERVERS * entry = NULL;
	SVRS_BY_IP::iterator iter;
	SVRS_BY_INDEX::iterator iter2;

	iter = map->ipm_serversByIp.find(ip);
	if( iter == map->ipm_serversByIp.end() )
	{
		return FALSE;
	}

	entry = iter->second;

	//	return the svr
	iter2 = entry->Servers.find( IpMapId( type, mIndex ) );
	if(iter2 == entry->Servers.end())
	{
		return FALSE;
	}
	toGet = iter2->second;

	return TRUE;
}


//----------------------------------------------------------------------------
// MAP PORT MATH METHODS
// NOTE: math based on ports mapped according to a 4 dim array as such:
//			channelPortLookup[ typeOffering ]
//								[ typeOfferedTo ]
//									[ typeOfferingMachineIndex ]
//										[ typeOfferedToMachineIndex ];
//			This gives a unique port # for two directions of communications
//				for any type of server to any other type with either server
//				on any machine index.
// CONS: this uses alot of port address'. svrtypes^2 * maxsvrpermachine^2 to be exact.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define PORT_COUNT			( MAX_SVR_INSTANCES_PER_MACHINE * MAX_SVR_INSTANCES_PER_MACHINE * TOTAL_SERVER_COUNT * TOTAL_SERVER_COUNT )
#define PORT_DIM_1_STEP		( MAX_SVR_INSTANCES_PER_MACHINE * MAX_SVR_INSTANCES_PER_MACHINE * TOTAL_SERVER_COUNT )
#define PORT_DIM_2_STEP		( MAX_SVR_INSTANCES_PER_MACHINE * MAX_SVR_INSTANCES_PER_MACHINE )
#define PORT_DIM_3_STEP		( MAX_SVR_INSTANCES_PER_MACHINE )
#define PORT_DIM_4_STEP		( 1 )

//----------------------------------------------------------------------------
// returns port in network order
//----------------------------------------------------------------------------
 SVRPORT NetMapGetCommunicationPort(
	SVRTYPE			offeringType,
	SVRMACHINEINDEX	offeringMachineIndex,
	SVRTYPE			userType,
	SVRMACHINEINDEX	userMachineIndex )
{
	ASSERT_RETX(offeringType		 < TOTAL_SERVER_COUNT,				INVALID_SVRPORT);
	ASSERT_RETX(offeringMachineIndex < MAX_SVR_INSTANCES_PER_MACHINE,	INVALID_SVRPORT);
	ASSERT_RETX(userType			 < TOTAL_SERVER_COUNT,				INVALID_SVRPORT);
	ASSERT_RETX(userMachineIndex	 < MAX_SVR_INSTANCES_PER_MACHINE,	INVALID_SVRPORT);

	//	4 dimensional array index
	return	htons(INTERNAL_SERVERS_START_PORT +					//	add cluster specified offset
			( offeringType			* PORT_DIM_1_STEP ) +
			( userType				* PORT_DIM_2_STEP ) +
			( offeringMachineIndex	* PORT_DIM_3_STEP ) +
			( userMachineIndex ));
}

 //----------------------------------------------------------------------------
 // expects port to be in network order
//----------------------------------------------------------------------------
CHANNEL_SERVERS	NetMapGetPortUsers(
	SVRPORT port )
{
	CHANNEL_SERVERS toRet = { (USHORT)-1, (USHORT)-1, (USHORT)-1, (USHORT)-1 };
	port = ntohs(port);
	if( port < INTERNAL_SERVERS_START_PORT )
		return toRet;
	if( port >= INTERNAL_SERVERS_START_PORT + PORT_COUNT )
		return toRet;
	
	port -= INTERNAL_SERVERS_START_PORT;		//	remove cluster specified offset

	//	4 dimensional array de-index...
	toRet.OfferingType			=	port /  PORT_DIM_1_STEP;
									port %= PORT_DIM_1_STEP;
	toRet.UserType				=	port /  PORT_DIM_2_STEP;
									port %= PORT_DIM_2_STEP;
	toRet.OfferingMachineIndex	=	port /  PORT_DIM_3_STEP;
	toRet.UserMachineIndex		=	port %  PORT_DIM_3_STEP;

	return toRet;
}


//----------------------------------------------------------------------------
// NETWORK MAP INTERFACE
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
NET_MAP * NetMapNew(
	struct MEMORYPOOL * pool )
{
	//	alloc a new map struct
	NET_MAP * toRet = (NET_MAP*)MALLOCZ( pool, sizeof( NET_MAP ) );
	ASSERT_RETNULL( toRet );

	toRet->m_mapLock.Init();
	toRet->m_pool = pool;

	//	init the ip map
	IpMapInit( toRet );

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NetMapFree(
	NET_MAP *	toFree )
{
	if( !toFree )
	{
		return;
	}

	//	free the ip map
	IpMapFree( toFree, toFree->m_pool );

	//	free the members
	toFree->m_mapLock.Free();

	//	free the memory
	for( int ii = 0; ii < TOTAL_SERVER_COUNT; ++ii )
	{
		if( toFree->m_serversById[ ii ] )
		{
			for( int jj = 0; jj < toFree->m_serversCounts[ii]; ++jj )
			{
				if(toFree->m_serversById[ii][jj])
					FREE(toFree->m_pool,toFree->m_serversById[ii][jj]);
			}
			FREE( toFree->m_pool, toFree->m_serversById[ ii ] );
		}
	}
	FREE( toFree->m_pool, toFree );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetMapSetServerInstanceCount(
	NET_MAP *	map,
	SVRTYPE		svrType,
	DWORD		svrCount )
{
	ASSERT_RETFALSE( map );
	ASSERT_RETFALSE( svrCount < 0xFFFF );
	ASSERT_RETFALSE( svrType < TOTAL_SERVER_COUNT );
	ASSERT_RETFALSE( !map->m_serversById[ svrType ] );

	HLRWWriteLock lock = &map->m_mapLock;

	map->m_serversCounts[ svrType ] = (UINT16)svrCount;
	map->m_serversById[ svrType ]   = (MAP_SVR**)MALLOCZ( map->m_pool, sizeof(MAP_SVR*) * svrCount );
	ASSERT_RETFALSE( map->m_serversById[ svrType ] );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetMapAddServer(
	NET_MAP *	map,
	SERVER &	svr )
{
	ASSERT_RETFALSE( map );
	ASSERT_RETFALSE( svr.svrType < TOTAL_SERVER_COUNT );
	ASSERT_RETFALSE( map->m_serversById[ svr.svrType ] );
	ASSERT_RETFALSE( svr.svrInstance < map->m_serversCounts[ svr.svrType ] );
	ASSERT_RETFALSE( !map->m_serversById[ svr.svrType ][ svr.svrInstance ] );

	HLRWWriteLock lock = &map->m_mapLock;

	//	add it to the ip map		
	MAP_SVR * added = (MAP_SVR*)MALLOCZ(map->m_pool,sizeof(MAP_SVR));
	ASSERT_RETFALSE(added);
		
	//	fill in the members
	added->svrType			= svr.svrType;
	added->svrInstance		= svr.svrInstance;
	added->svrMachineIndex	= svr.svrMachineIndex;
	added->svrIpAddress		= svr.svrIpAddress;

	ASSERT_GOTO(
		IpMapAdd(
			map,
			*added,
			map->m_pool,
			svr.svrIpAddress,
			svr.svrType,
			svr.svrMachineIndex ),Error);

	//	add it to the type table
	map->m_serversById[ svr.svrType ][ svr.svrInstance ] = added;

	return TRUE;
Error:
	if(added)
	{
		FREE(map->m_pool,added);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetMapRemoveServer(
	NET_MAP *	map,
	SERVER &	svr )
{
	ASSERT_RETFALSE( map );
	ASSERT_RETFALSE( svr.svrType < TOTAL_SERVER_COUNT );
	ASSERT_RETFALSE( map->m_serversById[ svr.svrType ] );
	ASSERT_RETFALSE( svr.svrInstance < map->m_serversCounts[ svr.svrType ] );

	HLRWWriteLock lock = &map->m_mapLock;

	MAP_SVR toFind;
	ASSERT_RETFALSE( IpMapGet( map, toFind, svr.svrIpAddress, svr.svrType, svr.svrMachineIndex ) );
	ASSERT_RETFALSE( map->m_serversById[ svr.svrType ][ svr.svrInstance ] );

	//	remove it from the hash
	IpMapRemove(
		map,
		svr.svrIpAddress,
		svr.svrType,
		svr.svrMachineIndex );

	//	remove it from the type lookup
	FREE(map->m_pool,map->m_serversById[ svr.svrType ][ svr.svrInstance ]);
	map->m_serversById[ svr.svrType ][ svr.svrInstance ] = NULL;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetMapGetServerAddrForUser(
	NET_MAP *			map,
	SVRTYPE				svrType,
	SVRINSTANCE			svrInstance,
	SVRTYPE				userType,
	SVRMACHINEINDEX		userMachineIndex,
	SOCKADDR_STORAGE &	svrAddr )
{
	ASSERT_RETFALSE( map );
	ASSERT_RETFALSE( svrType < TOTAL_SERVER_COUNT );
	ASSERT_RETFALSE( map->m_serversById[ svrType ] );
	ASSERT_RETFALSE( svrInstance < map->m_serversCounts[ svrType ] );

	HLRWReadLock lock = &map->m_mapLock;

	//	get the server entry
	MAP_SVR * entry = map->m_serversById[ svrType ][ svrInstance ];
	if( !entry )
	{
		//	there is no entry for this server instance yet.
		return FALSE;
	}

	svrAddr = entry->svrIpAddress;
	SVRPORT port = 	NetMapGetCommunicationPort(
									svrType,
									entry->svrMachineIndex,
									userType,
									userMachineIndex);
	ASSERT_RETFALSE(port != INVALID_SVRPORT);

	SS_PORT(&svrAddr) = port;
	return TRUE;
}

//----------------------------------------------------------------------------
//	returns local machine index in provider instance slot
//----------------------------------------------------------------------------
SERVICEID NetMapGetServiceIdFromUserAddress(
	NET_MAP *			map,
	SOCKADDR_STORAGE &	remoteAddress,
	SVRPORT				port)
{
	ASSERT_RETX( map, INVALID_SERVICEID );

	HLRWReadLock lock = &map->m_mapLock;

	//	get our lookup info
	CHANNEL_SERVERS svrs = NetMapGetPortUsers( port );
	ASSERT_RETX(svrs.UserType != INVALID_SVRTYPE, INVALID_SERVICEID);

	//	get the entry
	MAP_SVR  entry;
	if(!IpMapGet( map, entry,remoteAddress, svrs.UserType, svrs.UserMachineIndex ))
	{
		return INVALID_SERVICEID;
	}

	//	compute the service id
	return ServiceId(
				svrs.OfferingType,
				svrs.OfferingMachineIndex,	//	HACK: returns provider machine index in provider instance slot as that is what the net layer really needs...
				svrs.UserType,
				entry.svrInstance );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD NetMapGetServiceInstanceCount(
	NET_MAP *	map,
	SVRTYPE		svrType )
{
	ASSERT_RETZERO( map );
	ASSERT_RETZERO( svrType < TOTAL_SERVER_COUNT );
	return map->m_serversCounts[ svrType ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SVRMACHINEINDEX NetMapGetServerMachineIndex(
	NET_MAP *	map,
	SVRID       server )
{
	ASSERT_RETX( map, INVALID_SVRMACHINEINDEX );
	if( server == INVALID_SVRID )
		return INVALID_SVRMACHINEINDEX;

	SVRTYPE		svrType     = ServerIdGetType( server );
	SVRINSTANCE svrInstance = ServerIdGetInstance( server );

	ASSERT_RETX( svrType < TOTAL_SERVER_COUNT,					INVALID_SVRMACHINEINDEX );
	ASSERT_RETX( map->m_serversById[svrType],					INVALID_SVRMACHINEINDEX );
	ASSERT_RETX( svrInstance < map->m_serversCounts[svrType],	INVALID_SVRMACHINEINDEX );
	ASSERT_RETX( map->m_serversById[svrType][svrInstance],		INVALID_SVRMACHINEINDEX );

	return map->m_serversById[svrType][svrInstance]->svrMachineIndex;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool sockaddr_compare::operator()(
	const SOCKADDR_STORAGE& lhs, 
	const SOCKADDR_STORAGE&rhs) const
{
	if(lhs.ss_family != rhs.ss_family)
	{
		return lhs.ss_family < rhs.ss_family;
	}
	if(lhs.ss_family == AF_INET)
	{
		struct sockaddr_in  *lh4 = (struct sockaddr_in*)&lhs,
							*rh4 = (struct sockaddr_in*)&rhs;
		return lh4->sin_addr.s_addr < rh4->sin_addr.s_addr ;
	}
	if(lhs.ss_family == AF_INET6)
	{
		struct sockaddr_in6 *lh6 = (struct sockaddr_in6*)&lhs,
							*rh6 = (struct sockaddr_in6*)&rhs;
		if(lh6->sin6_scope_id != rh6->sin6_scope_id)
		{
			return (lh6->sin6_scope_id < rh6->sin6_scope_id);
		}
		return ( memcmp(&lh6->sin6_addr,&rh6->sin6_addr,sizeof(lh6->sin6_addr)) < 0);
	}
	return false;
}
