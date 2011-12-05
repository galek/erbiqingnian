//----------------------------------------------------------------------------
// servers.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef	_SERVERS_H_
#define _SERVERS_H_


//----------------------------------------------------------------------------
// SERVER LIST MACROS
//----------------------------------------------------------------------------
#ifndef START_SERVER_LIST
#define START_SERVER_LIST

#define HYBRID_SERVER_TYPE( typeName )	typeName,
#define PURE_SERVER_TYPE(   typeName )	typeName,

#if ISVERSION(SERVER_VERSION)
#define START_COMMON_TYPES				enum INTERNAL_SERVER_TYPES {
#define START_PURE_TYPES 
#define END_SERVER_LIST					TOTAL_SERVER_COUNT };

#else
#define START_COMMON_TYPES				enum INTERNAL_SERVER_TYPES {
#define START_PURE_TYPES				TOTAL_SERVER_COUNT }; \
										enum REMOTE_SERVER_TYPES { \
										_FIRST_REMOTE_SERVER_TYPE = (INT)TOTAL_SERVER_COUNT - 1,
#define END_SERVER_LIST					TOTAL_SERVER_TYPE_COUNT };
#endif


#endif	//	START_SERVER_LIST


//----------------------------------------------------------------------------
// SERVER START PORT
// Arbitrary starting port number for the internal server communication 
//	channels. The port range used is == INTERNAL_SERVERS_START_PORT to
//	INTERNAL_SERVERS_START_PORT + 
//	( TOTAL_SERVER_COUNT * TOTAL_SERVER_COUNT *
//	  MAX_SVR_INSTANCES_PER_MACHINE * MAX_SVR_INSTANCES_PER_MACHINE )
//----------------------------------------------------------------------------
#define INTERNAL_SERVERS_START_PORT		5053


#endif	//	_SERVERS_H_
#ifndef _SERVER_TYPE_DEFS_
#define _SERVER_TYPE_DEFS_


//----------------------------------------------------------------------------
// SERVER TYPE LIST
// Servers may be one of two types, pure, or hybrid.
// Pure servers are only linked and run in data center cluster server builds.
// Hybrid servers are linked and run on both platforms.
//
// By defining a server type you are telling the server runner API that
//		you will define a valid server specification of the name
//		SERVER_SPECIFICATION_NAME( <server type name> ) filled with
//		valid data, callbacks, and connection table definition.
//
// SEE: ServerSpecification.h and TestServers.h/cpp for usage and examples.
//----------------------------------------------------------------------------
START_SERVER_LIST

	START_COMMON_TYPES
	HYBRID_SERVER_TYPE( USER_SERVER )
	HYBRID_SERVER_TYPE( GAME_SERVER )
	HYBRID_SERVER_TYPE( CHAT_SERVER )

	START_PURE_TYPES
	PURE_SERVER_TYPE(   LOGIN_SERVER )
	PURE_SERVER_TYPE(   PATCH_SERVER )
	PURE_SERVER_TYPE(   GAME_LOADBALANCE_SERVER )
//	PURE_SERVER_TYPE(   FILLPAK_SERVER )
	PURE_SERVER_TYPE(   BILLING_PROXY )
	PURE_SERVER_TYPE(   CSR_BRIDGE_SERVER )
	PURE_SERVER_TYPE(   BATTLE_SERVER )
	PURE_SERVER_TYPE(   GLOBAL_GAME_EVENT_SERVER )
	PURE_SERVER_TYPE(   AUCTION_SERVER )

END_SERVER_LIST


#endif	//	_SERVER_TYPE_DEFS_
#ifndef _SERVER_FUNCTIONS_
#define _SERVER_FUNCTIONS_

//	not for tracing! use the !ServerType! trace macro for server types.
const char *
	ServerGetName(
		UINT serverType );
UINT
	ServerGetType(
		const WCHAR * serverName );

#endif  _SERVER_FUNCTIONS_
